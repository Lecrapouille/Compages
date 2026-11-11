// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace compages::gpu
{

// ****************************************************************************
//! \brief The GLSL of one stage after its includes are put in: what the driver
//! compiles, and what is needed to read its error log.
// ****************************************************************************
struct ResolvedSource
{
    //! \brief The text to compile: \#version first, then the defines, then the
    //! text with every \#include replaced by what it names.
    std::string text;
    //! \brief The name of each piece, by the number that \#line gives it: 0 is
    //! the main text, then each include in the order it was met.
    std::vector<std::string> pieces;
    //! \brief The files read from the disk, main file included: what a hot
    //! reload should watch.
    std::vector<std::filesystem::path> files;

    // ------------------------------------------------------------------------
    //! \brief Rewrite an error log of the driver so that it names the pieces:
    //! "0:12(5): error" or "0(12) : error" become "water.glsl:12...".
    //! \param[in] p_log The log as the driver wrote it.
    //! \return The same log, with the piece names.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string explain(std::string_view p_log) const;
};

// ****************************************************************************
//! \brief The GLSL of one stage, made of pieces: a main text that includes
//! other texts.
//!
//! A text says \#include "name" or \#include <name>. A name is looked for:
//! first among the texts given with include(), then (quotes only) beside the
//! file that includes it, then in each directory(). Each name is put in once;
//! a name that includes itself, even through others, is an error.
//!
//! \code
//! compages::gpu::ShaderSource water =
//!     compages::gpu::ShaderSource::fromFile("shaders/water.frag");
//! water.directory("shaders/common")
//!     .include("swells.glsl", swellsGlsl()) // GLSL made from C++ data
//!     .define("SWELL_COUNT", "8");
//! COMPAGES_TRY(program.load(vertex, water));
//! \endcode
//!
//! A \b hook is a named place that a shader of a library leaves open, with a
//! default text: \#include <hook/deform>. hook() gives it another text; a hook
//! name that the shader does not declare is an error of resolve(), so that a
//! typo is not silent.
// ****************************************************************************
class ShaderSource
{
public:

    // ------------------------------------------------------------------------
    //! \brief A source whose main text is given.
    //! \param[in] p_name What to call it in error messages.
    //! \param[in] p_text The GLSL, which starts with its \#version.
    // ------------------------------------------------------------------------
    [[nodiscard]] static ShaderSource fromText(std::string p_name,
                                               std::string p_text);

    // ------------------------------------------------------------------------
    //! \brief A source whose main text is a file, read by resolve(). Its
    //! folder is where \#include "name" looks first.
    //! \param[in] p_path The file.
    // ------------------------------------------------------------------------
    [[nodiscard]] static ShaderSource
    fromFile(std::filesystem::path const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Look for included files in this folder too, after the ones given
    //! before.
    //! \param[in] p_folder The folder.
    //! \return This source, to chain calls.
    // ------------------------------------------------------------------------
    ShaderSource& directory(std::filesystem::path p_folder);

    // ------------------------------------------------------------------------
    //! \brief Give the text of an include name, without a file: for GLSL made
    //! from C++ data, or kept in a C++ string. Wins over the files.
    //! \param[in] p_name The name a text gives to \#include.
    //! \param[in] p_text Its GLSL, without \#version.
    //! \return This source, to chain calls.
    // ------------------------------------------------------------------------
    ShaderSource& include(std::string p_name, std::string p_text);

    // ------------------------------------------------------------------------
    //! \brief Leave a hook open, with its default text: for the shaders of a
    //! library. A text puts it with \#include <hook/name>.
    //! \param[in] p_name The hook, without "hook/".
    //! \param[in] p_default Its GLSL until hook() replaces it.
    //! \return This source, to chain calls.
    // ------------------------------------------------------------------------
    ShaderSource& declareHook(std::string p_name, std::string p_default);

    // ------------------------------------------------------------------------
    //! \brief Replace the text of a hook. resolve() fails if no hook of this
    //! name was declared.
    //! \param[in] p_name The hook, without "hook/".
    //! \param[in] p_text Its new GLSL.
    //! \return This source, to chain calls.
    // ------------------------------------------------------------------------
    ShaderSource& hook(std::string p_name, std::string p_text);

    // ------------------------------------------------------------------------
    //! \brief Add "#define name value" just after the \#version.
    //! \param[in] p_name The macro.
    //! \param[in] p_value Its value (may be empty).
    //! \return This source, to chain calls.
    // ------------------------------------------------------------------------
    ShaderSource& define(std::string p_name, std::string p_value);

    // ------------------------------------------------------------------------
    //! \brief Put the includes in.
    //! \return The text to compile and how to read its errors, or what is
    //! missing: a file, an include, a hook, a \#version.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<ResolvedSource> resolve() const;

    // ------------------------------------------------------------------------
    //! \brief What the main text is called in messages.
    //! \return Its name, or its file.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& name() const
    {
        return m_name;
    }

private:

    ShaderSource() = default;

    //! \brief The main text, or empty when it is m_file.
    std::string m_text;
    std::filesystem::path m_file;
    std::string m_name;
    std::vector<std::filesystem::path> m_folders;
    std::map<std::string, std::string, std::less<>> m_includes;
    //! \brief The hooks declared, with their default text.
    std::map<std::string, std::string, std::less<>> m_hooks;
    //! \brief The texts given by hook(), checked by resolve().
    std::map<std::string, std::string, std::less<>> m_replaced;
    std::vector<std::pair<std::string, std::string>> m_defines;
};

} // namespace compages::gpu
