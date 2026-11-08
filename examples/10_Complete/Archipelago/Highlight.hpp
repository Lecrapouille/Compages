// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples::archipelago
{

// ****************************************************************************
//! \brief The region under the mouse, draped on the ground: a veil over its
//! cells (their exact Voronoi polygons) and a band along its outline, a few
//! pixels wide from where the camera is.
// ****************************************************************************
class Highlight
{
public:

    //! \brief The veil and the band, hidden. Once.
    void create(compages::renderer::Scene& p_scene);

    //! \brief Drape a region (-1: none), seen from \p p_distance. Made again
    //! only when the region changes or the camera moves much.
    void show(compages::renderer::Scene& p_scene,
              Land const& p_land,
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
    float m_width = 0.0f;
};

} // namespace examples::archipelago
