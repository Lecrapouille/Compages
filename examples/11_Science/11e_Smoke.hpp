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

namespace examples
{

// ****************************************************************************
//! \brief Smoke: a fluid in 2D, every step on the GPU (Stam, "Stable
//! Fluids", 1999), shown by scene.show().
//!
//! Five compute shaders come from one GLSL text, each chosen by a define:
//! \code
//! auto step = compages::gpu::ShaderSource::fromText("smoke.comp", SMOKE)
//!                 .define("STEP", "3");   // the Jacobi step of the pressure
//! COMPAGES_TRY(m_jacobi.load(step));
//! \endcode
// ****************************************************************************
class Smoke final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11e_Smoke";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    [[nodiscard]] compages::Status step(float p_dt);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::Grid2D m_grid;
    //! \brief The kernels: move the speeds, their divergence, a step of the
    //! pressure, remove its gradient, move the smoke.
    compages::gpu::ComputeProgram m_advect_velocity;
    compages::gpu::ComputeProgram m_divergence;
    compages::gpu::ComputeProgram m_jacobi;
    compages::gpu::ComputeProgram m_project;
    compages::gpu::ComputeProgram m_advect_smoke;
    std::array<compages::gpu::StorageBuffer<compages::Vector2f>, 2> m_velocity;
    std::array<compages::gpu::StorageBuffer<float>, 2> m_smoke;
    std::array<compages::gpu::StorageBuffer<float>, 2> m_pressure;
    compages::gpu::StorageBuffer<float> m_divergence_values;
    std::size_t m_current = 0u;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Inferno,
                                          .low = 0.0f,
                                          .high = 1.2f,
                                          .relief = 0.0f,
                                          .lit = false };
    compages::world::Entity m_view;
    float m_time = 0.0f;
    float m_buoyancy = 6.0f;
    float m_wind = 0.0f;
    int m_iterations = 30;
};

} // namespace examples
