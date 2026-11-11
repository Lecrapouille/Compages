// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06o_FieldProbe.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Components/Text.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <cmath>
#include <format>

namespace examples
{

//! \brief An island 2 km wide, 128 x 128 points.
static constexpr int POINTS = 128;
static constexpr float WIDTH = 2000.0f;

// ----------------------------------------------------------------------------
//! \brief The altitude at a point: two hills, a sea floor, some waves.
// ----------------------------------------------------------------------------
static float altitudeAt(compages::Vector2f p_at)
{
    auto hill = [p_at](float p_x, float p_z, float p_radius, float p_height)
    {
        const float d2 = ((p_at.x - p_x) * (p_at.x - p_x)) + ((p_at.y - p_z) * (p_at.y - p_z));
        return p_height * std::exp(-d2 / (2.0f * p_radius * p_radius));
    };
    return -150.0f + hill(-100.0f, 80.0f, 330.0f, 480.0f) + hill(420.0f, -300.0f, 180.0f, 230.0f) +
           (25.0f * std::sin(p_at.x / 90.0f) * std::cos(p_at.y / 110.0f));
}

// ----------------------------------------------------------------------------
std::string FieldProbe::description() const
{
    return "Read a field under the mouse. The altitude of an island is a "
           "Field<float>, shown by scene.show() with the Terrain colours and "
           "a relief. scene.probe(field, look, mouse) follows the ray of the "
           "pixel down to the relief as it is drawn and gives the point and "
           "the value there; a Tooltip writes them. Try it: the relief, the "
           "colours, the sea level of the colours.";
}

// ----------------------------------------------------------------------------
compages::Status FieldProbe::setUp()
{
    const std::filesystem::path font = dataPath("fonts/DejaVuSans.ttf");
    if (font.empty())
    {
        return compages::failure("fonts/DejaVuSans.ttf is missing from external/Compages-data/");
    }
    m_scene.font(font);

    const compages::Grid2D grid{ .columns = POINTS,
                                 .rows = POINTS,
                                 .origin = { -0.5f * WIDTH, -0.5f * WIDTH },
                                 .spacing = WIDTH / float(POINTS - 1) };
    m_altitude = compages::Field<float>(grid, 0.0f);
    for (std::size_t k = 0u; k < grid.count(); ++k)
    {
        m_altitude[k] = altitudeAt(grid.point(k));
    }

    m_scene.background(0.55f, 0.6f, 0.66f);
    // Kilometres away: the camera sees up to 20 km (1 km by default).
    compages::world::Entity camera = m_scene.camera();
    camera.position(0.0f, 1300.0f, 1900.0f).add<compages::world::Orbit>();
    camera.get<compages::world::Camera>().near_plane = 1.0f;
    camera.get<compages::world::Camera>().far_plane = 20000.0f;
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.3f);
    m_island = m_scene.show(m_altitude, m_look, "Island");
    m_probe = m_world.entity("Probe");
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void FieldProbe::draw(compages::world::ViewFrame const& p_frame)
{
    if (const compages::Status shown = m_scene.show(m_island, m_altitude, m_look); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    // The last picture tells where the camera was: probe, then draw. Without
    // the mouse over the view, the centre of the picture.
    compages::renderer::CameraFrame const& camera = m_scene.lastCamera();
    const compages::Vector2f pixel =
        p_frame.input.mouse_over ? p_frame.input.mouse
                                 : compages::Vector2f(0.5f * float(camera.viewport_width),
                                                      0.5f * float(camera.viewport_height));
    const std::optional<compages::renderer::FieldProbe> found =
        m_scene.probe(m_altitude, m_look, pixel);
    if (found)
    {
        m_probe.position(found->point)
            .set(compages::renderer::Tooltip{
                .title = std::format("{:.0f} m", found->value),
                .text = std::format("x {:.0f} m\nz {:.0f} m", found->point.x, found->point.z),
                .color = { 0.95f, 0.95f, 0.95f },
                .accent = { 1.0f, 0.75f, 0.3f },
                .background = { 0.1f, 0.12f, 0.15f },
                .opacity = 0.85f,
                .pixels = 16.0f });
    }
    else if (m_probe.has<compages::renderer::Tooltip>())
    {
        m_probe.remove<compages::renderer::Tooltip>();
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void FieldProbe::controls()
{
    ImGui::SliderFloat("Relief", &m_look.relief, 0.0f, 3.0f, "%.2f");
    int colors = static_cast<int>(m_look.colors);
    if (ImGui::Combo("Colours", &colors, "Viridis\0Inferno\0Coolwarm\0Terrain\0Grey\0"))
    {
        m_look.colors = static_cast<compages::renderer::ColorMap>(colors);
    }
    // Terrain puts the shore in the middle of the range: move the range,
    // the sea rises or falls.
    float sea = 0.5f * (m_look.low + m_look.high);
    if (ImGui::SliderFloat("Sea level of the colours (m)", &sea, -150.0f, 300.0f, "%.0f"))
    {
        m_look.low = sea - 400.0f;
        m_look.high = sea + 400.0f;
    }
    ImGui::Checkbox("Lit", &m_look.lit);
}

} // namespace examples
