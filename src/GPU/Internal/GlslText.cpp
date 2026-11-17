// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Internal/GlslText.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <set>

namespace compages::gpu::detail
{

std::string applyEdits(std::string_view p_text, std::vector<Edit> p_edits)
{
    std::sort(p_edits.begin(),
              p_edits.end(),
              [](Edit const& p_a, Edit const& p_b)
              { return (p_a.at != p_b.at) ? (p_a.at > p_b.at) : (p_a.length > p_b.length); });
    std::string out(p_text);
    for (Edit const& edit : p_edits)
    {
        out.replace(edit.at, edit.length, edit.text);
    }
    return out;
}

bool isWordChar(char p_c)
{
    return (std::isalnum(static_cast<unsigned char>(p_c)) != 0) || (p_c == '_');
}

bool isSpace(char p_c)
{
    return (p_c == ' ') || (p_c == '\t') || (p_c == '\r') || (p_c == '\n');
}

std::string_view trimmed(std::string_view p_text)
{
    while (!p_text.empty() && isSpace(p_text.front()))
    {
        p_text.remove_prefix(1u);
    }
    while (!p_text.empty() && isSpace(p_text.back()))
    {
        p_text.remove_suffix(1u);
    }
    return p_text;
}

std::string blankComments(std::string_view p_source)
{
    std::string code(p_source);
    std::size_t i = 0u;
    while (i < code.size())
    {
        if ((code[i] == '/') && (i + 1u < code.size()) && (code[i + 1u] == '/'))
        {
            while ((i < code.size()) && (code[i] != '\n'))
            {
                code[i++] = ' ';
            }
        }
        else if ((code[i] == '/') && (i + 1u < code.size()) && (code[i + 1u] == '*'))
        {
            code[i++] = ' ';
            code[i++] = ' ';
            while ((i < code.size()) && !((code[i] == '*') && (i + 1u < code.size()) && (code[i + 1u] == '/')))
            {
                if (code[i] != '\n')
                {
                    code[i] = ' ';
                }
                ++i;
            }
            for (int k = 0; (k < 2) && (i < code.size()); ++k)
            {
                code[i++] = ' ';
            }
        }
        else
        {
            ++i;
        }
    }
    return code;
}

std::string blankDirectives(std::string_view p_code)
{
    std::string code(p_code);
    bool continued = false;
    std::size_t begin = 0u;
    while (begin < code.size())
    {
        std::size_t end = code.find('\n', begin);
        if (end == std::string::npos)
        {
            end = code.size();
        }
        const std::string_view line = trimmed(std::string_view(code).substr(begin, end - begin));
        const bool directive = continued || (!line.empty() && (line.front() == '#'));
        continued = directive && !line.empty() && (line.back() == '\\');
        if (directive)
        {
            std::fill(code.begin() + static_cast<std::ptrdiff_t>(begin),
                      code.begin() + static_cast<std::ptrdiff_t>(end),
                      ' ');
        }
        begin = end + 1u;
    }
    return code;
}

int lineNumberAt(std::string_view p_code, std::size_t p_at)
{
    int number = 1;
    std::size_t begin = 0u;
    while (begin <= p_at && begin < p_code.size())
    {
        std::size_t end = p_code.find('\n', begin);
        if (end == std::string_view::npos)
        {
            end = p_code.size();
        }
        if (p_at <= end)
        {
            return number;
        }
        std::string_view line = trimmed(p_code.substr(begin, end - begin));
        number += 1;
        if (!line.empty() && (line.front() == '#'))
        {
            line = trimmed(line.substr(1u));
            if (line.starts_with("line"))
            {
                const std::string_view rest = trimmed(line.substr(4u));
                if (!rest.empty() && (std::isdigit(static_cast<unsigned char>(rest.front())) != 0))
                {
                    number = std::stoi(std::string(rest));
                }
            }
        }
        begin = end + 1u;
    }
    return number;
}

std::size_t matchingBracket(std::string_view p_code, std::size_t p_open)
{
    int depth = 0;
    for (std::size_t i = p_open; i < p_code.size(); ++i)
    {
        const char c = p_code[i];
        if ((c == '(') || (c == '[') || (c == '{'))
        {
            ++depth;
        }
        else if ((c == ')') || (c == ']') || (c == '}'))
        {
            --depth;
            if (depth == 0)
            {
                return i;
            }
        }
    }
    return std::string_view::npos;
}

std::string_view wordAt(std::string_view p_code, std::size_t p_at)
{
    std::size_t end = p_at;
    while ((end < p_code.size()) && isWordChar(p_code[end]))
    {
        ++end;
    }
    return p_code.substr(p_at, end - p_at);
}

std::size_t skipSpaces(std::string_view p_code, std::size_t p_at)
{
    while ((p_at < p_code.size()) && isSpace(p_code[p_at]))
    {
        ++p_at;
    }
    return p_at;
}

} // namespace compages::gpu::detail

