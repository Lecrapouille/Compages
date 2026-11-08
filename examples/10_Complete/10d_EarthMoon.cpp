// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10d_EarthMoon.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string_view>

//! \brief Each quantity carries its unit (6371.0_km, 5.2_au, 88.0_d,
//! 23.4_deg): the conversions to metres, radians and days are done by the
//! type, never by hand. Its cosine and sine too, in any unit of angle.
using namespace units::literals;
using units::math::cos;
using units::math::sin;

namespace examples
{

namespace
{

using Body = EarthMoon::Body;
using compages::Quatf;
using compages::toMetres;
using compages::Vector3f;
using compages::Vector3g;
using compages::Vector3m;
using compages::vector::cross;
using compages::vector::distance;
using compages::vector::dot;
using compages::vector::normalize;
using units::angle::degree_t;
using units::angle::radian_t;
using units::dimensionless::scalar_t;
using units::length::meter_t;
using units::time::day_t;

//! \brief Radii and distances.
constexpr meter_t EARTH_RADIUS = 6371.0_km;
constexpr meter_t MOON_RADIUS = 1737.4_km;
constexpr meter_t MOON_DISTANCE = 384400.0_km;
//! \brief The orbit the rocket ends on, above the surface of the Moon.
constexpr meter_t LUNAR_ALTITUDE = 100.0_km;
//! \brief The rocket: what "Look at" frames when it follows the rocket.
constexpr meter_t ROCKET_SIZE = 30.0_m;
//! \brief A thing seen from closer than this many times its radius is
//! larger than a dot (a fiftieth of the distance): it needs no name.
constexpr double NAMED_BEYOND_RADII = 50.0;

//! \brief 6 November 2026, as the time since 1 January 2000 at noon (J2000).
constexpr day_t START_DATE = 9805.5_d;
//! \brief The Earth on its orbit: mean longitude at J2000, and the year.
constexpr degree_t EARTH_LONGITUDE = 100.464_deg;
constexpr day_t EARTH_PERIOD = 365.256_d;
//! \brief Tilt of the axis of the Earth on the ecliptic.
constexpr degree_t OBLIQUITY = 23.4393_deg;

//! \brief Names for "Look at": the rocket, the Earth, the Moon, then the
//! bodies in the order of makeBodies(), from FIRST_BODY on.
constexpr std::array FOCUS_NAMES = { "Rocket", "Earth",   "Moon",   "Sun",
                                     "Mercury", "Venus",  "Mars",   "Jupiter",
                                     "Saturn",  "Uranus", "Neptune" };
constexpr std::size_t FIRST_BODY = 3u;
constexpr std::size_t SATURN_FOCUS = 8u;
static_assert(std::string_view(FOCUS_NAMES[FIRST_BODY]) == "Sun");
static_assert(std::string_view(FOCUS_NAMES[SATURN_FOCUS]) == "Saturn");

// ----------------------------------------------------------------------------
//! \brief Where a place of the globe is on a sphere of the Scene (whose
//! texture is equirectangular).
//! \param[in] p_latitude North positive.
//! \param[in] p_longitude East positive.
//! \return A unit direction from the centre of the sphere.
// ----------------------------------------------------------------------------
Vector3g onGlobe(degree_t p_latitude, degree_t p_longitude)
{
    // Texture column s = (lon + 180) / 360, drawn at u = 1 - s, theta = u turns.
    const double s = ((p_longitude + 180.0_deg) / 360.0_deg).value();
    const radian_t theta = units::angle::turn_t(1.0 - s);
    return { cos(p_latitude) * cos(theta), sin(p_latitude), cos(p_latitude) * sin(theta) };
}

//! \brief The Guiana Space Centre, Kourou.
const Vector3g KOUROU = onGlobe(5.2_deg, -52.8_deg);

//! \brief The centres, the Earth at the origin of the simulation, the Moon
//! a sixth of a turn from +X.
const Vector3m EARTH(0.0_m, 0.0_m, 0.0_m);
const Vector3m MOON = Vector3g(0.5, 0.0, -0.8660254) * MOON_DISTANCE;

// ----------------------------------------------------------------------------
//! \brief Mean longitude on a circular orbit: a turn per period.
//! \param[in] p_at_j2000 The mean longitude on 1 January 2000 at noon.
//! \param[in] p_period The time of one orbit.
//! \param[in] p_date The time since J2000.
//! \return The mean longitude at \c p_date.
// ----------------------------------------------------------------------------
degree_t meanLongitude(degree_t p_at_j2000, day_t p_period, day_t p_date)
{
    return p_at_j2000 + units::angle::turn_t((p_date / p_period).value());
}

// ----------------------------------------------------------------------------
//! \brief The plane of the planets (the ecliptic) in the simulation, on a
//! date.
//!
//! The simulation keeps the Earth at its centre and the launch pad on top,
//! so the solar system is turned to fit: the Sun is always seen from the
//! Earth in the same direction, above the pad, and the planets keep their
//! true places around it.
// ----------------------------------------------------------------------------
struct Ecliptic
{
    //! \brief The centre of the Sun, 1 au from the Earth.
    Vector3m sun;
    //! \brief The direction of ecliptic longitude 0 (the vernal point), seen
    //! from the Sun.
    Vector3g x;
    //! \brief The direction of ecliptic longitude 90 degrees.
    Vector3g y;
    //! \brief The north of the ecliptic: x, y, north turn like X, Y, Z.
    Vector3g north;

