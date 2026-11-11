// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/GPU/ComputeProgram.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <array>
#include <random>

namespace examples
{

// ****************************************************************************
//! \brief A field computed and drawn on the GPU: waves in a tank, never read
//! back by the CPU.
//!
//! A compute shader steps the wave equation on three storage buffers (the
//! surface before, now and next), then:
//! \code
//! m_scene.show(m_surface, m_heights[now], m_look);   // the buffer, as it is
//! \endcode
// ****************************************************************************
class RippleTank final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06n_RippleTank";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief One step of the waves; a drop where p_drop.z is not zero.
    [[nodiscard]] compages::Status step(compages::Vector3f p_drop);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::gpu::ComputeProgram m_waves;
    //! \brief The heights before, now and next, turning: m_now is "now".
    std::array<compages::gpu::StorageBuffer<float>, 3> m_heights;
    std::size_t m_now = 1u;
    compages::Grid2D m_grid;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Coolwarm,
                                          .low = -0.4f,
                                          .high = 0.4f,
                                          .relief = 0.35f,
                                          .lit = true };
    compages::world::Entity m_surface;
    std::mt19937 m_random{ 7u }; // NOSONAR: a demo, not a secret
    float m_rain = 3.0f;
    float m_damping = 0.998f;
    float m_next_drop = 0.0f;
};

} // namespace examples
