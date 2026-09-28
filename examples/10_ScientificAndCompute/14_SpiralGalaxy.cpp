//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================
//
// Credits: https://glumpy.github.io/
// based on https://github.com/glumpy/glumpy/blob/master/examples/galaxy.py
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
//=============================================================================

#include "10_ScientificAndCompute/14_SpiralGalaxy.hpp"

#include "Compages/Core/Transformation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using namespace units::literals;

namespace examples
{
namespace
{

//! \brief How large the disk is, in parsecs. Positions are divided by this
//! before they reach the shader, so the disk is about one unit across.
constexpr float GALAXY_RADIUS = 13000.0f;
constexpr float CORE_RADIUS = 4000.0f;
//! \brief Extra turn per parsec. Stored in the same number the cosine reads,
//! which is radians, even though the orbital angle itself is in degrees. The
//! Glumpy port does this, and the tightness of the arms depends on it.
constexpr float ANGLE_OFFSET = 0.0004f;
constexpr float INNER_ECCENTRICITY = 0.90f;
constexpr float OUTER_ECCENTRICITY = 0.90f;
//! \brief Degrees per year. One frame advances a hundred thousand years.
constexpr float SPIN = 0.000005f;
constexpr float YEARS_PER_FRAME = 100000.0f;
constexpr float TEMPERATURE_MIN = 1000.0f;
constexpr float TEMPERATURE_MAX = 10000.0f;
//! \brief Pixel size at an 800 pixel window. Stars sit just above the shader's
//! cutoff; dust is a wide, dim sprite.
constexpr float STAR_SIZE = 3.0f;
constexpr float DUST_SIZE = 64.0f;
constexpr float STAR = 0.0f;
constexpr float DUST = 1.0f;
constexpr float CLOUD = 2.0f;
constexpr float CORE = 3.0f;
constexpr std::uint32_t STARS = 35000u;
constexpr std::uint32_t DUST_COUNT = STARS * 3u / 4u;
constexpr std::uint32_t CLOUDS = 200u;
constexpr std::uint32_t RAMP = 256u;
constexpr std::uint32_t SPRITE = 64u;

constexpr char const* VERTEX = R"(#version 450 core
in vec2 position;
in float size;
in float kind;
in float temperature;
in float brightness;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler1D colormap;

out vec3 vColor;

void main()
{
    vColor = texture(colormap, temperature).rgb * brightness;
    // 2 is the hydrogen cloud, pulled toward red. 3 is its core, white.
    if (kind == 2.0)
    {
        vColor *= vec3(2.0, 1.0, 1.0);
    }
    else if (kind == 3.0)
    {
        vColor = vec3(0.9);
    }

    // A point size of zero is undefined. A body meant to vanish (a distant
    // hydrogen core) is sent outside the clip volume instead.
    if (size > 2.0)
    {
        gl_PointSize = size;
        gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
    }
    else
    {
        gl_PointSize = 1.0;
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }
}
)";

constexpr char const* FRAGMENT = R"(#version 450 core
in vec3 vColor;

uniform sampler2D sprite;

out vec4 oColor;

void main()
{
    // The sprite is a soft disc in its red channel. Alpha stays one: with
    // additive blending, a black texel adds nothing, which is the edge.
    oColor = vec4(texture(sprite, gl_PointCoord).r * vColor, 1.0);
}
)";

