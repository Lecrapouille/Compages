// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10d_EarthMoon.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace examples
{

namespace
{

using Point = EarthMoon::Point;

//! \brief Radii and distance, in metres.
constexpr double EARTH_RADIUS = 6.371e6;
constexpr double MOON_RADIUS = 1.7374e6;
constexpr double MOON_DISTANCE = 3.844e8;
//! \brief The orbit the rocket ends on, above the surface of the Moon.
constexpr double LUNAR_ALTITUDE = 1.0e5;

Point operator+(Point p_a, Point p_b)
{
    return { p_a.x + p_b.x, p_a.y + p_b.y, p_a.z + p_b.z };
}

Point operator-(Point p_a, Point p_b)
{
    return { p_a.x - p_b.x, p_a.y - p_b.y, p_a.z - p_b.z };
}

Point operator*(Point p_a, double p_k)
{
    return { p_a.x * p_k, p_a.y * p_k, p_a.z * p_k };
}

double length(Point p_a)
{
    return std::sqrt((p_a.x * p_a.x) + (p_a.y * p_a.y) + (p_a.z * p_a.z));
}

//! \brief Into the World: a float, 24 bits of precision. Fine for an offset
//! from the rocket; 32 m steps for a position near the Moon.
compages::Vector3f toFloat(Point p_a)
{
    return { float(p_a.x), float(p_a.y), float(p_a.z) };
}

//! \brief Turn +Y onto a direction: how the rocket follows its path. A
//! lookAt() would be lost on the pad, where the path points straight up.
compages::Quatf upTo(Point p_direction)
{
    const double size = length(p_direction);
    if (size <= 0.0)
    {
        return compages::Quatf::fromAngleAxis(units::angle::radian_t(0.0),
                                              { 0.0f, 1.0f, 0.0f });
    }
    const Point d = p_direction * (1.0 / size);
    // The axis is Y x d; the angle the one between them.
    const compages::Vector3f axis(float(d.z), 0.0f, float(-d.x));
    const double angle = std::acos(std::clamp(d.y, -1.0, 1.0));
    if (compages::vector::norm(axis) < 1e-6f)
    {
        return compages::Quatf::fromAngleAxis(units::angle::radian_t(angle),
                                              { 1.0f, 0.0f, 0.0f });
    }
    return compages::Quatf::fromAngleAxis(units::angle::radian_t(angle),
                                          compages::vector::normalize(axis));
}

//! \brief Where a place of the globe is on a sphere of the Scene (whose
//! texture is equirectangular), as a unit direction.
Point onGlobe(double p_latitude_deg, double p_longitude_deg)
{
    const double lat = p_latitude_deg * std::numbers::pi / 180.0;
    // Texture column s = (lon + 180) / 360, drawn at u = 1 - s, theta = 2 pi u.
    const double s = (p_longitude_deg + 180.0) / 360.0;
    const double theta = 2.0 * std::numbers::pi * (1.0 - s);
    return { std::cos(lat) * std::cos(theta),
             std::sin(lat),
             std::cos(lat) * std::sin(theta) };
}

//! \brief The Guiana Space Centre, Kourou.
const Point KOUROU = onGlobe(5.2, -52.8);

//! \brief The rotation of the Earth that brings a place to the top (+Y):
//! the inverse of the turn from +Y onto it.
compages::Quatf upFrom(Point p_place)
{
    return upTo(p_place).conjugate();
}

//! \brief The centres, the Earth at the origin of the simulation.
constexpr Point EARTH{ 0.0, 0.0, 0.0 };
const Point MOON{ MOON_DISTANCE * 0.5, 0.0, -MOON_DISTANCE * 0.8660254 };

// ----------------------------------------------------------------------------
//! \brief The path of the rocket: a cubic Hermite curve, from the launch pad
//! (going straight up) to the orbit around the Moon (going sideways). Not
//! orbital mechanics: a path that shows the scale.
// ----------------------------------------------------------------------------
Point path(double p_s, Point* p_tangent)
{
    const Point start{ 0.0, EARTH_RADIUS, 0.0 };
    const Point up{ 0.0, 1.0, 0.0 };
    const Point to_moon = MOON * (1.0 / length(MOON));
    const Point end = MOON - (to_moon * (MOON_RADIUS + LUNAR_ALTITUDE));
    const Point aside{ to_moon.z, 0.0, -to_moon.x };
    const Point t0 = up * MOON_DISTANCE;
    const Point t1 = aside * MOON_DISTANCE;

    const double s = p_s;
    const double s2 = s * s;
    const double s3 = s2 * s;
    if (p_tangent != nullptr)
    {
        *p_tangent = (start * ((6.0 * s2) - (6.0 * s))) +
                     (t0 * ((3.0 * s2) - (4.0 * s) + 1.0)) +
                     (end * ((-6.0 * s2) + (6.0 * s))) +
                     (t1 * ((3.0 * s2) - (2.0 * s)));
    }
    return (start * ((2.0 * s3) - (3.0 * s2) + 1.0)) +
           (t0 * (s3 - (2.0 * s2) + s)) + (end * ((-2.0 * s3) + (3.0 * s2))) +
           (t1 * (s3 - s2));
}

} // namespace

// ----------------------------------------------------------------------------
std::string EarthMoon::description() const
{
    return "A rocket goes from the Earth to the Moon, at the real scale. The "
           "wheel zooms in steps of the same ratio (Shift: faster), from the "
           "rocket to both worlds. The depth is reversed, so a near plane of "
           "half a metre lives with the Moon. The positions are computed in "
           "double and written relative to the rocket: untick Floating "
           "origin in Try it, follow it to the Moon, and it shakes. NASA "
           "textures (Blue Marble, LRO).";
}

// ----------------------------------------------------------------------------
compages::Status EarthMoon::setUp()
{
    const std::filesystem::path earth = dataPath("planets/earth.jpg");
    const std::filesystem::path moon = dataPath("planets/moon.jpg");
    if (earth.empty() || moon.empty())
    {
        return compages::failure(
            "planets/earth.jpg and planets/moon.jpg are missing from "
            "external/Compages-data/");
    }

    m_scene.background(0.0f, 0.0f, 0.01f).ambient(0.04f, 0.04f, 0.05f);
    m_scene.sun("Sun", { 1.0f, 1.0f, 0.95f }, 1.6f);
    // A shadow map spanning 400 000 km would be one blur: no shadows.
    m_scene.environment().shadows = false;

    // A sphere is one unit across: scaled to the diameter.
    m_earth = m_scene.sphere("Earth", compages::renderer::texture(earth))
                  .scale(float(2.0 * EARTH_RADIUS));
    m_moon = m_scene.sphere("Moon", compages::renderer::texture(moon))
                 .scale(float(2.0 * MOON_RADIUS));

    // Kourou, the launch pad, on top (+Y): the simulation starts there.
    m_earth.rotation(upFrom(KOUROU));
    // The Moon always shows the Earth the same face, its longitude 0, which
    // is at -X on the sphere: a turn about Y brings it toward the Earth.
    const Point to_earth = (EARTH - MOON) * (1.0 / length(EARTH - MOON));
    m_moon.rotation(float(std::atan2(to_earth.z, -to_earth.x)),
                    { 0.0f, 1.0f, 0.0f });

    // The rocket stands along +Y, its tail at its origin: a 40 m body and a
    // 12 m nose cone. place() turns it along its path.
    m_rocket = m_world.entity("Rocket");
    m_scene.cylinder("Body", compages::renderer::color(0.9f, 0.9f, 0.92f))
        .parent(m_rocket)
        .position(0.0f, 20.0f, 0.0f)
        .scale(8.0f, 40.0f, 8.0f);
    m_scene.cone("Nose", compages::renderer::color(0.85f, 0.15f, 0.1f))
        .parent(m_rocket)
        .position(0.0f, 46.0f, 0.0f)
        .scale(8.0f, 12.0f, 8.0f);

    // Half a metre to two million kilometres: only with the reversed depth.
    m_camera = m_scene.camera().position(60.0f, 25.0f, 120.0f);
    compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
    lens.near_plane = 0.5f;
    lens.far_plane = 2.0e9f;
    m_camera.add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.0f, 0.0f));
    compages::world::OrbitController& orbit =
        m_camera.get<compages::world::Orbit>().controller;
    orbit.min_distance = 20.0f;
    orbit.max_distance = 1.5e9f;

    place(0.0);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void EarthMoon::place(double p_s)
{
    m_position = path(p_s, &m_heading);

    // Floating origin: the rocket at 0, everything else as an offset from
    // it. Otherwise: the centre of the Earth at 0, and the rocket a float
    // hundreds of thousands of kilometres away.
    const Point origin = m_floating_origin ? m_position : EARTH;
    const compages::Vector3f rocket = toFloat(m_position - origin);
    m_earth.position(toFloat(EARTH - origin));
    m_moon.position(toFloat(MOON - origin));
    m_rocket.position(rocket);
    m_rocket.rotation(upTo(m_heading));
    m_camera.get<compages::world::Orbit>().controller.target = rocket;
}