    //! \brief The ecliptic on a date: the Earth has moved on its orbit, so
    //! the planets turn around the Sun as seen from the Earth.
    //! \param[in] p_date The time since J2000.
    explicit Ecliptic(day_t p_date)
    {
        const Vector3g to_sun = normalize(Vector3g(0.5, 0.6, 0.6));
        north = normalize(Vector3g::UNIT_Y - (to_sun * dot(Vector3g::UNIT_Y, to_sun)));
        sun = Vector3m(to_sun * 1.0_au);
        // Seen from the Earth, the Sun is half a turn from the Earth seen
        // from the Sun: the longitude 0 is that far back from the Sun.
        const radian_t angle =
            -(meanLongitude(EARTH_LONGITUDE, EARTH_PERIOD, p_date) + 180.0_deg);
        x = (to_sun * cos(angle)) + (cross(north, to_sun) * sin(angle));
        y = cross(north, x);
    }

    //! \brief The direction of an ecliptic longitude.
    //! \param[in] p_longitude The ecliptic longitude.
    //! \return A unit direction in the plane of the ecliptic.
    [[nodiscard]] Vector3g direction(radian_t p_longitude) const
    {
        return (x * cos(p_longitude)) + (y * sin(p_longitude));
    }

    //! \brief A point of a circular orbit around the Sun.
    //! \param[in] p_radius The radius of the orbit.
    //! \param[in] p_longitude The ecliptic longitude of the point.
    //! \return The point, in metres.
    [[nodiscard]] Vector3m onOrbit(meter_t p_radius, radian_t p_longitude) const
    {
        return sun + (direction(p_longitude) * p_radius);
    }

    //! \brief The ecliptic longitude of a point, seen from the Sun.
    //! \param[in] p_point A point of the plane of the ecliptic, in metres.
    //! \return Its longitude, in ]-180, 180] degrees.
    [[nodiscard]] radian_t longitude(Vector3m const& p_point) const
    {
        const Vector3m from = p_point - sun;
        return units::math::atan2(dot(from, y), dot(from, x));
    }