//! \brief CIE 1931 colour matching functions, 380 nm then every 5 nm. The last
//! row is 780 nm and is not sampled: the Glumpy loop stops before it. Public
//! domain, John Walker.
constexpr double CIE[][3] = {
    { 0.0014, 0.0000, 0.0065 }, { 0.0022, 0.0001, 0.0105 },
    { 0.0042, 0.0001, 0.0201 }, { 0.0076, 0.0002, 0.0362 },
    { 0.0143, 0.0004, 0.0679 }, { 0.0232, 0.0006, 0.1102 },
    { 0.0435, 0.0012, 0.2074 }, { 0.0776, 0.0022, 0.3713 },
    { 0.1344, 0.0040, 0.6456 }, { 0.2148, 0.0073, 1.0391 },
    { 0.2839, 0.0116, 1.3856 }, { 0.3285, 0.0168, 1.6230 },
    { 0.3483, 0.0230, 1.7471 }, { 0.3481, 0.0298, 1.7826 },
    { 0.3362, 0.0380, 1.7721 }, { 0.3187, 0.0480, 1.7441 },
    { 0.2908, 0.0600, 1.6692 }, { 0.2511, 0.0739, 1.5281 },
    { 0.1954, 0.0910, 1.2876 }, { 0.1421, 0.1126, 1.0419 },
    { 0.0956, 0.1390, 0.8130 }, { 0.0580, 0.1693, 0.6162 },
    { 0.0320, 0.2080, 0.4652 }, { 0.0147, 0.2586, 0.3533 },
    { 0.0049, 0.3230, 0.2720 }, { 0.0024, 0.4073, 0.2123 },
    { 0.0093, 0.5030, 0.1582 }, { 0.0291, 0.6082, 0.1117 },
    { 0.0633, 0.7100, 0.0782 }, { 0.1096, 0.7932, 0.0573 },
    { 0.1655, 0.8620, 0.0422 }, { 0.2257, 0.9149, 0.0298 },
    { 0.2904, 0.9540, 0.0203 }, { 0.3597, 0.9803, 0.0134 },
    { 0.4334, 0.9950, 0.0087 }, { 0.5121, 1.0000, 0.0057 },
    { 0.5945, 0.9950, 0.0039 }, { 0.6784, 0.9786, 0.0027 },
    { 0.7621, 0.9520, 0.0021 }, { 0.8425, 0.9154, 0.0018 },
    { 0.9163, 0.8700, 0.0017 }, { 0.9786, 0.8163, 0.0014 },
    { 1.0263, 0.7570, 0.0011 }, { 1.0567, 0.6949, 0.0010 },
    { 1.0622, 0.6310, 0.0008 }, { 1.0456, 0.5668, 0.0006 },
    { 1.0026, 0.5030, 0.0003 }, { 0.9384, 0.4412, 0.0002 },
    { 0.8544, 0.3810, 0.0002 }, { 0.7514, 0.3210, 0.0001 },
    { 0.6424, 0.2650, 0.0000 }, { 0.5419, 0.2170, 0.0000 },
    { 0.4479, 0.1750, 0.0000 }, { 0.3608, 0.1382, 0.0000 },
    { 0.2835, 0.1070, 0.0000 }, { 0.2187, 0.0816, 0.0000 },
    { 0.1649, 0.0610, 0.0000 }, { 0.1212, 0.0446, 0.0000 },
    { 0.0874, 0.0320, 0.0000 }, { 0.0636, 0.0232, 0.0000 },
    { 0.0468, 0.0170, 0.0000 }, { 0.0329, 0.0119, 0.0000 },
    { 0.0227, 0.0082, 0.0000 }, { 0.0158, 0.0057, 0.0000 },
    { 0.0114, 0.0041, 0.0000 }, { 0.0081, 0.0029, 0.0000 },
    { 0.0058, 0.0021, 0.0000 }, { 0.0041, 0.0015, 0.0000 },
    { 0.0029, 0.0010, 0.0000 }, { 0.0020, 0.0007, 0.0000 },
    { 0.0014, 0.0005, 0.0000 }, { 0.0010, 0.0004, 0.0000 },
    { 0.0007, 0.0002, 0.0000 }, { 0.0005, 0.0002, 0.0000 },
    { 0.0003, 0.0001, 0.0000 }, { 0.0002, 0.0001, 0.0000 },
    { 0.0002, 0.0001, 0.0000 }, { 0.0001, 0.0000, 0.0000 },
    { 0.0001, 0.0000, 0.0000 }, { 0.0001, 0.0000, 0.0000 },
    { 0.0000, 0.0000, 0.0000 },
};

