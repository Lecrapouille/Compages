// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/ShaderSource.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace compages::gpu
{

namespace
{

constexpr std::string_view HOOK_PREFIX = "hook/";

// ----------------------------------------------------------------------------
//! \brief The line without its spaces in front.
// ----------------------------------------------------------------------------
std::string_view trimFront(std::string_view p_line)
{
    const std::size_t first = p_line.find_first_not_of(" \t");
    return (first == std::string_view::npos) ? std::string_view{}
                                             : p_line.substr(first);
}

// ----------------------------------------------------------------------------
//! \brief True when the line is the directive (spaces allowed after '#').
// ----------------------------------------------------------------------------
bool isDirective(std::string_view p_line, std::string_view p_word)
{
    std::string_view rest = trimFront(p_line);
    if (rest.empty() || (rest.front() != '#'))
    {
        return false;
    }
    rest = trimFront(rest.substr(1u));
    return rest.starts_with(p_word) &&
           ((rest.size() == p_word.size()) ||
            (std::string_view(" \t\"<").find(rest[p_word.size()]) !=
             std::string_view::npos));
}

// ----------------------------------------------------------------------------
//! \brief Cut a text into lines, without their end of line.
// ----------------------------------------------------------------------------
std::vector<std::string_view> linesOf(std::string_view p_text)
{
    std::vector<std::string_view> lines;
    std::size_t start = 0u;
    while (start <= p_text.size())
    {
        const std::size_t end = p_text.find('\n', start);
        if (end == std::string_view::npos)
        {
            if (start < p_text.size())
            {
                lines.push_back(p_text.substr(start));
            }
            break;
        }
        lines.push_back(p_text.substr(start, end - start));
        start = end + 1u;
    }
    return lines;
}

// ----------------------------------------------------------------------------
//! \brief Read a whole file.
// ----------------------------------------------------------------------------
Result<std::string> readText(std::filesystem::path const& p_path)
{
    std::ifstream file(p_path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return failure("cannot read the shader '" + p_path.string() + "'");
    }
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

// ****************************************************************************
//! \brief Puts the includes of one ShaderSource in, depth first.
// ****************************************************************************
class Resolver
{
public:

    Resolver(std::string_view p_name,
             std::vector<std::filesystem::path> const& p_folders,
             std::map<std::string, std::string, std::less<>> const& p_includes,
             std::map<std::string, std::string, std::less<>> const& p_hooks,
             std::map<std::string, std::string, std::less<>> const& p_replaced)
        : m_folders(p_folders),
          m_includes(p_includes),
          m_hooks(p_hooks),
          m_replaced(p_replaced)
    {
        m_out.pieces.emplace_back(p_name);
        m_stack.emplace_back(p_name);
    }

    // ------------------------------------------------------------------------
    //! \brief The main text: its \#version, the defines, then the rest.
    // ------------------------------------------------------------------------
    Status main(std::string_view p_text,
                std::filesystem::path const& p_folder,
                std::vector<std::pair<std::string, std::string>> const& p_defines)
    {
        const std::vector<std::string_view> lines = linesOf(p_text);
        auto version = std::find_if(lines.begin(),
                                    lines.end(),
                                    [](std::string_view p_line)
                                    { return isDirective(p_line, "version"); });
        if (version == lines.end())
        {
            return failure("'" + m_out.pieces.front() +
                           "' has no #version: the main text of a shader "
                           "starts with it");
        }
        const auto at = std::size_t(version - lines.begin());
        for (std::size_t i = 0u; i <= at; ++i)
        {
            m_out.text.append(lines[i]).push_back('\n');
        }
        for (auto const& [name, value] : p_defines)
        {
            m_out.text += "#define " + name;
            if (!value.empty())
            {
                m_out.text += " " + value;
            }
            m_out.text.push_back('\n');
        }
        return body(lines, at + 1u, 0u, p_folder);
    }

    ResolvedSource take()
    {
        return std::move(m_out);
    }

    void read(std::filesystem::path p_file)
    {
        m_out.files.push_back(std::move(p_file));
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Copy lines from p_first, putting each include in.
    // ------------------------------------------------------------------------
    Status body(std::vector<std::string_view> const& p_lines,
                std::size_t p_first,
                std::size_t p_piece,
                std::filesystem::path const& p_folder)
    {
        lineMark(p_first + 1u, p_piece);
        for (std::size_t i = p_first; i < p_lines.size(); ++i)
        {
            std::string_view const line = p_lines[i];
            if (isDirective(line, "version"))
            {
                return failure("'" + m_out.pieces[p_piece] +
                               "' has a #version: only the main text has one");
            }
            if (!isDirective(line, "include"))
            {
                m_out.text.append(line).push_back('\n');
                continue;
            }
            COMPAGES_TRY(include(line, p_piece, p_folder));
            lineMark(i + 2u, p_piece);
        }
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief The next line is line p_line of piece p_piece.
    // ------------------------------------------------------------------------
    void lineMark(std::size_t p_line, std::size_t p_piece)
    {
        m_out.text += "#line " + std::to_string(p_line) + " " +
                      std::to_string(p_piece) + "\n";
    }

    // ------------------------------------------------------------------------
    //! \brief One \#include line.
    // ------------------------------------------------------------------------
    Status include(std::string_view p_line,
                   std::size_t p_parent,
                   std::filesystem::path const& p_folder)
    {
        std::string const& parent = m_out.pieces[p_parent];
        const std::size_t open = p_line.find_first_of("\"<");
        const char close = (open == std::string_view::npos)
                               ? '\0'
                               : ((p_line[open] == '"') ? '"' : '>');
        const std::size_t end = (open == std::string_view::npos)
                                    ? std::string_view::npos
                                    : p_line.find(close, open + 1u);
        if (end == std::string_view::npos)
        {
            return failure("'" + parent + "' has a bad #include: '" +
                           std::string(p_line) + "'");
        }
        const std::string name(p_line.substr(open + 1u, end - open - 1u));

        std::string text;
        std::string key;
        std::filesystem::path folder;
        if (name.starts_with(HOOK_PREFIX))
        {
            const std::string_view hook =
                std::string_view(name).substr(HOOK_PREFIX.size());
            auto declared = m_hooks.find(hook);
            if (declared == m_hooks.end())
            {
                return failure("'" + parent + "' includes '" + name +
                               "', but no hook '" + std::string(hook) +
                               "' is declared");
            }
            auto replaced = m_replaced.find(hook);
            text = (replaced != m_replaced.end()) ? replaced->second
                                                  : declared->second;
            key = name;
        }
        else if (auto given = m_includes.find(name); given != m_includes.end())
        {
            text = given->second;
            key = name;
        }
        else
        {
            std::filesystem::path found;
            if ((close == '"') && !p_folder.empty() &&
                std::filesystem::is_regular_file(p_folder / name))
            {
                found = p_folder / name;
            }
            for (auto it = m_folders.begin();
                 found.empty() && (it != m_folders.end());
                 ++it)
            {
                if (std::filesystem::is_regular_file(*it / name))
                {
                    found = *it / name;
                }
            }
            if (found.empty())
            {
                return failure("'" + parent + "' includes '" + name +
                               "', which is neither given with include() nor "
                               "found in the include directories");
            }
            found = std::filesystem::weakly_canonical(found);
            COMPAGES_TRY_ASSIGN(text, readText(found));
            key = found.string();
            folder = found.parent_path();
        }

        if (std::find(m_stack.begin(), m_stack.end(), key) != m_stack.end())
        {
            std::string chain;
            for (std::string const& piece : m_stack)
            {
                chain += piece + " -> ";
            }
            return failure("the includes go round in a circle: " + chain + key);
        }
        if (!m_done.insert(key).second)
        {
            return success(); // already in: each name once
        }
        if (!folder.empty())
        {
            read(std::filesystem::path(key));
        }
        const std::size_t piece = m_out.pieces.size();
        m_out.pieces.push_back(folder.empty() ? name : key);
        m_stack.push_back(key);
        COMPAGES_TRY(body(linesOf(text), 0u, piece, folder));
        m_stack.pop_back();
        return success();
    }

    std::vector<std::filesystem::path> const& m_folders;
    std::map<std::string, std::string, std::less<>> const& m_includes;
    std::map<std::string, std::string, std::less<>> const& m_hooks;
    std::map<std::string, std::string, std::less<>> const& m_replaced;
    ResolvedSource m_out;
    std::vector<std::string> m_stack;
    std::set<std::string> m_done;
};

} // namespace

// ----------------------------------------------------------------------------
std::string ResolvedSource::explain(std::string_view p_log) const
{
    // A source number then a line: "0:12" (Mesa, AMD) or "0(12)" (NVIDIA).
    static const std::regex where(R"((^|[^0-9A-Za-z_.])(\d+)(?::(\d+)|\((\d+)\)))");
    std::string result;
    for (std::string_view line : linesOf(p_log))
    {
        const std::string row(line);
        std::smatch match;
        if (std::regex_search(row, match, where))
        {
            const std::size_t piece = std::stoul(match[2].str());
            if (piece < pieces.size())
            {
                const std::string number =
                    match[3].matched ? match[3].str() : match[4].str();
                result += match.prefix().str() + match[1].str() +
                          pieces[piece] + ":" + number +
                          match.suffix().str() + "\n";
                continue;
            }
        }
        result += row + "\n";
    }
    return result;
}

// ----------------------------------------------------------------------------
ShaderSource ShaderSource::fromText(std::string p_name, std::string p_text)
{
    ShaderSource source;
    source.m_name = std::move(p_name);
    source.m_text = std::move(p_text);
    return source;
}

// ----------------------------------------------------------------------------
ShaderSource ShaderSource::fromFile(std::filesystem::path const& p_path)
{
    ShaderSource source;
    source.m_name = p_path.string();
    source.m_file = p_path;
    return source;
}

// ----------------------------------------------------------------------------
ShaderSource& ShaderSource::directory(std::filesystem::path p_folder)
{
    m_folders.push_back(std::move(p_folder));
    return *this;
}

// ----------------------------------------------------------------------------
ShaderSource& ShaderSource::include(std::string p_name, std::string p_text)
{
    m_includes.insert_or_assign(std::move(p_name), std::move(p_text));
    return *this;
}

// ----------------------------------------------------------------------------
ShaderSource& ShaderSource::declareHook(std::string p_name,
                                        std::string p_default)
{
    m_hooks.insert_or_assign(std::move(p_name), std::move(p_default));
    return *this;
}

// ----------------------------------------------------------------------------
ShaderSource& ShaderSource::hook(std::string p_name, std::string p_text)
{
    m_replaced.insert_or_assign(std::move(p_name), std::move(p_text));
    return *this;
}

// ----------------------------------------------------------------------------
ShaderSource& ShaderSource::define(std::string p_name, std::string p_value)
{
    m_defines.emplace_back(std::move(p_name), std::move(p_value));
    return *this;
}

// ----------------------------------------------------------------------------
Result<ResolvedSource> ShaderSource::resolve() const
{
    for (auto const& [name, text] : m_replaced)
    {
        if (!m_hooks.contains(name))
        {
            std::string known;
            for (auto const& [declared, unused] : m_hooks)
            {
                known += (known.empty() ? "" : ", ") + declared;
            }
            return failure("'" + m_name + "' has no hook '" + name +
                           "' (known: " + (known.empty() ? "none" : known) +
                           ")");
        }
    }

    Resolver resolver(m_name, m_folders, m_includes, m_hooks, m_replaced);
    std::string text = m_text;
    std::filesystem::path folder;
    if (!m_file.empty())
    {
        COMPAGES_TRY_ASSIGN(text, readText(m_file));
        if (text.empty())
        {
            return failure("the shader '" + m_name + "' is empty");
        }
        resolver.read(m_file);
        folder = m_file.parent_path();
    }
    COMPAGES_TRY(resolver.main(text, folder, m_defines));
    return resolver.take();
}

} // namespace compages::gpu
