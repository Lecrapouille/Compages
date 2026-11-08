// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Core/Result.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <array>
#include <vector>

namespace examples::archipelago
{

//! \brief The coarse map: one patch of 512 quads (31 m) and 1024 pixels (16
//! m), cut into 8 x 8 tiles of 2 km.
inline constexpr int TILES = 8;
inline constexpr int BASE_CELLS = 512;
inline constexpr int BASE_PIXELS = 1024;
inline constexpr float TILE = MAP / float(TILES);

// ****************************************************************************
//! \brief The ground and the sea as the GPU draws them.
//!
//! - The map: the PBR shader of the Renderer with another main() (its lights,
//!   its shadow, its fog and its tone curve are kept), on 8 x 8 coarse tiles
//!   of one picture whose alpha tells where the water is. Near the camera,
//!   pictures of grass, sand and rock give the land its grain; waves in the
//!   normals, the sky in the water (Fresnel), the glint, the surf.
//! - The ocean around the map, out to 150 km.
//! - The surface of the sea near the camera: a grid whose corners move with
//!   Gerstner waves (GPU Gems 1, chapter 1), see-through over the sea bed.
// ****************************************************************************
class Sea
{
public:

    //! \brief The materials and the pictures of the ground. Once.
    [[nodiscard]] compages::Status create(compages::renderer::Scene& p_scene);

    //! \brief Show the coarse patch of a new map: its tiles (made the first
    //! time, with the ocean and the surface), its picture.
    [[nodiscard]] compages::Status show(compages::renderer::Scene& p_scene,
                                        Patch const& p_base);

    //! \brief Each frame: the surface under the camera (sized to what it
    //! sees, hidden from afar), the clock of the waves, the pictures of the
    //! ground on their units.
    void update(compages::renderer::Scene& p_scene,
                compages::world::RigController const& p_rig,
                float p_time);

    //! \brief The 8 x 8 coarse tiles, row after row along +Z.
    [[nodiscard]] std::vector<compages::world::Entity>& tiles()
    {
        return m_tiles;
    }

    //! \brief The material of the map, for the fine tiles.
    [[nodiscard]] compages::renderer::MaterialId map() const
    {
        return m_map;
    }

private:

    [[nodiscard]] compages::Status
    makeMaterials(compages::renderer::Scene& p_scene);
    [[nodiscard]] compages::Status
    makeOceanAndSurface(compages::renderer::Scene& p_scene);

    compages::renderer::MaterialId m_map{};
    compages::renderer::MaterialId m_surface_material{};
    compages::renderer::TextureAssetId m_picture{};
    //! \brief The pictures of the ground: grass, sand, rock.
    std::array<compages::gpu::Texture, 3> m_grounds;
    std::vector<compages::world::Entity> m_tiles;
    compages::world::Entity m_ocean;
    compages::world::Entity m_surface;
    //! \brief Where the surface of the sea is, and its half width (negative:
    //! hidden).
    compages::Vector2f m_surface_centre{ 0.0f, 0.0f };
    float m_surface_radius = -1.0f;
};

} // namespace examples::archipelago