// ****************************************************************************
// A small preprocessor: only what is needed to know which lines are kept.
// ****************************************************************************

namespace compages::gpu::detail
{

namespace
{

struct Macros
{
    std::map<std::string, std::string, std::less<>> values;
    std::set<std::string, std::less<>> functions;
};

//! \brief The tokens of a condition: numbers, names, operators.
std::vector<std::string> tokensOf(std::string_view p_text)
{
    std::vector<std::string> tokens;
    std::size_t i = 0u;
    while (i < p_text.size())
    {
        const char c = p_text[i];
        if (isSpace(c))
        {
            ++i;
        }
        else if (isWordChar(c))
        {
            const std::string_view word = wordAt(p_text, i);
            tokens.emplace_back(word);
            i += word.size();
        }
        else
        {
            static constexpr std::string_view twos[] = { "||", "&&", "==", "!=", "<=", ">=", "<<", ">>" };
            const std::string_view two = p_text.substr(i, 2u);
            const bool pair = std::find(std::begin(twos), std::end(twos), two) != std::end(twos);
            tokens.emplace_back(p_text.substr(i, pair ? 2u : 1u));
            i += pair ? 2u : 1u;
        }
    }
    return tokens;
}

//! \brief Replace `defined X` and the macros by their values; what is left
//! unknown is 0, as in C.
bool expand(std::vector<std::string> const& p_in, Macros const& p_macros, std::vector<std::string>& p_out, int p_depth)
{
    if (p_depth > 32)
    {
        return false;
    }
    for (std::size_t i = 0u; i < p_in.size(); ++i)
    {
        std::string const& token = p_in[i];
        if (token == "defined")
        {
            std::size_t k = i + 1u;
            const bool paren = (k < p_in.size()) && (p_in[k] == "(");
            k += paren ? 1u : 0u;
            if (k >= p_in.size())
            {
                return false;
            }
            const bool is = p_macros.values.contains(p_in[k]) || p_macros.functions.contains(p_in[k]);
            p_out.emplace_back(is ? "1" : "0");
            i = k + (paren ? 1u : 0u);
        }
        else if (!token.empty() && (std::isalpha(static_cast<unsigned char>(token[0])) != 0 || token[0] == '_'))
        {
            auto found = p_macros.values.find(token);
            if (found != p_macros.values.end())
            {
                if (!expand(tokensOf(found->second), p_macros, p_out, p_depth + 1))
                {
                    return false;
                }
            }
            else
            {
                if (p_macros.functions.contains(token) && (i + 1u < p_in.size()) && (p_in[i + 1u] == "("))
                {
                    int depth = 0;
                    for (++i; i < p_in.size(); ++i)
                    {
                        depth += (p_in[i] == "(") ? 1 : (p_in[i] == ")") ? -1 : 0;
                        if (depth == 0)
                        {
                            break;
                        }
                    }
                }
                p_out.emplace_back("0");
            }
        }
        else
        {
            p_out.push_back(token);
        }
    }
    return true;
}

//! \brief The value of a condition, by recursive descent with the precedence
//! of C.
class Condition
{
public:

