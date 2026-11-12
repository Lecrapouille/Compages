// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "11_Science/11h_Thermometers.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Compages/Renderer/Components/Text.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace examples
{

//! \brief A copper plate 2 m wide, 64 x 64 points; the air at 20 degrees.
static constexpr int POINTS = 64;
static constexpr float WIDTH = 2.0f;
static constexpr float DIFFUSIVITY = 1.1e-4f;
static constexpr float ROOM = 20.0f;

// ----------------------------------------------------------------------------
std::string Thermometers::description() const
{
    return "Follow values over time. A heater goes round a copper plate; two "
           "thermometers read its temperature (Field::sample) and a Chart "
           "draws their curves, the last two minutes, on a board: a "
           "CanvasTexture on a plane. A click on the plate moves the orange "
           "thermometer there (scene.probe). Try it: the speed of time.";
}

// ----------------------------------------------------------------------------
void Thermometers::spread(float p_seconds)
{
    compages::Grid2D const& grid = m_temperature.grid();
    const float h2 = grid.spacing * grid.spacing;
    const int steps = std::max(1, int(std::ceil(p_seconds / (0.2f * h2 / DIFFUSIVITY))));
    const float rate = DIFFUSIVITY * (p_seconds / float(steps)) / h2;
    for (int step = 0; step < steps; ++step)
    {
        m_time += p_seconds / float(steps);
        const compages::Vector2f heater(0.6f * std::cos(0.02f * m_time), 0.6f * std::sin(0.02f * m_time));
        for (int j = 0; j < grid.rows; ++j)
        {
            for (int i = 0; i < grid.columns; ++i)
            {
                const compages::Vector2f p = grid.point(i, j);
                const bool edge = (i == 0) || (j == 0) || (i == grid.columns - 1) || (j == grid.rows - 1);
                const float t = m_temperature(i, j);
                m_next(i, j) = edge ? ROOM
                               : (std::hypot(p.x - heater.x, p.y - heater.y) < 0.12f)
                                   ? 300.0f
                                   : t + (rate * (m_temperature(i - 1, j) + m_temperature(i + 1, j) +
                                                  m_temperature(i, j - 1) + m_temperature(i, j + 1) -
                                                  (4.0f * t)));
            }
        }
        std::swap(m_temperature, m_next);
    }
}

// ----------------------------------------------------------------------------
compages::Status Thermometers::setUp()
{
    const compages::Grid2D grid{ .columns = POINTS,
                                 .rows = POINTS,
                                 .origin = { -0.5f * WIDTH, -0.5f * WIDTH },
                                 .spacing = WIDTH / float(POINTS - 1) };
    m_temperature = compages::Field<float>(grid, ROOM);
    m_next = m_temperature;

    const std::filesystem::path font = dataPath("fonts/DejaVuSans.ttf");
    if (!font.empty())
    {
        m_scene.font(font);
    }
    m_scene.background(0.55f, 0.6f, 0.66f);
    m_scene.camera().position(0.0f, 2.0f, 2.6f).add<compages::world::Orbit>(
        compages::Vector3f(0.0f, 0.75f, -0.6f));
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.2f);
    m_plate = m_scene.show(m_temperature, m_look, "Plate");

    // The board behind the plate: a plane stands up, facing +z.
    // The values axis follows the temperatures: the curves fill the board.
    COMPAGES_TRY(m_chart.create(m_scene,
                                { .width = 1024u,
                                  .height = 512u,
                                  .seconds = 120.0f,
                                  .low = 0.0f,
                                  .high = 100.0f,
                                  .scale = compages::renderer::ChartScale::FollowValues },
                                "Thermometers"));
    compages::world::Entity board = m_scene.plane("Board")
                                        .position(0.0f, 1.35f, -1.2f)
                                        .scale(2.6f, 1.3f, 1.0f);
    m_chart.applyLook(m_scene, board.id());
    // A screen: the picture as it is, not lit by the sun (its dark
    // background would turn grey). The hook "main" of the PBR shader reads
    // the picture the look above gave the board.
    compages::renderer::shaders::ShaderPair screen = compages::renderer::shaders::pbr();
    screen.fragment().hook("main", "void main() { oColor = vec4(texture(baseColorMap, vUv).rgb, opacity); }");
    compages::renderer::Material material;
    COMPAGES_TRY_ASSIGN(material,
                        compages::renderer::Material::create(
                            "Screen", { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                        .shaders = std::move(screen),
                                        .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                        .see_through = std::nullopt }));
    compages::renderer::MaterialId shown;
    COMPAGES_TRY_ASSIGN(shown, m_scene.assets().addMaterial(material.name, std::move(material)));
    m_scene.look(board, shown);
    // A frame round the screen.
    m_scene.box("Screen frame", compages::renderer::color(0.12f, 0.13f, 0.15f))
        .position(0.0f, 1.35f, -1.23f)
        .scale(2.7f, 1.4f, 0.04f);
    board.child("Caption").position(-0.48f, 0.58f, 0.0f)
        .set(compages::renderer::Label{ .text = "Temperature (degrees) over the last 2 minutes",
                                        .color = { 1.0f, 1.0f, 1.0f },
                                        .pixels = 14.0f,
                                        .hide_closer_than = 0.0f });

    const compages::renderer::CanvasTexture::Color colors[2] = {
        compages::renderer::CanvasTexture::Color::rgb(255, 140, 0),
        compages::renderer::CanvasTexture::Color::rgb(80, 180, 255)
    };
    for (std::size_t k = 0u; k < 2u; ++k)
    {
        m_curves[k] = m_chart.series(colors[k]);
        m_marks[k] = m_scene.sphere("Thermometer",
                                    compages::renderer::unlit(float(colors[k].r) / 255.0f,
                                                              float(colors[k].g) / 255.0f,
                                                              float(colors[k].b) / 255.0f))
                         .scale(0.06f);
        // Its reading, written over it (draw() updates the text).
        m_marks[k].set(compages::renderer::Label{ .text = "",
                                                  .color = { float(colors[k].r) / 255.0f,
                                                             float(colors[k].g) / 255.0f,
                                                             float(colors[k].b) / 255.0f },
                                                  .pixels = 16.0f,
                                                  .hide_closer_than = 0.0f });
    }
    // A minute and a half of heating before the first picture.
    for (int k = 0; k < 90; ++k)
    {
        spread(1.0f);
        for (std::size_t n = 0u; n < 2u; ++n)
        {
            m_chart.add(m_curves[n], m_time, m_temperature.sample(m_spots[n]));
        }
    }
    COMPAGES_TRY(m_chart.draw());
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Thermometers::draw(compages::world::ViewFrame const& p_frame)
{
    if (p_frame.input.mouse_left_pressed && p_frame.input.mouse_over)
    {
        if (const auto found = m_scene.probe(m_temperature, m_look, p_frame.input.mouse))
        {
            m_spots[0] = compages::Vector2f(found->point.x, found->point.z);
        }
    }
    spread(m_speed * std::min(p_frame.elapsed, 0.05f));
    for (std::size_t k = 0u; k < 2u; ++k)
    {
        m_chart.add(m_curves[k], m_time, m_temperature.sample(m_spots[k]));
        m_marks[k].position(m_spots[k].x, 0.05f, m_spots[k].y);
        std::array<char, 32> reading{};
        std::snprintf(reading.data(), reading.size(), "%.0f degrees", double(m_temperature.sample(m_spots[k])));
        m_marks[k].get<compages::renderer::Label>().text = reading.data();
    }
    compages::Status shown = m_scene.show(m_plate, m_temperature, m_look);
    if (shown)
    {
        shown = m_chart.draw();
    }
    if (!shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Thermometers::controls()
{
    ImGui::SliderFloat("Time (s per s)", &m_speed, 0.0f, 200.0f, "%.0f");
    ImGui::Text("Orange: %.0f degrees, blue: %.0f degrees", double(m_temperature.sample(m_spots[0])),
                double(m_temperature.sample(m_spots[1])));
}

} // namespace examples
