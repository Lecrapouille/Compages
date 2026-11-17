// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Small tools to rewrite GLSL text, shared by GlslDialect (older
//! dialects) and ComputeEmulation (compute shaders without compute).
//!
//! The rewriters search a copy of the text where comments (and, when asked,
//! preprocessor lines) are replaced by spaces: same length, same positions.
//! A search then never matches inside a comment, and every position found
//! applies to the original text, where the edits are made.
// ****************************************************************************

namespace compages::gpu::detail
{

//! \brief One change of a text, applied once every position is known.
struct Edit
{
    std::size_t at = 0u;
    std::size_t length = 0u;
    std::string text;
};

//! \brief Apply the edits, from the end so that the positions still hold. At
//! the same position, the replacement goes first and the insertion lands in
//! front of it.
[[nodiscard]] std::string applyEdits(std::string_view p_text, std::vector<Edit> p_edits);

[[nodiscard]] bool isWordChar(char p_c);
[[nodiscard]] bool isSpace(char p_c);
[[nodiscard]] std::string_view trimmed(std::string_view p_text);

//! \brief The text with its comments replaced by spaces, newlines kept.
[[nodiscard]] std::string blankComments(std::string_view p_source);

//! \brief The text with its preprocessor lines (and the lines they continue
//! with a backslash) replaced by spaces, newlines kept. Give it a text whose
//! comments are already blank.
[[nodiscard]] std::string blankDirectives(std::string_view p_code);

//! \brief The line number the compiler gives to the character at \c p_at,
//! `#line` directives taken into account.
[[nodiscard]] int lineNumberAt(std::string_view p_code, std::size_t p_at);

//! \brief The position of the bracket closing the one at \c p_open ( `(` or
//! `[` or `{` ), counting the nested ones; npos if it is not closed.
[[nodiscard]] std::size_t matchingBracket(std::string_view p_code, std::size_t p_open);

//! \brief The word starting at \c p_at, empty if none starts there.
[[nodiscard]] std::string_view wordAt(std::string_view p_code, std::size_t p_at);

//! \brief The first position at or after \c p_at that is not a space.
[[nodiscard]] std::size_t skipSpaces(std::string_view p_code, std::size_t p_at);

// ----------------------------------------------------------------------------
//! \brief The text with the lines that `#if`, `#ifdef`, `#ifndef`, `#elif`
//! and `#else` leave out made empty, and these directives with them.
//!
//! Why: a rewriter that must know what the program holds (which storage
//! blocks exist) cannot read a text where one source declares a block for
//! one value of a macro and another for another. Lines are emptied, never
//! removed: the line numbers stay those of the source.
//!
//! Understands `#define NAME value` (no parameters), `#undef`, `defined`,
//! integers and the operators of C. A function-like macro is kept in the text
//! for the real preprocessor but is worth 0 in a condition.
//!
//! \param[in] p_defined names defined before the first line (with value 1).
//! \return the text, or why a condition could not be read.
// ----------------------------------------------------------------------------
[[nodiscard]] std::string keepActiveLines(std::string_view p_source, std::vector<std::string> const& p_defined,
                                          std::string& p_error);

} // namespace compages::gpu::detail
