// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/GPU/ComputeProgram.hpp"
#include "Compages/GPU/Drawable.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Render/RenderSnapshot.hpp"

#include <chrono>

namespace compages::renderer
{

class LightClusters;
class ShadowPass;

// ****************************************************************************
//! \brief Volumetric fog in froxels (B. Wronski, "Volumetric Fog: Unified
//! compute shader based solution to atmospheric scattering", SIGGRAPH 2014).
//!
//! A 3D picture follows the camera: WIDTH x HEIGHT tiles of the picture
//! times SLICES slices of the depth, thinner near the camera (a froxel is a
//! voxel of the frustum). Two compute shaders fill it at each render:
//! 1. inject: in each froxel, how thick the fog is (Environment::volumetric:
//!    a density thinning with height, clumps drifting with the wind), and
//!    the light it sends toward the eye: the sun and the four nearest lamps
//!    with their shadows (shafts, shadows in the air), the clustered lamps,
//!    the ambient light, each weighted by the phase function of
//!    Henyey-Greenstein;
//! 2. integrate: from the camera outward, slice after slice, the light
//!    gathered and what is let through (S. Hillaire, "Physically Based and
//!    Unified Volumetric Rendering in Frostbite", SIGGRAPH 2015: integrated
//!    over each slice, so it does not depend on its thickness).
//! applyFog() of the lit shaders then reads the picture at the pixel and the
//! depth of each surface: colour x transmittance + light gathered.
// ****************************************************************************
class FogVolume: private compages::NotCopiable
{
public:

    static constexpr std::uint32_t WIDTH = 128u;
    static constexpr std::uint32_t HEIGHT = 72u;
    static constexpr std::uint32_t SLICES = 64u;

    FogVolume() = default;

    // ------------------------------------------------------------------------
    //! \brief Fill the froxels for the camera and the lights of a snapshot.
    //! Does nothing when Environment::volumetric is off.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status update(RenderSnapshot const& p_snapshot, ShadowPass const& p_shadows,
                                LightClusters const& p_clusters);

    // ------------------------------------------------------------------------
    //! \brief Give a lit program the fog (texture unit 10), or tell it there
    //! is none.
    // ------------------------------------------------------------------------
    void bind(compages::gpu::Program& p_program) const;

    // ------------------------------------------------------------------------
    //! \brief Lay the fog of the farthest slice over the background (the
    //! colour of the pass or the sky), before the objects: what has no
    //! surface is seen through the fog too.
    // ------------------------------------------------------------------------
    void drawBackground(RenderSnapshot const& p_snapshot);

private:

    [[nodiscard]] Status create();

    bool m_on = false;
    compages::gpu::ComputeProgram m_inject;
    compages::gpu::ComputeProgram m_integrate;
    compages::gpu::Texture m_scattering;
    compages::gpu::Texture m_integrated;
    compages::gpu::Drawable m_background;
    compages::Vector2f m_depth{ 0.1f, 60.0f };
    compages::Vector4f m_viewport{ 0.0f, 0.0f, 1.0f, 1.0f };
    compages::Matrix44f m_view{ compages::matrix::Identity };
    std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
};

} // namespace compages::renderer
