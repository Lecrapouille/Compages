// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A texture read by a shader, and a strip instead of separate
//! triangles.
//!
//! The texture is computed here rather than read from a file, so that the
//! example depends on nothing outside itself and so that its content is
//! knowable: a checkerboard, with a gradient over it. Reading a file is one
//! call away, and Texture::fromFile() is what 09_HeightMap uses.
//!
//! Uniforms and textures are given by name, with the same syntax as the
//! attributes of 01b: the shader says which is which.
//! \code
//! m_quad["scale"] = 2.0f;        // a uniform, written immediately
//! m_quad["image"] = m_texture;   // a sampler: a texture unit is picked for it
//! \endcode
//! The four corners are drawn as a triangle strip: the primitive belongs to the
//! render state of the drawable, not to each draw.
// ****************************************************************************
class TexturedQuad: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "03a_TexturedQuad";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    // ------------------------------------------------------------------------
    //! \brief Build the checkerboard the example shows.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::Status makeTexture();

    //! \brief Read by m_quad, which keeps a pointer to it: declared first so
    //! that it is destroyed last.
    compages::gpu::Texture m_texture;
    compages::gpu::Drawable m_quad;
};

} // namespace examples
