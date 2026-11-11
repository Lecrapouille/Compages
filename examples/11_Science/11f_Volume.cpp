// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "11_Science/11f_Volume.hpp"

#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <cmath>

namespace examples
{

//! \brief A room 3 m wide, 2 m high: 48 x 32 x 48 points.
static constexpr int ACROSS = 48;
static constexpr int HIGH = 32;
static constexpr float SPACING = 3.0f / float(ACROSS - 1);

// ----------------------------------------------------------------------------
std::string Volume::description() const
{
    return "A volume, seen through and cut. The temperature of a room heated "
           "by three moving heaters is a Field<float, Grid3D>. scene.show() "
           "draws it as a see-through box: along each ray of the picture, the "
           "colours of the warm air add up (Inferno). compages::slice() takes "
           "one horizontal layer as a 2D field, shown by scene.show() with "
           "its own colours (Viridis). The values go to the GPU as a 3D "
           "texture (gpu::Texture::fromField): same order of points and "
           "texels. Try it: the height of the cut, the range, the heaters.";
}

// ----------------------------------------------------------------------------
//! Each heater warms the air around it, more above it (warm air rises).
// ----------------------------------------------------------------------------
void Volume::warm(float p_time)
{
    compages::Grid3D const& grid = m_heat.grid();
    const compages::Vector3f heaters[3] = {
        { 0.8f * std::cos(0.5f * p_time), 0.3f, 0.8f * std::sin(0.5f * p_time) },
        { -0.9f, 0.4f + 0.2f * std::sin(0.8f * p_time), 0.6f * std::cos(0.3f * p_time) },
        { 0.4f * std::sin(0.4f * p_time), 0.2f, -0.9f },
    };
    for (std::size_t k = 0u; k < m_heat.size(); ++k)
    {
        const compages::Vector3f p = grid.point(k);
        float t = 20.0f;
        for (compages::Vector3f const& h : heaters)
        {
            const float dx = p.x - h.x;
            const float dz = p.z - h.z;
            // Above the heater the plume is wider and goes higher.
            const float up = std::max(p.y - h.y, 0.0f);
            const float r2 = (dx * dx) + (dz * dz);
            const float width = 0.08f + (0.12f * up);
            const float below = std::min(p.y - h.y, 0.0f);
            t += 30.0f * std::exp(-(r2 / width) - (0.6f * up) - (20.0f * below * below));
        }
        m_heat[k] = t;
    }
}

// ----------------------------------------------------------------------------
compages::Status Volume::setUp()
{
    const compages::Grid3D grid{ .columns = ACROSS,
                                 .rows = HIGH,
                                 .layers = ACROSS,
                                 .origin = { -1.5f, 0.0f, -1.5f },
                                 .spacing = SPACING };
    m_heat = compages::Field<float, compages::Grid3D>(grid, 20.0f);
    warm(0.0f);

    m_scene.background(0.08f, 0.09f, 0.11f);
    m_scene.camera().position(3.2f, 2.6f, 3.6f).add<compages::world::Orbit>(
        compages::Vector3f(0.0f, 0.8f, 0.0f));
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.0f);
    m_room = m_scene.show(m_heat, m_look, "Room");
    m_cut = m_scene.show(compages::slice(m_heat, compages::Axis::Y, m_layer), m_cut_look, "Cut");
    m_cut.position(0.0f, grid.point(0, m_layer, 0).y, 0.0f);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Volume::draw(compages::world::ViewFrame const& p_frame)
{
    if (m_moving)
    {
        m_time += p_frame.elapsed;
        warm(m_time);
    }
    compages::Status shown = m_scene.show(m_room, m_heat, m_look);
    if (shown)
    {
        shown = m_scene.show(m_cut, compages::slice(m_heat, compages::Axis::Y, m_layer), m_cut_look);
    }
    if (!shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_cut.position(0.0f, m_heat.grid().point(0, m_layer, 0).y, 0.0f);
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Volume::controls()
{
    ImGui::SliderInt("Height of the cut", &m_layer, 0, HIGH - 1);
    ImGui::SliderFloat("Air: from (degrees)", &m_look.low, 20.0f, 40.0f, "%.1f");
    ImGui::SliderFloat("Air: to (degrees)", &m_look.high, 25.0f, 60.0f, "%.1f");
    ImGui::Checkbox("Heaters move", &m_moving);
}

} // namespace examples
