// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Render/RenderSnapshot.hpp"

#include <array>

namespace compages::renderer
{

class AssetManager;
struct Material;

// ****************************************************************************
//! \brief The shadow pictures of a frame: one for the sun, an atlas of six
//! faces per lamp. Part of Renderer; you do not need it to draw a scene.
//!
//! render() draws the depth of the casters as seen by each light, before the
//! scene; bind() gives a lit material the pictures and the matrices to look
//! them up.
//!
//! \code
//! COMPAGES_TRY(m_shadows.render(snapshot, assets));
//! m_shadows.bind(material); // for each lit material
//! \endcode
// ****************************************************************************
class ShadowPass
{
public:

    //! \brief Draw the shadow pictures the environment and the lamps ask for.
    [[nodiscard]] Status render(RenderSnapshot const& p_snapshot,
                                AssetManager& p_assets);

    //! \brief Set the shadow uniforms of a lit material and bind its pictures.
    void bind(Material& p_material) const;

private:

    //! \brief The shadow of the sun.
    struct Sun
    {
        bool enabled = false;
        compages::core::Matrix44f light_view_projection{ compages::core::matrix::Identity };
        float texel = 0.0f;
        float bias = 0.0f;
    };

    //! \brief The shadows of the lamps: which ones cast, and the view of each
    //! face.
    struct Lamps
    {
        bool any = false;
        compages::core::Vector4f enabled{ 0.0f, 0.0f, 0.0f, 0.0f };
        std::array<compages::core::Matrix44f, 24u> faces{};
    };

    [[nodiscard]] Status renderSun(RenderSnapshot const& p_snapshot,
                                   AssetManager& p_assets);
    [[nodiscard]] Status renderLamps(RenderSnapshot const& p_snapshot,
                                     AssetManager& p_assets);

    Sun m_sun;
    compages::gpu::Texture m_sun_depth;
    compages::gpu::Framebuffer m_sun_target;
    compages::gpu::Program m_sun_program;
    compages::gpu::Pipeline m_sun_pipeline;
    Lamps m_lamps;
    compages::gpu::Texture m_lamp_depth;
    compages::gpu::Framebuffer m_lamp_target;
    compages::gpu::Program m_lamp_program;
    compages::gpu::Pipeline m_lamp_pipeline;
};

} // namespace compages::renderer