struct Rgb
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

//--------------------------------------------------------------------------
double blackbody(double p_wavelength_nm, double p_temperature)
{
    const double metres = p_wavelength_nm * 1.0e-9;
    return (3.74183e-16 * std::pow(metres, -5.0)) /
           (std::exp(1.4388e-2 / (metres * p_temperature)) - 1.0);
}

//--------------------------------------------------------------------------
//! \brief SMPTE primaries. The weights are Walker's xyz_to_rgb.
//--------------------------------------------------------------------------
Rgb xyzToRgb(double p_x, double p_y, double p_z)
{
    constexpr double xr = 0.63;
    constexpr double yr = 0.34;
    constexpr double xg = 0.31;
    constexpr double yg = 0.595;
    constexpr double xb = 0.155;
    constexpr double yb = 0.07;
    constexpr double xw = 0.3127;
    constexpr double yw = 0.3291;
    constexpr double zr = 1.0 - (xr + yr);
    constexpr double zg = 1.0 - (xg + yg);
    constexpr double zb = 1.0 - (xb + yb);
    constexpr double zw = 1.0 - (xw + yw);

    double rx = (yg * zb) - (yb * zg);
    double ry = (xb * zg) - (xg * zb);
    double rz = (xg * yb) - (xb * yg);
    double gx = (yb * zr) - (yr * zb);
    double gy = (xr * zb) - (xb * zr);
    double gz = (xb * yr) - (xr * yb);
    double bx = (yr * zg) - (yg * zr);
    double by = (xg * zr) - (xr * zg);
    double bz = (xr * yg) - (xg * yr);

    const double rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
    const double gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
    const double bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;
    rx /= rw;
    ry /= rw;
    rz /= rw;
    gx /= gw;
    gy /= gw;
    gz /= gw;
    bx /= bw;
    by /= bw;
    bz /= bw;

    return { (rx * p_x) + (ry * p_y) + (rz * p_z),
             (gx * p_x) + (gy * p_y) + (gz * p_z),
             (bx * p_x) + (by * p_y) + (bz * p_z) };
}

//--------------------------------------------------------------------------
Rgb spectrum(double p_temperature)
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    constexpr std::size_t rows = sizeof(CIE) / sizeof(CIE[0]);
    for (std::size_t i = 0u; i < rows; ++i)
    {
        const double lambda = 380.0 + (5.0 * static_cast<double>(i));
        if (lambda >= 780.0)
        {
            break;
        }
        const double emitted = blackbody(lambda, p_temperature);
        x += emitted * CIE[i][0];
        y += emitted * CIE[i][1];
        z += emitted * CIE[i][2];
    }
    const double sum = x + y + z;
    Rgb linear = xyzToRgb(x / sum, y / sum, z / sum);
    linear.r = std::min(std::max(linear.r, 0.0), 1.0);
    linear.g = std::min(std::max(linear.g, 0.0), 1.0);
    linear.b = std::min(std::max(linear.b, 0.0), 1.0);
    const double peak = std::max(linear.r, std::max(linear.g, linear.b));
    if (peak > 0.0)
    {
        linear.r /= peak;
        linear.g /= peak;
        linear.b /= peak;
    }
    return linear;
}

//--------------------------------------------------------------------------
std::uint8_t channel(double p_value)
{
    return static_cast<std::uint8_t>(std::round(p_value * 255.0));
}

