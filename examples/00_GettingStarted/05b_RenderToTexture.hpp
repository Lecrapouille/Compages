// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/ColoredCube.hpp"
#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A picture drawn into textures, then read as a picture.
//!
//! The window is one target. A framebuffer is another: colour and depth
//! textures wired together so that a pass writes into them instead of onto the
//! screen. The cube of 05 is drawn only there. The window never sees it as a
//! mesh; it sees the texture that pass produced, once as it is and once through
//! a fullscreen effect.
//!
//! \code
//! {
//!     compages::gpu::RenderPass offscreen(m_target);   // over the window pass
//!     m_cube.draw();
//! }                                          // back to the window
//! m_screen.draw(3u);                         // samples m_color
//! \endcode
//!
//! The textures are named by the framebuffer, not owned: destroying it does
//! not destroy the images, which is why the screen can still sample them. And
//! the offscreen picture is a size of its own, not the size of the window.
// ****************************************************************************
class RenderToTexture: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "05b_RenderToTexture";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief The corner of the cube of Common/ColoredCube.hpp.
    using Vertex = CubeVertex;

    [[nodiscard]] compages::Status makeCube();
    [[nodiscard]] compages::Status makeTarget();

    //! \brief Declared before the drawables sampling them, so destroyed after.
    compages::gpu::Texture m_color;
    compages::gpu::Texture m_depth;
    compages::gpu::Framebuffer m_target;
    compages::gpu::Drawable m_cube;
    //! \brief Two shaders making a triangle over the screen from gl_VertexID,
    //! one copying the picture, one treating it.
    compages::gpu::Drawable m_blit;
    compages::gpu::Drawable m_process;
};

} // namespace examples
