// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/FineTiles.hpp"
#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples::archipelago
{

// ****************************************************************************
//! \brief The region under the mouse, draped on the ground: a veil over its
//! cells (their exact Voronoi polygons) and a band along its outline, a few
//! pixels wide from where the camera is.
//!
//! The veil follows the ground as the screen shows it (FineTiles::groundAt),
//! on a grid aligned with the ground: squares of 16 m under the fine tiles,
//! of 31 m (the coarse quads) elsewhere, each cut by the outline of its
//! cell. A coarser veil would cut straight between its corners, and the
//! bumps of the ground in between would show through it.
// ****************************************************************************
class Highlight
{
public:

    //! \brief The veil and the band, hidden. Once.
    void create(compages::renderer::Scene& p_scene);

    //! \brief Drape a region (-1: none), seen from \p p_distance. The veil
    //! is made again only when the region changes, the camera moves much, or
    //! fine tiles are shown or hidden under it; the band when the distance
    //! changes.
    //! \param[in] p_base The coarse patch of the whole map.
    void show(compages::renderer::Scene& p_scene,
              Land const& p_land,
              Patch const& p_base,
              FineTiles const& p_fine,
              int p_region,
              float p_distance);

    //! \brief The land changed: the next show() makes the meshes again.
    void reset()
    {
        m_region = -2;
    }

private:

    compages::world::Entity m_veil;
    compages::world::Entity m_band;
    int m_region = -2;
    //! \brief The width of the band and the lift of the veil they were made
    //! with.
    float m_width = 0.0f;
    float m_lift = 0.0f;
    //! \brief The box of the region, the tiles of the map it is on, and
    //! those of them shown fine when the veil was made.
    compages::Vector2f m_low{ 0.0f, 0.0f };
    compages::Vector2f m_high{ 0.0f, 0.0f };
    std::uint64_t m_tiles = 0u;
    std::uint64_t m_under = 0u;
};

} // namespace examples::archipelago
