// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Common/SourceView.hpp"

#include "Common/Gui.hpp"

#include <cctype>
#include <string_view>
#include <unordered_set>

namespace examples
{

namespace
{

const std::unordered_set<std::string_view> CPP_KEYWORDS = {
    "alignas", "alignof", "auto", "break", "case", "catch", "class", "const", "consteval", "constexpr",
    "constinit", "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do",
    "else", "enum", "explicit", "export", "extern", "false", "final", "for", "friend", "goto", "if",
    "inline", "mutable", "namespace", "new", "noexcept", "nullptr", "operator", "override", "private",
    "protected", "public", "requires", "return", "sizeof", "static", "static_assert", "static_cast",
    "dynamic_cast", "reinterpret_cast", "const_cast", "struct", "switch", "template", "this", "throw",
    "true", "try", "typedef", "typename", "union", "using", "virtual", "volatile", "while", "concept",
};

const std::unordered_set<std::string_view> CPP_TYPES = {
    "bool", "char", "char8_t", "char16_t", "char32_t", "double", "float", "int", "long", "short",
    "signed", "unsigned", "void", "wchar_t", "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t", "int16_t", "int32_t", "int64_t", "string", "string_view", "vector", "array", "span",
    "optional", "map", "unordered_map", "unique_ptr", "shared_ptr", "function", "pair",
};

const std::unordered_set<std::string_view> GLSL_KEYWORDS = {
    "attribute", "break", "buffer", "centroid", "coherent", "const", "continue", "discard", "do", "else",
    "false", "flat", "for", "highp", "if", "in", "inout", "invariant", "layout", "lowp", "mediump",
    "noperspective", "out", "precision", "readonly", "restrict", "return", "shared", "smooth", "struct",
    "switch", "case", "default", "true", "uniform", "varying", "volatile", "while", "writeonly",
};

const std::unordered_set<std::string_view> GLSL_TYPES = {
    "bool", "int", "uint", "float", "double", "void", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4",
    "uvec2", "uvec3", "uvec4", "bvec2", "bvec3", "bvec4", "dvec2", "dvec3", "dvec4", "mat2", "mat3",
    "mat4", "mat2x2", "mat3x3", "mat4x4", "sampler2D", "sampler3D", "samplerCube", "sampler2DArray",
    "sampler2DShadow", "isampler2D", "usampler2D", "image2D", "image3D",
};

// ----------------------------------------------------------------------------
//! The state a line starts in: inside a block comment, or inside a raw string
//! whose closing is ")" + delimiter + '"'.
// ----------------------------------------------------------------------------
struct Carry
{
    bool in_comment = false;
    bool in_raw = false;
    std::string raw_close;
};

bool isIdentifierStart(char p_c)
{
    return (std::isalpha(static_cast<unsigned char>(p_c)) != 0) || (p_c == '_');
}

bool isIdentifierChar(char p_c)
{
    return (std::isalnum(static_cast<unsigned char>(p_c)) != 0) || (p_c == '_');
}

bool allCapitals(std::string_view p_word)
{
    bool letter = false;
    for (char c : p_word)
    {
        if (std::islower(static_cast<unsigned char>(c)) != 0)
        {
            return false;
        }
        letter = letter || (std::isupper(static_cast<unsigned char>(c)) != 0);
    }
    return letter && (p_word.size() > 1u);
}

// ----------------------------------------------------------------------------
//! Adds a span, merged with the last one when it has the same colour.
// ----------------------------------------------------------------------------
void push(SourceSpans& p_spans, std::size_t p_begin, std::size_t p_end, SourceToken p_token)
{
    if (p_end <= p_begin)
    {
        return;
    }
    if (!p_spans.empty() && (p_spans.back().token == p_token) && (p_spans.back().end == p_begin))
    {
        p_spans.back().end = std::uint32_t(p_end);
        return;
    }
    p_spans.push_back({ std::uint32_t(p_begin), std::uint32_t(p_end), p_token });
}

// ----------------------------------------------------------------------------
//! The colour of a word, in C++ or in GLSL. In C++, a word followed by "::"
//! is a namespace or a class used as one.
// ----------------------------------------------------------------------------
SourceToken wordToken(std::string_view p_word, std::string const& p_line, std::size_t p_end, bool p_glsl)
{
    if (p_glsl)
    {
        if (GLSL_KEYWORDS.contains(p_word))
        {
            return SourceToken::Keyword;
        }
        if (GLSL_TYPES.contains(p_word))
        {
            return SourceToken::Type;
        }
        if (allCapitals(p_word))
        {
            return SourceToken::Constant;
        }
        return SourceToken::Shader;
    }
    if (CPP_KEYWORDS.contains(p_word))
    {
        return SourceToken::Keyword;
    }
    if (CPP_TYPES.contains(p_word))
    {
        return SourceToken::Type;
    }
    if (p_line.compare(p_end, 2u, "::") == 0)
    {
        return std::isupper(static_cast<unsigned char>(p_word.front())) != 0 ? SourceToken::Type
                                                                             : SourceToken::Namespace;
    }
    if (allCapitals(p_word))
    {
        return SourceToken::Constant;
    }
    if (std::isupper(static_cast<unsigned char>(p_word.front())) != 0)
    {
        return SourceToken::Type;
    }
    return SourceToken::Plain;
}

// ----------------------------------------------------------------------------
//! Colours the code between p_from and p_to of a line, C++ or GLSL. Returns
//! where it stopped: p_to, or the start of a raw string or a block comment
//! that the caller handles (p_carry tells which).
// ----------------------------------------------------------------------------
std::size_t colourCode(std::string const& p_line, std::size_t p_from, std::size_t p_to, bool p_glsl,
                       SourceSpans& p_spans, Carry& p_carry)
{
    const SourceToken plain = p_glsl ? SourceToken::Shader : SourceToken::Plain;
    std::size_t i = p_from;

    // A preprocessor line: # first (C++ or GLSL), to the end.
    const std::size_t first = p_line.find_first_not_of(" \t", p_from);
    if ((first != std::string::npos) && (first < p_to) && (p_line[first] == '#'))
    {
        push(p_spans, p_from, first, plain);
        push(p_spans, first, p_to, SourceToken::Preprocessor);
        return p_to;
    }

    while (i < p_to)
    {
        const char c = p_line[i];
        if ((c == '/') && (i + 1u < p_to) && (p_line[i + 1u] == '/'))
        {
            push(p_spans, i, p_to, SourceToken::Comment);
            return p_to;
        }
        if ((c == '/') && (i + 1u < p_to) && (p_line[i + 1u] == '*'))
        {
            p_carry.in_comment = true;
            return i;
        }
        if (!p_glsl && (c == 'R') && (p_line.compare(i, 2u, "R\"") == 0))
        {
            const std::size_t open = p_line.find('(', i + 2u);
            if (open != std::string::npos)
            {
                p_carry.in_raw = true;
                p_carry.raw_close = ")" + p_line.substr(i + 2u, open - (i + 2u)) + "\"";
                push(p_spans, i, open + 1u, SourceToken::String);
                return open + 1u;
            }
        }
        if ((c == '"') || ((c == '\'') && !p_glsl))
        {
            std::size_t j = i + 1u;
            while ((j < p_to) && (p_line[j] != c))
            {
                j += (p_line[j] == '\\') ? 2u : 1u;
            }
            j = std::min(j + 1u, p_to);
            push(p_spans, i, j, SourceToken::String);
            i = j;
            continue;
        }
        if ((std::isdigit(static_cast<unsigned char>(c)) != 0) ||
            ((c == '.') && (i + 1u < p_to) && (std::isdigit(static_cast<unsigned char>(p_line[i + 1u])) != 0)))
        {
            std::size_t j = i + 1u;
            while ((j < p_to) && (isIdentifierChar(p_line[j]) || (p_line[j] == '.') || (p_line[j] == '\'') ||
                                  (((p_line[j] == '-') || (p_line[j] == '+')) &&
                                   ((p_line[j - 1u] == 'e') || (p_line[j - 1u] == 'E')))))
            {
                ++j;
            }
            push(p_spans, i, j, SourceToken::Number);
            i = j;
            continue;
        }
        if (isIdentifierStart(c))
        {
            std::size_t j = i + 1u;
            while ((j < p_to) && isIdentifierChar(p_line[j]))
            {
                ++j;
            }
            const std::string_view word(p_line.data() + i, j - i);
            push(p_spans, i, j, wordToken(word, p_line, j, p_glsl));
            i = j;
            continue;
        }
        push(p_spans, i, i + 1u, plain);
        ++i;
    }
    return p_to;
}

// ----------------------------------------------------------------------------
//! The palette, for the dark theme of the gallery.
// ----------------------------------------------------------------------------
ImVec4 colourOf(SourceToken p_token)
{
    switch (p_token)
    {
        case SourceToken::Comment:
            return { 0.50f, 0.72f, 0.50f, 1.0f };
        case SourceToken::Keyword:
            return { 0.78f, 0.57f, 0.92f, 1.0f };
        case SourceToken::Type:
            return { 0.40f, 0.80f, 0.85f, 1.0f };
        case SourceToken::Constant:
            return { 0.92f, 0.80f, 0.50f, 1.0f };
        case SourceToken::Namespace:
            return { 0.55f, 0.62f, 0.72f, 1.0f };
        case SourceToken::String:
            return { 0.90f, 0.62f, 0.45f, 1.0f };
        case SourceToken::Number:
            return { 0.70f, 0.85f, 0.55f, 1.0f };
        case SourceToken::Preprocessor:
            return { 0.86f, 0.55f, 0.70f, 1.0f };
        case SourceToken::Shader:
            return { 0.92f, 0.86f, 0.74f, 1.0f };
        case SourceToken::Plain:
            break;
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

} // namespace

// ----------------------------------------------------------------------------
std::vector<SourceSpans> colourSource(std::vector<std::string> const& p_lines)
{
    std::vector<SourceSpans> result;
    result.reserve(p_lines.size());
    Carry carry;
    for (std::string const& line : p_lines)
    {
        SourceSpans spans;
        std::size_t i = 0u;
        const std::size_t size = line.size();
        while (i < size)
        {
            if (carry.in_comment)
            {
                const std::size_t close = line.find("*/", i);
                const std::size_t end = (close == std::string::npos) ? size : close + 2u;
                push(spans, i, end, SourceToken::Comment);
                carry.in_comment = (close == std::string::npos);
                i = end;
                continue;
            }
            if (carry.in_raw)
            {
                // GLSL until the closing of the raw string, which may be on
                // a later line.
                const std::size_t close = line.find(carry.raw_close, i);
                const std::size_t stop = (close == std::string::npos) ? size : close;
                Carry inner;
                std::size_t at = i;
                while (at < stop)
                {
                    at = colourCode(line, at, stop, true, spans, inner);
                    if (inner.in_comment)
                    {
                        const std::size_t end_comment = line.find("*/", at + 2u);
                        const std::size_t end = (end_comment == std::string::npos || end_comment >= stop)
                                                    ? stop
                                                    : end_comment + 2u;
                        push(spans, at, end, SourceToken::Comment);
                        inner.in_comment = false;
                        at = end;
                    }
                }
                if (close == std::string::npos)
                {
                    i = size;
                    continue;
                }
                push(spans, close, close + carry.raw_close.size(), SourceToken::String);
                carry.in_raw = false;
                i = close + carry.raw_close.size();
                continue;
            }
            i = colourCode(line, i, size, false, spans, carry);
            if (carry.in_comment)
            {
                push(spans, i, i + 2u, SourceToken::Comment);
                i += 2u;
            }
        }
        result.push_back(std::move(spans));
    }
    return result;
}

// ----------------------------------------------------------------------------
void drawSourceLine(std::string const& p_line, SourceSpans const& p_spans)
{
    if (p_spans.empty())
    {
        ImGui::TextUnformatted("");
        return;
    }
    bool first = true;
    for (SourceSpan const& span : p_spans)
    {
        if (!first)
        {
            ImGui::SameLine(0.0f, 0.0f);
        }
        first = false;
        ImGui::PushStyleColor(ImGuiCol_Text, colourOf(span.token));
        ImGui::TextUnformatted(p_line.data() + span.begin, p_line.data() + span.end);
        ImGui::PopStyleColor();
    }
}

} // namespace examples
