// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A sweating head with decals, both computed in the texture space of
//! the mesh.
//!
//! The head of three.js webgl_decals (Lee Perry-Smith). Nothing is simulated
//! in 3D: the mesh is drawn into its own UV map (gl_Position = uv) and every
//! texel of a 2D texture becomes a small piece of skin.
//! - Force pass: gravity projected on the skin, written as a speed in texels
//!   per second, plus how much the skin faces down (where drops fall).
//! - Fluid pass (ping-pong): pores fill small beads; a bead heavier than a
//!   threshold runs down the force field and leaves a thin wet film behind.
//!   Water is only moved between texels, never created or lost, except by
//!   pores, evaporation and falling drops.
//! - Decal pass: a box is projected on the mesh where the mouse clicks, and
//!   the paint is baked into a texture of the same UV map.
//! - The skin material is the PBR shader of the Renderer with another
//!   main(): it adds the normal map, the decals and the water.
// ****************************************************************************
class SweatAndDecals final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "54_SweatAndDecals";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief Where a ray meets the head, in world space and in UV space.
    struct Hit
    {
        compages::core::Vector3f point;
        compages::core::Vector3f normal;
        compages::core::Vector2f uv;
    };

    compages::Status makeSkinMaterial(compages::world::Entity p_mesh);
    compages::Status makeTargets();
    [[nodiscard]] std::optional<Hit>
    raycast(compages::core::Vector2f p_pixel) const;
    void computeForces();
    void simulate(float p_dt);
    void stampDecal(Hit const& p_hit);
    void clear(compages::gpu::Framebuffer const& p_target);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_root;
    compages::world::Entity m_head;

    //! \brief A copy of the head mesh, for the UV passes and the raycast.
    std::vector<compages::renderer::MeshVertex> m_vertices;
    std::vector<std::uint32_t> m_indices;
    compages::core::AABB m_bounds;

    //! \brief The skin material, owned by the AssetManager.
    compages::renderer::MaterialId m_skin;
    compages::gpu::Texture m_normal_map;
    compages::gpu::Texture m_decal_image;

    //! \brief Force field: rg = speed in texels/s, b = faces down, a = sweat
    //! zone.
    compages::gpu::Texture m_forces;
    compages::gpu::Framebuffer m_forces_target;
    //! \brief Water: r = amount, g = wetness, b = running.
    compages::gpu::Texture m_fluid[2];
    compages::gpu::Framebuffer m_fluid_target[2];
    int m_current = 0;
    //! \brief Painted decals, premultiplied by nothing: rgb + coverage.
    compages::gpu::Texture m_decals;
    compages::gpu::Framebuffer m_decals_target;

    compages::gpu::Drawable m_force_pass;
    compages::gpu::Drawable m_fluid_pass;
    compages::gpu::Drawable m_decal_pass;

    std::mt19937 m_random{ 54u };
    compages::core::Vector4f m_drop{ 0.0f, 0.0f, 0.0f, 0.0f };

    // Try it panel.
    int m_click = 0; // 0 = decal, 1 = drop of water
    bool m_sweat = true;
    bool m_show_water = false;
    float m_pore_rate = 0.2f;
    float m_flow_speed = 0.06f;
    float m_evaporation = 0.03f;
    float m_pitch = 0.0f;
    float m_roll = 0.0f;
    bool m_warm = false;
    bool m_first_decals = false;
    bool m_wipe = false;
    bool m_peel = false;
};

} // namespace examples