// ----------------------------------------------------------------------------
void EarthMoon::draw(compages::world::ViewFrame const& p_frame)
{
    if (m_running && (m_progress < 1.0))
    {
        m_progress += double(p_frame.elapsed) / double(m_mission_seconds);
        m_progress = std::min(m_progress, 1.0);
    }
    // Slow on the pad and near the Moon, fast in between: the launch shows.
    place(0.5 - (0.5 * std::cos(std::numbers::pi * m_progress)));
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void EarthMoon::controls()
{
    ImGui::Checkbox("Run", &m_running);
    ImGui::SameLine();
    if (ImGui::Button("Launch again"))
    {
        m_progress = 0.0;
        m_running = true;
    }
    float progress = float(m_progress);
    if (ImGui::SliderFloat("Mission", &progress, 0.0f, 1.0f, "%.4f"))
    {
        m_progress = double(progress);
    }
    ImGui::SliderFloat("Seconds to the Moon", &m_mission_seconds, 10.0f, 600.0f, "%.0f");
    ImGui::Checkbox("Floating origin", &m_floating_origin);
    ImGui::TextWrapped("Wheel: zoom, Shift + wheel: six times faster. "
                       "Right drag: turn around the rocket.");
}

// ----------------------------------------------------------------------------
std::string EarthMoon::hud() const
{
    const double altitude = length(m_position - EARTH) - EARTH_RADIUS;
    const double to_moon = length(MOON - m_position) - MOON_RADIUS;
    const float camera = m_camera.get<compages::world::Orbit>().controller.distance;
    char text[256];
    std::snprintf(text,
                  sizeof(text),
                  "Altitude %.1f km   Moon %.0f km   Camera %.3g km   %s",
                  altitude / 1000.0,
                  to_moon / 1000.0,
                  double(camera) / 1000.0,
                  m_floating_origin ? "floating origin"
                                    : "origin at the Earth: 32 m steps");
    return text;
}

} // namespace examples
