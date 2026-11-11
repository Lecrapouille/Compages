// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Fields/Field.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A model of the CPU bends a mesh on the GPU: a bimetal strip, brass
//! on steel, heated at one end, curls up.
//!
//! The heat runs along the strip (a Field<float> of one row); brass grows
//! more than steel, so each slice bends by a curvature proportional to its
//! heat. The CPU adds the bends up into the middle line of the strip, a
//! storage buffer; the hook "deform" of the PBR shader puts each corner of a
//! straight strip on that line:
//! \code
//! pbr.vertex().hook("deform", BEND);           // reads the buffer "Beam"
//! material.buffers = { { .block = "Beam", .buffer = m_line.handle() } };
//! \endcode
// ****************************************************************************
class BimetalBeam final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06p_BimetalBeam";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief Heat for p_seconds, then the middle line from the heat.
    void heat(float p_seconds);
    void bend();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    //! \brief The temperature along the strip, in degrees.
    compages::Field<float> m_heat;
    compages::Field<float> m_next;
    //! \brief Per slice: x, y of the middle line, its angle, its temperature.
    std::vector<compages::Vector4f> m_slices;
    compages::gpu::StorageBuffer<compages::Vector4f> m_line;
    compages::world::Entity m_flame;
    float m_flame_temperature = 600.0f;
    bool m_burning = true;
    float m_speed = 20.0f;
};

} // namespace examples