//--------------------------------------------------------------------------
//! \brief Round at the core, then the eccentricity of the disk. Past the disk
//! it eases back to a circle. A negative radius, which the spread can produce,
//! is left circular: that is what the original falls through to.
//--------------------------------------------------------------------------
float eccentricity(float p_radius)
{
    if (p_radius < CORE_RADIUS)
    {
        return 1.0f + ((p_radius / CORE_RADIUS) * (INNER_ECCENTRICITY - 1.0f));
    }
    if ((p_radius > CORE_RADIUS) && (p_radius <= GALAXY_RADIUS))
    {
        const float span = GALAXY_RADIUS - CORE_RADIUS;
        const float rise = OUTER_ECCENTRICITY - INNER_ECCENTRICITY;
        return INNER_ECCENTRICITY + (((p_radius - CORE_RADIUS) / span) * rise);
    }
    const float distant = GALAXY_RADIUS * 2.0f;
    if ((p_radius > GALAXY_RADIUS) && (p_radius < distant))
    {
        const float span = distant - GALAXY_RADIUS;
        const float rise = 1.0f - OUTER_ECCENTRICITY;
        return OUTER_ECCENTRICITY +
               (((p_radius - GALAXY_RADIUS) / span) * rise);
    }
    return 1.0f;
}

} // namespace

//------------------------------------------------------------------------------
std::string SpiralGalaxy::description() const
{
    return "The Glumpy spiral. Stars, dust and hydrogen clouds ride ellipses "
           "whose tilt depends on radius: a density wave, not gravity. "
           "Temperature is a coordinate in a one dimensional blackbody ramp, "
           "and each point is a soft sprite added into the dark. Position and "
           "size are written every frame; draw sends what changed.";
}

//------------------------------------------------------------------------------
void SpiralGalaxy::seed()
{
    const std::size_t count = static_cast<std::size_t>(STARS) + DUST_COUNT +
                              (static_cast<std::size_t>(CLOUDS) * 2u);
    m_bodies.assign(count, Body{});
    m_positions.resize(count);
    m_sizes.resize(count);
    m_clouds = static_cast<std::size_t>(STARS) + DUST_COUNT;
    m_cloud_count = CLOUDS;

    std::mt19937 rng(1u);
    std::normal_distribution<float> spread(0.0f, 0.5f);
    std::uniform_real_distribution<float> turn(0.0f, 360.0f);
    std::uniform_real_distribution<float> heat(3000.0f, 9000.0f);
    std::uniform_real_distribution<float> glow(0.05f, 0.25f);
    std::uniform_real_distribution<float> across(0.0f, 2.0f * GALAXY_RADIUS);
    std::uniform_real_distribution<float> along(-GALAXY_RADIUS, GALAXY_RADIUS);
    std::uniform_real_distribution<float> dim(0.01f, 0.02f);
    std::uniform_real_distribution<float> box(-GALAXY_RADIUS, GALAXY_RADIUS);
    std::uniform_real_distribution<float> faint(0.005f, 0.010f);

    for (std::uint32_t i = 0u; i < STARS; ++i)
    {
        Body& body = m_bodies[i];
        const float radius = spread(rng) * GALAXY_RADIUS;
        body.semi_major = radius;
        body.semi_minor = radius * eccentricity(radius);
        body.angle = 90.0f - (radius * ANGLE_OFFSET);
        body.theta = turn(rng);
        body.temperature = heat(rng);
        body.brightness = glow(rng);
        body.velocity = SPIN;
        body.size = STAR_SIZE;
        body.kind = STAR;
    }

    for (std::uint32_t i = 0u; i < DUST_COUNT; ++i)
    {
        Body& body = m_bodies[static_cast<std::size_t>(STARS) + i];
        const float x = across(rng);
        const float y = along(rng);
        const float radius = std::sqrt((x * x) + (y * y));
        body.semi_major = radius;
        body.semi_minor = radius * eccentricity(radius);
        body.angle = radius * ANGLE_OFFSET;
        body.theta = turn(rng);
        body.temperature = 6000.0f + (radius / 4.0f);
        body.brightness = dim(rng);
        body.velocity = SPIN;
        body.size = DUST_SIZE;
        body.kind = DUST;
    }

    for (std::uint32_t i = 0u; i < CLOUDS; ++i)
    {
        const float x = box(rng);
        const float y = box(rng);
        const float radius = std::sqrt((x * x) + (y * y));
        const float minor = radius * eccentricity(radius);
        const float angle = radius * ANGLE_OFFSET;
        const float theta = turn(rng);
        const float temperature = heat(rng);
        const float brightness = faint(rng);

        Body& cloud = m_bodies[m_clouds + i];
        Body& core = m_bodies[m_clouds + m_cloud_count + i];
        cloud.semi_major = radius;
        core.semi_major = radius + 1000.0f;
        cloud.semi_minor = minor;
        core.semi_minor = minor;
        cloud.angle = angle;
        core.angle = angle;
        cloud.theta = theta;
        core.theta = theta;
        cloud.temperature = temperature;
        core.temperature = temperature;
        cloud.brightness = brightness;
        core.brightness = brightness;
        cloud.velocity = SPIN;
        core.velocity = SPIN;
        cloud.kind = CLOUD;
        core.kind = CORE;
    }
}

