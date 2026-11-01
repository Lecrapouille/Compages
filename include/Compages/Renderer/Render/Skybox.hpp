// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/GPU/Drawable.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"

#include <array>
#include <string>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Six pictures around the camera, drawn behind everything.
//!
//! The camera turns inside the sky but never moves away from it. Drawn first
//! and without the depth test, the sky is behind whatever comes after. Used by
//! Scene::skybox().
//!
//! \code
//! compages::renderer::Skybox sky;
//! COMPAGES_TRY(sky.load({ "px.jpg", "nx.jpg", "py.jpg", "ny.jpg", "pz.jpg", "nz.jpg" }));
//! sky.draw(camera); // inside a render pass, before the scene
//! \endcode
// ****************************************************************************
class Skybox
{
public:

    //! \brief Load the six faces: +X, -X, +Y, -Y, +Z, -Z.
    [[nodiscard]] Status load(std::array<std::string, 6u> const& p_faces);

    //! \brief Was a sky loaded?
    [[nodiscard]] bool valid() const
    {
        return m_texture.valid();
    }

    //! \brief Draw the sky as seen by the camera. Nothing without a sky.
    void draw(CameraFrame const& p_camera);

private:

    compages::gpu::Texture m_texture;
    compages::gpu::Drawable m_cube;
};

} // namespace compages::renderer
