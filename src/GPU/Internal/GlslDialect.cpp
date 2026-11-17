// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Internal/GlslDialect.hpp"
#include "GPU/Internal/GlslText.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>

namespace compages::gpu::detail
{

namespace
{

//! \brief Default precisions of OpenGL ES 3.00. A fragment shader has none
//! for float, and no shader has one for these samplers: declaring them is
//! what lets a shader written for the desktop compile unchanged.
constexpr char const* ES_PRECISIONS =
    "precision highp float; precision highp int; "
    "precision highp sampler2D; precision highp sampler3D; "
    "precision highp samplerCube; precision highp sampler2DArray; "
    "precision highp sampler2DShadow; precision highp samplerCubeShadow; "
    "precision highp sampler2DArrayShadow; "
    "precision highp isampler2D; precision highp isampler3D; "
    "precision highp isamplerCube; precision highp isampler2DArray; "
    "precision highp usampler2D; precision highp usampler3D; "
    "precision highp usamplerCube; precision highp usampler2DArray;";

//! \brief A line of the text, and what the compiler will call it.
struct Line
{
    std::size_t begin = 0u;
    std::size_t end = 0u; //!< Before the newline.
    //! \brief Its line number for the compiler, `#line` taken into account.
    int number = 1;
    //! \brief The directive, "version" for `#version 450 core`; empty if none.
    std::string_view directive;
    //! \brief What follows the directive name.
    std::string_view arguments;
};

std::vector<Line> splitLines(std::string const& p_code)
{
    std::vector<Line> lines;
    int number = 1;
    std::size_t begin = 0u;
    while (begin <= p_code.size())
    {
        std::size_t end = p_code.find('\n', begin);
        if (end == std::string::npos)
        {
            end = p_code.size();
        }
        Line line{ begin, end, number, {}, {} };
        std::string_view text = trimmed(std::string_view(p_code).substr(begin, end - begin));
        number += 1;
        if (!text.empty() && (text.front() == '#'))
        {
            text = trimmed(text.substr(1u));
            std::size_t name_end = 0u;
            while ((name_end < text.size()) && isWordChar(text[name_end]))
            {
                ++name_end;
            }
            line.directive = text.substr(0u, name_end);
            line.arguments = trimmed(text.substr(name_end));
            if ((line.directive == "line") && !line.arguments.empty() &&
                (std::isdigit(static_cast<unsigned char>(line.arguments.front())) != 0))
            {
                // The line after `#line K` is line K.
                number = std::stoi(std::string(line.arguments));
            }
        }
        lines.emplace_back(line);
        if (end == p_code.size())
        {
            break;
        }
        begin = end + 1u;
    }
    return lines;
}

//! \brief The \#version line for the dialect, or empty to keep the line.
std::string loweredVersion(std::string_view p_arguments, GlslDialect p_dialect, int& p_version)
{
    std::size_t digits = 0u;
    while ((digits < p_arguments.size()) &&
           (std::isdigit(static_cast<unsigned char>(p_arguments[digits])) != 0))
    {
        ++digits;
    }
    if (digits == 0u)
    {
        return {};
    }
    p_version = std::stoi(std::string(p_arguments.substr(0u, digits)));
    const std::string_view profile = trimmed(p_arguments.substr(digits));

    if (p_dialect == GlslDialect::Es300)
    {
        return (profile == "es") ? std::string{} : std::string("#version 300 es");
    }
    if ((profile == "es") || (p_version <= 410))
    {
        return {};
    }
    return "#version 410 core";
}

//! \brief Is \c p_item `binding = N`? Gives N.
bool isBinding(std::string_view p_item, int& p_binding)
{
    p_item = trimmed(p_item);
    constexpr std::string_view keyword = "binding";
    if ((p_item.substr(0u, keyword.size()) != keyword) ||
        ((p_item.size() > keyword.size()) && isWordChar(p_item[keyword.size()])))
    {
        return false;
    }
    p_item = trimmed(p_item.substr(keyword.size()));
    if (p_item.empty() || (p_item.front() != '='))
    {
        return false;
    }
    p_item = trimmed(p_item.substr(1u));
    if (p_item.empty() ||
        !std::all_of(p_item.begin(), p_item.end(),
                     [](char p_c) { return std::isdigit(static_cast<unsigned char>(p_c)) != 0; }))
    {
        return false;
    }
    p_binding = std::stoi(std::string(p_item));
    return true;
}

//! \brief What follows a `layout(...)`: the words up to `;`, `{` or `[`.
struct Declaration
{
    std::vector<std::string_view> words;
    bool block = false;
};

Declaration declarationAfter(std::string const& p_code, std::size_t p_from)
{
    Declaration declaration;
    std::size_t i = p_from;
    while (i < p_code.size())
    {
        const char c = p_code[i];
        if (isSpace(c))
        {
            ++i;
        }
        else if (isWordChar(c))
        {
            const std::size_t start = i;
            while ((i < p_code.size()) && isWordChar(p_code[i]))
            {
                ++i;
            }
            declaration.words.emplace_back(std::string_view(p_code).substr(start, i - start));
        }
        else
        {
            declaration.block = (c == '{');
            break;
        }
    }
    return declaration;
}

bool hasWord(Declaration const& p_declaration, std::string_view p_word)
{
    return std::find(p_declaration.words.begin(), p_declaration.words.end(), p_word) !=
           p_declaration.words.end();
}

bool declaresSampler(Declaration const& p_declaration)
{
    return std::any_of(p_declaration.words.begin(), p_declaration.words.end(),
                       [](std::string_view p_word)
                       {
                           return p_word.starts_with("sampler") || p_word.starts_with("isampler") ||
                                  p_word.starts_with("usampler");
                       });
}

//! \brief Take `binding = N` out of the layouts of samplers and uniform
//! blocks. Storage blocks and images keep theirs: the driver refuses them
//! anyway, and its message then names what is missing.
void moveBindings(std::string_view p_source,
                  std::string const& p_code,
                  std::vector<Edit>& p_edits,
                  std::vector<FixedBinding>& p_bindings)
{
    constexpr std::string_view keyword = "layout";
    std::size_t at = p_code.find(keyword);
    for (; at != std::string::npos; at = p_code.find(keyword, at + keyword.size()))
    {
        const bool starts_word = (at == 0u) || !isWordChar(p_code[at - 1u]);
        const std::size_t after = at + keyword.size();
        if (!starts_word || ((after < p_code.size()) && isWordChar(p_code[after])))
        {
            continue;
        }
        std::size_t open = after;
        while ((open < p_code.size()) && isSpace(p_code[open]))
        {
            ++open;
        }
        if ((open >= p_code.size()) || (p_code[open] != '('))
        {
            continue;
        }
        const std::size_t close = p_code.find(')', open);
        if (close == std::string::npos)
        {
            return;
        }

        // The items of the layout, binding apart.
        std::vector<std::string_view> kept;
        int binding = -1;
        std::size_t item_begin = open + 1u;
        while (item_begin <= close)
        {
            std::size_t item_end = p_code.find(',', item_begin);
            if ((item_end == std::string::npos) || (item_end > close))
            {
                item_end = close;
            }
            const std::string_view item = p_source.substr(item_begin, item_end - item_begin);
            int number = 0;
            if (isBinding(std::string_view(p_code).substr(item_begin, item_end - item_begin), number))
            {
                binding = number;
            }
            else if (!trimmed(item).empty())
            {
                kept.emplace_back(trimmed(item));
            }
            item_begin = item_end + 1u;
        }
        if (binding < 0)
        {
            continue;
        }

        const Declaration declaration = declarationAfter(p_code, close + 1u);
        if (declaration.words.empty() || !hasWord(declaration, "uniform"))
        {
            continue;
        }
        const bool sampler = !declaration.block && declaresSampler(declaration);
        if (!declaration.block && !sampler)
        {
            continue;
        }
        p_bindings.push_back(FixedBinding{ std::string(declaration.words.back()), binding, declaration.block });

        std::string text;
        if (!kept.empty())
        {
            text = "layout(";
            for (std::size_t k = 0u; k < kept.size(); ++k)
            {
                text += (k == 0u) ? "" : ", ";
                text += kept[k];
            }
            text += ")";
        }
        p_edits.push_back(Edit{ at, close + 1u - at, std::move(text) });
    }
}

} // namespace

TranslatedGlsl translateGlsl(std::string_view p_source, GlslDialect p_dialect)
{
    TranslatedGlsl out;
    const std::string code = blankComments(p_source);
    const std::vector<Line> lines = splitLines(code);
    std::vector<Edit> edits;

    // The version, and after it (and after the extensions, which must come
    // before any statement on ES) what the dialect needs.
    std::size_t version = lines.size();
    std::size_t last_header = lines.size();
    for (std::size_t i = 0u; i < lines.size(); ++i)
    {
        if ((lines[i].directive == "version") && (version == lines.size()))
        {
            version = i;
            last_header = i;
            std::string lowered = loweredVersion(lines[i].arguments, p_dialect, out.source_version);
            if (!lowered.empty())
            {
                edits.push_back(Edit{ lines[i].begin, lines[i].end - lines[i].begin, std::move(lowered) });
            }
        }
        else if ((lines[i].directive == "extension") && (version != lines.size()))
        {
            last_header = i;
        }
    }
    if ((version != lines.size()) && (p_dialect == GlslDialect::Es300))
    {
        Line const& header = lines[last_header];
        const int next = (last_header + 1u < lines.size()) ? lines[last_header + 1u].number : header.number + 1;
        std::string added = std::string(ES_PRECISIONS) + "\n#line " + std::to_string(next) + "\n";
        if (header.end == code.size())
        {
            added = "\n" + added;
        }
        const std::size_t at = (header.end == code.size()) ? header.end : header.end + 1u;
        edits.push_back(Edit{ at, 0u, std::move(added) });
    }

    moveBindings(p_source, code, edits, out.bindings);

    out.text = applyEdits(p_source, std::move(edits));
    return out;
}

} // namespace compages::gpu::detail