//------------------------------------------------------------------------------
void SpiralGalaxy::step()
{
    constexpr float to_radian = 0.017453292f;
    for (Body& body : m_bodies)
    {
        body.theta =
            std::fmod(body.theta + (body.velocity * YEARS_PER_FRAME), 360.0f);
        const float alpha = body.theta * to_radian;
        // Not converted: see ANGLE_OFFSET.
        const float beta = -body.angle;
        const float cos_alpha = std::cos(alpha);
        const float sin_alpha = std::sin(alpha);
        const float cos_beta = std::cos(beta);
        const float sin_beta = std::sin(beta);
        body.x = (body.semi_major * cos_alpha * cos_beta) -
                 (body.semi_minor * sin_alpha * sin_beta);
        body.y = (body.semi_major * cos_alpha * sin_beta) +
                 (body.semi_minor * sin_alpha * cos_beta);
    }

    // A hydrogen region is a pair. When the two draw close, the cloud swells
    // and the core lights up; far apart, both drop under the size cutoff.
    for (std::size_t i = 0u; i < m_cloud_count; ++i)
    {
        Body& cloud = m_bodies[m_clouds + i];
        Body& core = m_bodies[m_clouds + m_cloud_count + i];
        const float dx = cloud.x - core.x;
        const float dy = cloud.y - core.y;
        const float distance = std::sqrt((dx * dx) + (dy * dy));
        const float span =
            std::max(1.0f, ((1000.0f - distance) / 10.0f) - 50.0f);
        cloud.size = 2.0f * span;
        core.size = span / 6.0f;
    }
}

//------------------------------------------------------------------------------
void SpiralGalaxy::flush(float p_scale)
{
    for (std::size_t i = 0u; i < m_bodies.size(); ++i)
    {
        m_positions[i] = Vector2f(m_bodies[i].x / GALAXY_RADIUS,
                                  m_bodies[i].y / GALAXY_RADIUS);
        m_sizes[i] = m_bodies[i].size * p_scale;
    }
    m_points["position"] = m_positions;
    m_points["size"] = m_sizes;
}

