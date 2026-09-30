// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Seventeen hundred cubes in five colours: one mesh, five looks.
//!
//! A copy shares the mesh and the look of what it copies, so the whole field
//! costs one mesh and five sets of shader parameters, however many cubes:
//! \code
//! compages::world::Entity sky = m_scene.box("sky",
//! compages::renderer::color(0.35f, 0.55f, 0.85f));
//! m_scene.copy(sky).position(x, y, z);
//! \endcode
//!
//! Two things happen behind draw(). The cubes are sorted by look, so that
//! the shader parameters change five times a frame rather than at every
//! cube; and the cubes outside the view are not drawn at all, which the
//! draw-call counter of the overlay shows as the camera turns.
// ****************************************************************************
class ManyCubes final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "20b_ManyCubes";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