    //! \brief A direction of the sky, given as the astronomers do: right
    //! ascension and declination, on the equator of the Earth.
    //! \param[in] p_ra The right ascension.
    //! \param[in] p_dec The declination.
    //! \return A unit direction of the simulation.
    [[nodiscard]] Vector3g sky(degree_t p_ra, degree_t p_dec) const
    {
        // Equator to ecliptic: a turn of the obliquity about the X axis.
        const scalar_t along_x = cos(p_dec) * cos(p_ra);
        const scalar_t along_y =
            (cos(p_dec) * sin(p_ra) * cos(OBLIQUITY)) + (sin(p_dec) * sin(OBLIQUITY));
        const scalar_t along_north =
            (sin(p_dec) * cos(OBLIQUITY)) - (cos(p_dec) * sin(p_ra) * sin(OBLIQUITY));
        return (x * along_x) + (y * along_y) + (north * along_north);
    }
};

// ----------------------------------------------------------------------------
//! \brief The Sun and the planets other than the Earth: radius, mean
//! distance to the Sun, mean longitude at J2000, time of one orbit, and the
//! direction of the north pole. Circular orbits in the ecliptic: right to
//! a few degrees, enough to see where each one is. (The Sun does not move:
//! its period only avoids a division by zero.)
//! \return The Sun first, then the planets from Mercury to Neptune.
// ----------------------------------------------------------------------------
std::vector<Body> makeBodies()
{
    return {
        { "Sun", nullptr, 695700.0_km, 0.0_au, 0.0_deg, 1.0_d, 286.13_deg, 63.87_deg },
        { "Mercury", "planets/mercury.jpg", 2439.7_km, 0.3871_au, 252.251_deg, 87.969_d, 281.01_deg, 61.41_deg },
        { "Venus", "planets/venus.jpg", 6051.8_km, 0.7233_au, 181.980_deg, 224.701_d, 272.76_deg, 67.16_deg },
        { "Mars", "planets/mars.jpg", 3389.5_km, 1.5237_au, 355.433_deg, 686.980_d, 317.68_deg, 52.89_deg },
        { "Jupiter", "planets/jupiter.jpg", 69911.0_km, 5.2026_au, 34.351_deg, 4332.59_d, 268.06_deg, 64.50_deg },
        { "Saturn", "planets/saturn.jpg", 58232.0_km, 9.5549_au, 50.077_deg, 10759.22_d, 40.59_deg, 83.54_deg },
        { "Uranus", "planets/uranus.jpg", 25362.0_km, 19.2184_au, 314.055_deg, 30688.5_d, 257.31_deg, -15.18_deg },
        { "Neptune", "planets/neptune.jpg", 24622.0_km, 30.1104_au, 304.348_deg, 60182.0_d, 299.36_deg, 43.46_deg },
    };
}

// ----------------------------------------------------------------------------
//! \brief A flat ring in the XZ plane, seen from both sides (each triangle
//! twice, once per winding: the culling keeps one).
//! \param[in] p_inner The radius of its inner edge.
//! \param[in] p_outer The radius of its outer edge.
//! \return The mesh, centred on the origin.
// ----------------------------------------------------------------------------
compages::renderer::MeshAsset makeRing(meter_t p_inner, meter_t p_outer)
{
    constexpr std::uint32_t CORNERS = 180u;
    compages::renderer::MeshAsset mesh;
    for (std::uint32_t i = 0u; i < CORNERS; ++i)
    {
        const radian_t angle = units::angle::turn_t(double(i) / double(CORNERS));
        for (const meter_t radius : { p_inner, p_outer })
        {
            compages::renderer::MeshVertex vertex;
            vertex.position =
                Vector3f(toMetres(radius * cos(angle)), 0.0f, toMetres(radius * sin(angle)));
            vertex.normal = Vector3f::UNIT_Y;
            mesh.source_vertices.emplace_back(vertex);
        }
        const std::uint32_t a = 2u * i;
        const std::uint32_t b = 2u * ((i + 1u) % CORNERS);
        for (const std::uint32_t index :
             { a, a + 1u, b, b, a + 1u, b + 1u, a, b, a + 1u, b, b + 1u, a + 1u })
        {
            mesh.source_indices.emplace_back(index);
        }
    }
    const float outer = toMetres(p_outer);
    mesh.index_count = mesh.source_indices.size();
    mesh.local_bounds = compages::AABB::fromCorners(Vector3f(-outer, -1.0f, -outer),
                                                    Vector3f(outer, 1.0f, outer));
    return mesh;
}

// ----------------------------------------------------------------------------
//! \brief The path of the rocket: a cubic Hermite curve, from the launch pad
//! (going straight up) to the orbit around the Moon (going sideways). Not
//! orbital mechanics: a path that shows the scale.
// ----------------------------------------------------------------------------
struct Path
{
    //! \brief The launch pad, on top of the Earth.
    Vector3m start;
    //! \brief The orbit around the Moon, on the side of the Earth.
    Vector3m end;
    //! \brief The tangent at the start: straight up.
    Vector3m leave;
    //! \brief The tangent at the end: sideways, to go round the Moon.
    Vector3m arrive;

