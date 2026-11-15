// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief The colours of the Source panel: C++, and the GLSL written inside
//! its raw strings (R"( … )"), each word in the colour of what it is.
//!
//! The panel shows the code of a demo; it does not edit it. So a colourer is
//! enough, and no editor widget is needed: the lines are cut into spans once,
//! when the file is read, and each frame draws the visible lines span by
//! span. The cut is lexical (keywords, types, strings, numbers, comments,
//! preprocessor), not a parser: a name is a type when it starts with a
//! capital letter, as in the code of Compages, and a constant when it is all
//! capitals.
//!
//! \code
//! std::vector<SourceSpans> colours = colourSource(lines);   // once
//! // each visible line i:
//! drawSourceLine(lines[i], colours[i]);
//! \endcode
// ****************************************************************************

//! \brief What a piece of a line is.
enum class SourceToken : std::uint8_t
{
    Plain,
    Comment,
    Keyword,
    Type,
    Constant,
    Namespace,
    String,
    Number,
    Preprocessor,
    //! \brief GLSL inside a raw string, not otherwise coloured.
    Shader,
};

//! \brief A run of one colour in a line: bytes [begin, end).
struct SourceSpan
{
    std::uint32_t begin;
    std::uint32_t end;
    SourceToken token;
};

//! \brief The spans of one line, in order, covering it all.
using SourceSpans = std::vector<SourceSpan>;

// ----------------------------------------------------------------------------
//! \brief Cut the lines of a C++ file into spans of one colour. Block
//! comments and raw strings may run over several lines.
// ----------------------------------------------------------------------------
[[nodiscard]] std::vector<SourceSpans> colourSource(std::vector<std::string> const& p_lines);

// ----------------------------------------------------------------------------
//! \brief Draw one line with ImGui, span by span, on the current line of the
//! window.
// ----------------------------------------------------------------------------
void drawSourceLine(std::string const& p_line, SourceSpans const& p_spans);

} // namespace examples