    explicit Condition(std::vector<std::string> p_tokens) : m_tokens(std::move(p_tokens)) {}

    bool evaluate(long long& p_value)
    {
        p_value = ternary();
        return m_ok && (m_at == m_tokens.size());
    }

private:

    bool accept(std::string_view p_token)
    {
        if ((m_at < m_tokens.size()) && (m_tokens[m_at] == p_token))
        {
            ++m_at;
            return true;
        }
        return false;
    }

    long long ternary()
    {
        const long long condition = binary(0);
        if (!accept("?"))
        {
            return condition;
        }
        const long long yes = ternary();
        m_ok = m_ok && accept(":");
        const long long no = ternary();
        return (condition != 0) ? yes : no;
    }

    long long binary(int p_level)
    {
        static const std::vector<std::vector<std::string_view>> levels = {
            { "||" }, { "&&" }, { "|" }, { "^" }, { "&" }, { "==", "!=" },
            { "<", ">", "<=", ">=" }, { "<<", ">>" }, { "+", "-" }, { "*", "/", "%" },
        };
        if (p_level == int(levels.size()))
        {
            return unary();
        }
        long long left = binary(p_level + 1);
        while (m_at < m_tokens.size())
        {
            auto const& operators = levels[std::size_t(p_level)];
            auto op = std::find(operators.begin(), operators.end(), m_tokens[m_at]);
            if (op == operators.end())
            {
                break;
            }
            ++m_at;
            const long long right = binary(p_level + 1);
            const std::string_view o = *op;
            if (((o == "/") || (o == "%")) && (right == 0))
            {
                m_ok = false;
                return 0;
            }
            left = (o == "||") ? (left || right) : (o == "&&") ? (left && right) : (o == "|") ? (left | right)
                 : (o == "^") ? (left ^ right) : (o == "&") ? (left & right) : (o == "==") ? (left == right)
                 : (o == "!=") ? (left != right) : (o == "<") ? (left < right) : (o == ">") ? (left > right)
                 : (o == "<=") ? (left <= right) : (o == ">=") ? (left >= right) : (o == "<<") ? (left << right)
                 : (o == ">>") ? (left >> right) : (o == "+") ? (left + right) : (o == "-") ? (left - right)
                 : (o == "*") ? (left * right) : (o == "/") ? (left / right) : (left % right);
        }
        return left;
    }

    long long unary()
    {
        if (accept("!"))
        {
            return !unary();
        }
        if (accept("-"))
        {
            return -unary();
        }
        if (accept("+"))
        {
            return unary();
        }
        if (accept("~"))
        {
            return ~unary();
        }
        if (accept("("))
        {
            const long long value = ternary();
            m_ok = m_ok && accept(")");
            return value;
        }
        if ((m_at < m_tokens.size()) && (std::isdigit(static_cast<unsigned char>(m_tokens[m_at][0])) != 0))
        {
            return std::stoll(m_tokens[m_at++], nullptr, 0);
        }
        m_ok = false;
        return 0;
    }

