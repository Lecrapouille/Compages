// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace compages::gpu::detail
{

// ----------------------------------------------------------------------------
//! \brief The GLSL a backend older than OpenGL 4.5 compiles.
// ----------------------------------------------------------------------------
enum class GlslDialect
{
    //! \brief `#version 410 core`: OpenGL 4.1, the last one of macOS.
    Desktop410,
    //! \brief `#version 300 es`: OpenGL ES 3.0, which is WebGL 2.
    Es300,
};

// ----------------------------------------------------------------------------
//! \brief A `layout(binding = N)` taken out of a shader, to be applied by the
//! backend after linking.
// ----------------------------------------------------------------------------
struct FixedBinding
{
    //! \brief The sampler, or the name of the uniform block (not its
    //! instance name).
    std::string name;
    //! \brief The number written in the shader.
    int binding = 0;
    //! \brief A uniform block (glUniformBlockBinding) rather than a sampler
    //! (a texture unit).
    bool block = false;
};

// ----------------------------------------------------------------------------
//! \brief A shader rewritten for an older dialect.
// ----------------------------------------------------------------------------
struct TranslatedGlsl
{
    //! \brief The source to give the driver.
    std::string text;
    //! \brief The bindings removed from the text.
    std::vector<FixedBinding> bindings;
    //! \brief The \#version the source asked for, 0 when it had none.
    int source_version = 0;
};

// ----------------------------------------------------------------------------
//! \brief Rewrite a shader written for `#version 450 core` so that an older
//! driver compiles it.
//!
//! Why: every shader of Compages, of its demos and of its users is written
//! once, for GLSL 4.50. The GL41 and GLES3 backends compile the same text,
//! so they translate what has a direct equivalent:
//!
//! - the `#version` line is lowered to the dialect (a lower version is kept
//!   on the desktop);
//! - OpenGL ES gets default precisions (`highp` everywhere, the samplers
//!   that have none included);
//! - `binding = N` is removed from `layout(...)` on samplers and uniform
//!   blocks (GLSL 4.20) and returned in TranslatedGlsl::bindings; the
//!   backend applies them after linking, so the program behaves the same;
//! - `COMPAGES_NO_STORAGE` is defined, for a shader to keep a path without
//!   storage blocks, images and compute.
//!
//! The line numbers of the compiler log stay those of the source: a `#line`
//! follows what is added. Comments are never changed.
//!
//! \note What has no equivalent (storage blocks, images, compute,
//! geometry on ES, implicit int to float conversions on ES) is left as it
//! is: the driver refuses it with its own message.
// ----------------------------------------------------------------------------
[[nodiscard]] TranslatedGlsl translateGlsl(std::string_view p_source,
                                           GlslDialect p_dialect);

} // namespace compages::gpu::detail