//------------------------------------------------------------------------------
compages::gpu::Status SpiralGalaxy::makeColormap()
{
    compages::gpu::TextureDesc desc;
    desc.kind = compages::gpu::TextureKind::Texture1D;
    desc.format = compages::gpu::PixelFormat::RGBA8;
    desc.width = RAMP;
    desc.filter(compages::gpu::Filter::Linear)
        .wrap(compages::gpu::Wrap::ClampToEdge);
    COMPAGES_TRY(m_colormap.allocate(desc));

    // 256 texels from 1000 K up to, but not including, 10000 K: the same
    // samples as the Glumpy loop. Clamp on the texture covers the dust, which
    // runs hotter than the last texel.
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(RAMP) * 4u);
    constexpr double span =
        static_cast<double>(TEMPERATURE_MAX - TEMPERATURE_MIN);
    constexpr double step = span / static_cast<double>(RAMP);
    for (std::uint32_t i = 0u; i < RAMP; ++i)
    {
        const Rgb rgb = spectrum(static_cast<double>(TEMPERATURE_MIN) +
                                 (static_cast<double>(i) * step));
        const std::size_t at = static_cast<std::size_t>(i) * 4u;
        pixels[at] = channel(rgb.r);
        pixels[at + 1u] = channel(rgb.g);
        pixels[at + 2u] = channel(rgb.b);
        pixels[at + 3u] = 255u;
    }
    return m_colormap.write(
        std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

//------------------------------------------------------------------------------
compages::gpu::Status SpiralGalaxy::makeSprite()
{
    COMPAGES_TRY(
        m_sprite.allocate(compages::gpu::TextureDesc::image(
                              SPRITE, SPRITE, compages::gpu::PixelFormat::R8)
                              .filter(compages::gpu::Filter::Linear)
                              .wrap(compages::gpu::Wrap::ClampToEdge)));

    // A stand-in for Glumpy's particle.png: bright in the middle, gone at the
    // edge, so an additive point reads as a glow rather than a square.
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(SPRITE) * SPRITE);
    for (std::uint32_t y = 0u; y < SPRITE; ++y)
    {
        for (std::uint32_t x = 0u; x < SPRITE; ++x)
        {
            const float u =
                ((static_cast<float>(x) + 0.5f) / static_cast<float>(SPRITE)) -
                0.5f;
            const float v =
                ((static_cast<float>(y) + 0.5f) / static_cast<float>(SPRITE)) -
                0.5f;
            const float falloff = std::exp(-22.0f * ((u * u) + (v * v)));
            pixels[(static_cast<std::size_t>(y) * SPRITE) + x] =
                static_cast<std::uint8_t>(std::round(falloff * 255.0f));
        }
    }
    return m_sprite.write(std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

//------------------------------------------------------------------------------
compages::gpu::Status SpiralGalaxy::setUp()
{
    seed();
    COMPAGES_TRY(m_points.load(VERTEX, FRAGMENT));
    COMPAGES_TRY(makeColormap());
    COMPAGES_TRY(makeSprite());

    std::vector<float> temperature(m_bodies.size());
    std::vector<float> brightness(m_bodies.size());
    std::vector<float> kind(m_bodies.size());
    constexpr float span = TEMPERATURE_MAX - TEMPERATURE_MIN;
    for (std::size_t i = 0u; i < m_bodies.size(); ++i)
    {
        temperature[i] = (m_bodies[i].temperature - TEMPERATURE_MIN) / span;
        brightness[i] = m_bodies[i].brightness;
        kind[i] = m_bodies[i].kind;
    }
    m_points["temperature"] = temperature;
    m_points["brightness"] = brightness;
    m_points["kind"] = kind;

    step();
    flush(1.0f);

    m_points["colormap"] = m_colormap;
    m_points["sprite"] = m_sprite;
    m_points["model"] = Matrix44f(compages::matrix::Identity);
    // Five units out on +Z: the Glumpy view, translated by (0, 0, -5). The
    // disk lies in z = 0, so the camera looks straight at it.
    m_points["view"] = compages::matrix::lookAt(Vector3f(0.0f, 0.0f, 5.0f),
                                                Vector3f(0.0f, 0.0f, 0.0f),
                                                Vector3f(0.0f, 1.0f, 0.0f));
    m_points.primitive(compages::gpu::Primitive::Points)
        .blend(compages::gpu::Blend::additive());
    return m_points.prepare();
}

//------------------------------------------------------------------------------
void SpiralGalaxy::draw(Frame const& p_frame)
{
    step();
    // Authored in pixels for an 800 pixel frame, as in the Glumpy example.
    // Growing them with a larger window piles the dust sprites into a white
    // fog. Shrinking them crosses the shader cutoff (size 2 or less is
    // dropped) and deletes the stars. They stay as written.
    flush(1.0f);

    m_points["projection"] =
        compages::matrix::perspective(45.0_deg, aspect(p_frame), 1.0f, 1000.0f);

    compages::gpu::clear({ 0.0f, 0.0f, 0.03f });
    m_points.draw();
}

} // namespace examples