    std::vector<std::string> m_tokens;
    std::size_t m_at = 0u;
    bool m_ok = true;
};

//! \brief One #if and the branches already passed.
struct Branch
{
    bool parent_active = true; //!< Lines of the whole #if...#endif may be kept.
    bool taken = false;        //!< A branch has been kept already.
    bool active = true;        //!< The lines of the current branch are kept.
};

} // namespace

std::string keepActiveLines(std::string_view p_source, std::vector<std::string> const& p_defined, std::string& p_error)
{
    const std::string code = blankComments(p_source);
    std::string out(p_source);
    Macros macros;
    for (std::string const& name : p_defined)
    {
        macros.values[name] = "1";
    }
    std::vector<Branch> stack;
    const auto active = [&]() { return stack.empty() || stack.back().active; };
    const auto blank = [&](std::size_t p_begin, std::size_t p_end)
    {
        for (std::size_t i = p_begin; i < p_end; ++i)
        {
            out[i] = (out[i] == '\n') ? '\n' : ' ';
        }
    };
    const auto test = [&](std::string_view p_condition, bool& p_value)
    {
        std::vector<std::string> tokens;
        long long value = 0;
        if (!expand(tokensOf(p_condition), macros, tokens, 0) || !Condition(std::move(tokens)).evaluate(value))
        {
            p_error = "cannot read the condition '#if " + std::string(trimmed(p_condition)) + "'";
            return false;
        }
        p_value = (value != 0);
        return true;
    };

    std::size_t begin = 0u;
    while (begin < code.size())
    {
        // One logical line: a directive may continue on the next lines.
        std::size_t end = begin;
        std::string line;
        while (true)
        {
            std::size_t stop = code.find('\n', end);
            stop = (stop == std::string::npos) ? code.size() : stop;
            std::string_view piece = std::string_view(code).substr(end, stop - end);
            const bool continued = !trimmed(piece).empty() && (trimmed(piece).back() == '\\');
            line += continued ? std::string(piece.substr(0u, piece.rfind('\\'))) : std::string(piece);
            end = stop;
            if (!continued || (stop >= code.size()))
            {
                break;
            }
            end = stop + 1u;
            line += ' ';
        }

        std::string_view text = trimmed(line);
        const bool directive = !text.empty() && (text.front() == '#');
        std::string_view name;
        std::string_view rest;
        if (directive)
        {
            text = trimmed(text.substr(1u));
            name = wordAt(text, 0u);
            rest = trimmed(text.substr(name.size()));
        }

        if (directive && ((name == "if") || (name == "ifdef") || (name == "ifndef")))
        {
            bool value = false;
            if (name == "if")
            {
                if (active() && !test(rest, value))
                {
                    return {};
                }
            }
            else
            {
                const std::string_view macro = wordAt(rest, 0u);
                const bool is = macros.values.contains(macro) || macros.functions.contains(macro);
                value = (name == "ifdef") ? is : !is;
            }
            const bool parent = active();
            stack.push_back(Branch{ parent, parent && value, parent && value });
            blank(begin, end);
        }
        else if (directive && ((name == "elif") || (name == "else")))
        {
            if (stack.empty())
            {
                p_error = "#" + std::string(name) + " without #if";
                return {};
            }
            Branch& branch = stack.back();
            bool value = (name == "else");
            if ((name == "elif") && branch.parent_active && !branch.taken && !test(rest, value))
            {
                return {};
            }
            branch.active = branch.parent_active && !branch.taken && value;
            branch.taken = branch.taken || branch.active;
            blank(begin, end);
        }
        else if (directive && (name == "endif"))
        {
            if (stack.empty())
            {
                p_error = "#endif without #if";
                return {};
            }
            stack.pop_back();
            blank(begin, end);
        }
        else if (!active())
        {
            blank(begin, end);
        }
        else if (directive && (name == "define"))
        {
            const std::string_view macro = wordAt(rest, 0u);
            const std::string_view after = rest.substr(macro.size());
            if (!after.empty() && (after.front() == '('))
            {
                macros.functions.insert(std::string(macro));
            }
            else
            {
                macros.values[std::string(macro)] = std::string(trimmed(after));
            }
        }
        else if (directive && (name == "undef"))
        {
            const std::string_view macro = wordAt(rest, 0u);
            macros.values.erase(std::string(macro));
            macros.functions.erase(std::string(macro));
        }
        begin = end + 1u;
    }
    if (!stack.empty())
    {
        p_error = "an #if is not closed by #endif";
        return {};
    }
    return out;
}

} // namespace compages::gpu::detail