    //! \brief The path from Kourou (on top) to the Moon. The tangents are as
    //! long as the trip, for a curve that neither loops nor goes straight.
    Path()
    {
        const Vector3g to_moon = normalize(MOON);
        start = Vector3g::UNIT_Y * EARTH_RADIUS;
        end = MOON - (to_moon * (MOON_RADIUS + LUNAR_ALTITUDE));
        leave = Vector3g::UNIT_Y * MOON_DISTANCE;
        arrive = Vector3g(to_moon.z, 0.0, -to_moon.x) * MOON_DISTANCE;
    }

    //! \brief Where the rocket is.
    //! \param[in] p_s Where on the path, 0 on the pad, 1 near the Moon.
    //! \return The position, in metres.
    [[nodiscard]] Vector3m at(double p_s) const
    {
        const double s2 = p_s * p_s;
        const double s3 = s2 * p_s;
        return (start * ((2.0 * s3) - (3.0 * s2) + 1.0)) + (leave * (s3 - (2.0 * s2) + p_s)) +
               (end * ((-2.0 * s3) + (3.0 * s2))) + (arrive * (s3 - s2));
    }

    //! \brief Where the rocket goes: the derivative of at().
    //! \param[in] p_s Where on the path, 0 on the pad, 1 near the Moon.
    //! \return The tangent, in metres per unit of \c p_s.
    [[nodiscard]] Vector3m tangent(double p_s) const
    {
        const double s2 = p_s * p_s;
        return (start * ((6.0 * s2) - (6.0 * p_s))) + (leave * ((3.0 * s2) - (4.0 * p_s) + 1.0)) +
               (end * ((-6.0 * s2) + (6.0 * p_s))) + (arrive * ((3.0 * s2) - (2.0 * p_s)));
    }
};

} // namespace

// ----------------------------------------------------------------------------
std::string EarthMoon::description() const
{
    return "A rocket goes from the Earth to the Moon, at the real scale, with "
           "the Sun and the planets where they are on the chosen date. The "
           "wheel zooms in steps of the same ratio (Shift: faster), from the "
           "rocket to Neptune. The depth is reversed, so a near plane of half "
           "a metre lives with the whole solar system. The positions are "
           "computed in double and written relative to what you look at: "
           "untick Floating origin in Try it, follow the rocket to the Moon, "
           "and it shakes. Textures: NASA (Earth, Moon), Solar System Scope.";
}

// ----------------------------------------------------------------------------
compages::Status EarthMoon::setUp()
{
    const std::filesystem::path earth = dataPath("planets/earth.jpg");
    const std::filesystem::path moon = dataPath("planets/moon.jpg");
    const std::filesystem::path font = dataPath("fonts/DejaVuSans.ttf");
    if (earth.empty() || moon.empty() || font.empty())
    {
        return compages::failure(
            "planets/earth.jpg, planets/moon.jpg and fonts/DejaVuSans.ttf are "
            "missing from external/Compages-data/");
    }

    m_scene.background(0.0f, 0.0f, 0.01f).ambient(0.04f, 0.04f, 0.05f);
    // A shadow map spanning the solar system would be one blur: no shadows.
    m_scene.environment().shadows = false;

    // A sphere is one unit (one metre) across: scaled to the diameter.
    m_earth = m_scene.sphere("Earth", compages::renderer::texture(earth))
                  .scale(toMetres(2.0 * EARTH_RADIUS));
    m_moon = m_scene.sphere("Moon", compages::renderer::texture(moon))
                 .scale(toMetres(2.0 * MOON_RADIUS));

    // Kourou, the launch pad, on top (+Y): the simulation starts there.
    m_earth.rotation(Quatf(compages::upFrom(KOUROU)));
    // The Moon always shows the Earth the same face, its longitude 0, which
    // is at -X on the sphere: a turn about Y brings it toward the Earth.
    const Vector3m to_earth = EARTH - MOON;
    m_moon.rotation(units::math::atan2(to_earth.z, -to_earth.x), Vector3f::UNIT_Y);

    // The Sun and the planets. Each hangs its sphere from a pivot, which
    // place() moves and movePlanets() tilts along the pole of the planet.
    m_bodies = makeBodies();
    for (Body& body : m_bodies)
    {
        body.pivot = m_world.entity(body.name);
        compages::renderer::Look look = compages::renderer::unlit(1.0f, 0.9f, 0.7f);
        if (body.file != nullptr)
        {
            const std::filesystem::path picture = dataPath(body.file);
            if (picture.empty())
            {
                return compages::failure(std::string(body.file) +
                                         " is missing from external/Compages-data/");
            }
            look = compages::renderer::texture(picture);
        }
        m_scene.sphere(std::string(body.name) + " globe", look)
            .parent(body.pivot)
            .scale(toMetres(2.0 * body.radius));
    }

    // The Sun is a lamp, not a directional light: each planet is lit from
    // where the Sun is for it. The range is far past Neptune, so the light
    // hardly fades: real sunlight at Neptune is 900 times dimmer, and the
    // planet would be black.
    m_sunlight = m_scene.lamp("Sunlight", { 1.0f, 1.0f, 0.95f }, 1.6f, toMetres(700.0_au))
                     .parent(m_bodies.front().pivot);

    // The rings of Saturn, in its equator: C (faint), B (bright), the
    // Cassini division, A.
    struct Band
    {
        meter_t inner;
        meter_t outer;
        float shade;
        float opacity;
    };
    for (Band const& band : { Band{ 74660.0_km, 92000.0_km, 0.55f, 0.25f },
                              Band{ 92000.0_km, 117580.0_km, 0.90f, 0.85f },
                              Band{ 122170.0_km, 136780.0_km, 0.80f, 0.60f } })
    {
        m_scene
            .mesh(makeRing(band.inner, band.outer),
                  "Saturn rings",
                  compages::renderer::seeThrough(
                      compages::renderer::unlit(band.shade,
                                                band.shade * 0.9f,
                                                band.shade * 0.75f),
                      band.opacity))
            .parent(m_bodies[SATURN_FOCUS - FIRST_BODY].pivot);
    }

    // The rocket stands along +Y, its tail at its origin: a 40 m body and a
    // 12 m nose cone, 8 m wide. place() turns it along its path.
    m_rocket = m_world.entity("Rocket");
    m_scene.cylinder("Body", compages::renderer::color(0.9f, 0.9f, 0.92f))
        .parent(m_rocket)
        .position(0.0f, toMetres(20.0_m), 0.0f)
        .scale(toMetres(8.0_m), toMetres(40.0_m), toMetres(8.0_m));
    m_scene.cone("Nose", compages::renderer::color(0.85f, 0.15f, 0.1f))
        .parent(m_rocket)
        .position(0.0f, toMetres(46.0_m), 0.0f)
        .scale(toMetres(8.0_m), toMetres(12.0_m), toMetres(8.0_m));

    // Half a metre to 130 astronomical units: only with the reversed depth.
    m_camera = m_scene.camera().position(60.0f, 25.0f, 120.0f);
    compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
    lens.near_plane = toMetres(0.5_m);
    lens.far_plane = toMetres(130.0_au);
    m_camera.add<compages::world::Orbit>(Vector3f::ZERO);
    compages::world::OrbitController& orbit =
        m_camera.get<compages::world::Orbit>().controller;
    orbit.min_distance = toMetres(20.0_m);
    orbit.max_distance = toMetres(50.0_au);

    // The names: from afar a planet is smaller than a pixel, its name finds
    // it. The Scene projects each one where its entity is, at each picture.
    m_scene.font(font);
    const compages::world::Entity named[FIRST_BODY] = { m_rocket, m_earth, m_moon };
    for (std::size_t i = 0u; i < FOCUS_NAMES.size(); ++i)
    {
        compages::world::Entity thing = (i < FIRST_BODY) ? named[i] : m_bodies[i - FIRST_BODY].pivot;
        thing.set(compages::renderer::Label{
            .text = FOCUS_NAMES[i],
            .color = { 1.0f, 0.96f, 0.88f },
            .pixels = 15.0f,
            .hide_closer_than = toMetres(NAMED_BEYOND_RADII * focus(i).radius) });
    }

    // The numbers, top left of the picture of the camera; draw() writes
    // them each frame.
    m_camera.set(compages::renderer::Caption{ .text = "",
                                              .color = { 1.0f, 0.96f, 0.88f },
                                              .pixels = 16.0f,
                                              .at = { 12.0f, 10.0f } });

    m_date = START_DATE;
    movePlanets();
    place(0.0);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void EarthMoon::movePlanets()
{
    const Ecliptic ecliptic(m_date);
    for (Body& body : m_bodies)
    {
        body.at = ecliptic.onOrbit(body.distance,
                                   meanLongitude(body.longitude, body.period, m_date));
        body.pivot.rotation(Quatf(compages::upTo(ecliptic.sky(body.pole_ra, body.pole_dec))));
    }
}

// ----------------------------------------------------------------------------
EarthMoon::Focus EarthMoon::focus(std::size_t p_focus) const
{
    switch (p_focus)
    {
        case 0u:
            return { m_position, ROCKET_SIZE };
        case 1u:
            return { EARTH, EARTH_RADIUS };
        case 2u:
            return { MOON, MOON_RADIUS };
        default:
            Body const& body = m_bodies[p_focus - FIRST_BODY];
            return { body.at, body.radius };
    }
}

// ----------------------------------------------------------------------------
void EarthMoon::lookAt(std::size_t p_focus)
{
    m_focus = std::min(p_focus, FOCUS_NAMES.size() - 1u);
    const meter_t radius = focus(m_focus).radius;
    compages::world::OrbitController& orbit =
        m_camera.get<compages::world::Orbit>().controller;
    // Stay out of the world, and frame it; Saturn with its rings.
    orbit.min_distance = toMetres((m_focus == 0u) ? 20.0_m : (1.2 * radius));
    orbit.distance = toMetres(((m_focus == SATURN_FOCUS) ? 6.0 : 4.0) * radius);
    // A cut, not a glide: the target jumps too.
    orbit.snap();
}

// ----------------------------------------------------------------------------
void EarthMoon::place(double p_s)
{
    const Path path;
    m_position = path.at(p_s);
    m_heading = path.tangent(p_s);

    // Floating origin: what the camera looks at is at 0, everything else is
    // an offset from it. Otherwise: the centre of the Earth at 0, and the
    // rocket a float hundreds of thousands of kilometres away.
    m_origin = m_floating_origin ? focus(m_focus).centre : EARTH;
    m_earth.position(fromOrigin(EARTH));
    m_moon.position(fromOrigin(MOON));
    for (Body& body : m_bodies)
    {
        body.pivot.position(fromOrigin(body.at));
    }
    m_rocket.position(fromOrigin(m_position));
    m_rocket.rotation(Quatf(compages::upTo(m_heading)));
    m_camera.get<compages::world::Orbit>().controller.target =
        fromOrigin(focus(m_focus).centre);
}

// ----------------------------------------------------------------------------
void EarthMoon::draw(compages::world::ViewFrame const& p_frame)
{
    const units::time::second_t elapsed(p_frame.elapsed);
    if (m_running && (m_progress < 1.0))
    {
        m_progress += double(p_frame.elapsed) / double(m_mission_seconds);
        m_progress = std::min(m_progress, 1.0);
    }
    // The date runs: the planets go round the Sun.
    if (m_days_per_second > 0.0f)
    {
        m_date += day_t(double(m_days_per_second) * elapsed.value());
        movePlanets();
    }

    // Slow on the pad and near the Moon, fast in between: the launch shows.
    place(0.5 - (0.5 * cos(units::angle::turn_t(0.5 * m_progress))));

    // A curve of the simulation, as debug lines from the floating origin:
    // p_point(t) for t from 0 to 1.
    auto polyline = [&](int p_segments, auto&& p_point, Vector3f const& p_color)
    {
        Vector3m previous = p_point(0.0);
        for (int i = 1; i <= p_segments; ++i)
        {
            const Vector3m next = p_point(double(i) / double(p_segments));
            m_scene.debug().line(fromOrigin(previous), fromOrigin(next), p_color);
            previous = next;
        }
    };

    // The path of the rocket, in red, from the pad to the Moon.
    if (m_trajectory)
    {
        const Path path;
        polyline(400, [&](double p_s) { return path.at(p_s); }, { 1.0f, 0.1f, 0.1f });
    }

    // The orbits: from afar a planet is smaller than a pixel, its circle
    // shows where it is.
    if (m_orbits)
    {
        const Ecliptic ecliptic(m_date);
        // Started at the planet, so that it sits on a corner of the circle.
        auto circle = [&](meter_t p_radius, Vector3m const& p_planet, Vector3f const& p_color)
        {
            const radian_t start = ecliptic.longitude(p_planet);
            polyline(
                256,
                [&](double p_turn)
                { return ecliptic.onOrbit(p_radius, start + units::angle::turn_t(p_turn)); },
                p_color);
        };
        circle(1.0_au, EARTH, { 0.2f, 0.4f, 0.9f });
        for (std::size_t i = 1u; i < m_bodies.size(); ++i)
        {
            circle(m_bodies[i].distance, m_bodies[i].at, { 0.35f, 0.35f, 0.4f });
        }
    }
    m_camera.get<compages::renderer::Caption>().text = status();
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

    int focus = int(m_focus);
    if (ImGui::Combo("Look at", &focus, FOCUS_NAMES.data(), int(FOCUS_NAMES.size())))
    {
        lookAt(std::size_t(focus));
    }
    // Saturn shows its rings edge-on in 2025 and wide open in 2017. A Julian
    // year is 365.25 days, the year of J2000.
    float year = float(2000.0 + units::time::julian_year_t(m_date).value());
    if (ImGui::SliderFloat("Year", &year, 1950.0f, 2100.0f, "%.2f"))
    {
        m_date = units::time::julian_year_t(double(year) - 2000.0);
        movePlanets();
    }
    ImGui::SliderFloat("Days per second", &m_days_per_second, 0.0f, 3650.0f, "%.1f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::Checkbox("Orbits", &m_orbits);
    ImGui::SameLine();
    ImGui::Checkbox("Trajectory", &m_trajectory);
    ImGui::SameLine();
    ImGui::Checkbox("Floating origin", &m_floating_origin);
    ImGui::TextWrapped("Wheel: zoom, Shift + wheel: six times faster. "
                       "Right drag: turn around what you look at.");
}

// ----------------------------------------------------------------------------
std::string EarthMoon::status() const
{
    using units::length::kilometer_t;
    const kilometer_t altitude = distance(m_position, EARTH) - EARTH_RADIUS;
    const kilometer_t to_moon = distance(MOON, m_position) - MOON_RADIUS;
    const kilometer_t camera =
        meter_t(m_camera.get<compages::world::Orbit>().controller.distance);
    char text[256];
    std::snprintf(text,
                  sizeof(text),
                  "Altitude %.1f km   Moon %.0f km   %s, camera %.3g km   %s",
                  altitude.value(),
                  to_moon.value(),
                  FOCUS_NAMES[m_focus],
                  camera.value(),
                  m_floating_origin ? "floating origin"
                                    : "origin at the Earth: 32 m steps");
    return text;
}

} // namespace examples
