// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "05_Simulation/05e_SpiralGalaxy.hpp"
#include "Compages/Core/Transformation.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using namespace units::literals;

namespace examples
{

namespace
{

//! \brief Size of the disk, in parsecs. The shader divides by it, so the disk
//! is about one unit across.
constexpr float GALAXY_RADIUS = 13000.0f;
constexpr float CORE_RADIUS = 4000.0f;
//! \brief Extra tilt per parsec. Read by the cosine as radians, although the
//! orbital angle is in degrees: Glumpy does this, and the arms depend on it.
constexpr float TILT_PER_PARSEC = 0.0004f;
constexpr float ECCENTRICITY = 0.90f;
//! \brief Degrees per year, and years per second of the demo.
constexpr float SPIN = 0.000005f;
constexpr float YEARS_PER_SECOND = 6.0e6f;
//! \brief One turn, in years: the time wraps there, the formula does not see
//! the difference.
constexpr float YEARS_PER_TURN = 360.0f / SPIN;

constexpr float STAR = 0.0f;
constexpr float DUST = 1.0f;
constexpr float CLOUD = 2.0f;
constexpr float CORE = 3.0f;

constexpr std::uint32_t STARS = 35000u;
constexpr std::uint32_t DUST_COUNT = STARS * 3u / 4u;
constexpr std::uint32_t CLOUDS = 200u;
constexpr std::uint32_t SPRITE = 64u;

// ----------------------------------------------------------------------------
//! The whole animation is orbit(): the position on the ellipse at this year.
//! A hydrogen cloud and its core ride two ellipses of the same shape; the
//! shader computes the other one of the pair too, to know how close they are.
constexpr char const* VERTEX = R"(#version 450 core
in float semi_major;
in float semi_minor;
in float tilt;
in float theta;
in float size;
in float kind;
in float temperature;
in float brightness;
in float partner;

uniform float years;
uniform float spin;
uniform float radius;
uniform mat4 view;
uniform mat4 projection;
uniform sampler1D colormap;

out vec3 vColor;

vec2 orbit(float a, float b)
{
    float alpha = radians(mod(theta + spin * years, 360.0));
    float beta = -tilt;
    return vec2(a * cos(alpha) * cos(beta) - b * sin(alpha) * sin(beta),
                a * cos(alpha) * sin(beta) + b * sin(alpha) * cos(beta));
}

void main()
{
    vec2 position = orbit(semi_major, semi_minor);

    // A hydrogen region shines when its cloud and its core draw close.
    float shown = size;
    if (kind >= 2.0)
    {
        float apart = distance(position, orbit(semi_major + partner, semi_minor));
        float span = max(1.0, (1000.0 - apart) / 10.0 - 50.0);
        shown = (kind == 2.0) ? 2.0 * span : span / 6.0;
    }

    vColor = texture(colormap, temperature).rgb * brightness;
    if (kind == 2.0)
    {
        vColor *= vec3(2.0, 1.0, 1.0);   // the cloud, pulled toward red
    }
    else if (kind == 3.0)
    {
        vColor = vec3(0.9);              // its core, white
    }

    // A point size of zero is undefined: a body meant to vanish is sent
    // outside the clip volume instead.
    if (shown > 2.0)
    {
        gl_PointSize = shown;
        gl_Position = projection * view * vec4(position / radius, 0.0, 1.0);
    }
    else
    {
        gl_PointSize = 1.0;
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }
}
)";

// ----------------------------------------------------------------------------
constexpr char const* FRAGMENT = R"(#version 450 core
in vec3 vColor;
uniform sampler2D sprite;
out vec4 oColor;

void main()
{
    // A soft disc: with additive blending, a black texel adds nothing.
    oColor = vec4(texture(sprite, gl_PointCoord).r * vColor, 1.0);
}
)";

// ----------------------------------------------------------------------------
//! \brief Colour of a black body from 1000 K to 10000 K, one entry per
//! 1000 K, brightest channel at 255 (John Walker's spectrum rendering, public
//! domain, as Glumpy uses). The texture blends between them.
// ----------------------------------------------------------------------------
constexpr std::uint8_t BLACKBODY[][4] = {
    { 255, 0, 0, 255 },     { 255, 65, 3, 255 },    { 255, 116, 38, 255 },
    { 255, 162, 94, 255 },  { 255, 198, 158, 255 }, { 255, 227, 222, 255 },
    { 231, 226, 255, 255 }, { 194, 204, 255, 255 }, { 170, 188, 255, 255 },
    { 154, 177, 255, 255 },
};

// ----------------------------------------------------------------------------
//! \brief Round at the core, flattened to ECCENTRICITY in the disk, round
//! again far outside.
// ----------------------------------------------------------------------------
float eccentricity(float p_radius)
{
    if (p_radius < CORE_RADIUS)
    {
        return 1.0f + ((p_radius / CORE_RADIUS) * (ECCENTRICITY - 1.0f));
    }
    if (p_radius <= GALAXY_RADIUS)
    {
        return ECCENTRICITY;
    }
    if (p_radius < 2.0f * GALAXY_RADIUS)
    {
        return ECCENTRICITY + (((p_radius - GALAXY_RADIUS) / GALAXY_RADIUS) *
                               (1.0f - ECCENTRICITY));
    }
    return 1.0f;
}

// ----------------------------------------------------------------------------
float coordinate(float p_kelvin)
{
    return (p_kelvin - 1000.0f) / 9000.0f;
}

} // namespace

// ----------------------------------------------------------------------------
std::string SpiralGalaxy::description() const
{
    return "Stars, dust and hydrogen clouds ride ellipses whose tilt grows "
           "with the radius: a density wave, not gravity. The position is a "
           "formula of the time, so the vertex shader computes it: the "
           "vertices are sent once, and each frame writes one number, the "
           "year. No CPU loop, no compute shader. Ported from the galaxy "
           "example of Glumpy (Nicolas P. Rougier), after the Galaxy "
           "Renderer of Ingo Berg.";
}

// ----------------------------------------------------------------------------
std::vector<SpiralGalaxy::Body> SpiralGalaxy::seed()
{
    std::mt19937 rng(1u);
    std::normal_distribution<float> spread(0.0f, 0.5f);
    std::uniform_real_distribution<float> turn(0.0f, 360.0f);
    std::uniform_real_distribution<float> heat(3000.0f, 9000.0f);
    std::uniform_real_distribution<float> glow(0.05f, 0.25f);
    std::uniform_real_distribution<float> across(0.0f, 2.0f * GALAXY_RADIUS);
    std::uniform_real_distribution<float> along(-GALAXY_RADIUS, GALAXY_RADIUS);
    std::uniform_real_distribution<float> dim(0.01f, 0.02f);
    std::uniform_real_distribution<float> faint(0.005f, 0.010f);

    std::vector<Body> bodies;
    bodies.reserve(STARS + DUST_COUNT + (2u * CLOUDS));

    // Stars: bright, small, spread around the centre.
    for (std::uint32_t i = 0u; i < STARS; ++i)
    {
        const float r = spread(rng) * GALAXY_RADIUS;
        bodies.push_back({ .semi_major = r,
                           .semi_minor = r * eccentricity(r),
                           .tilt = 90.0f - (r * TILT_PER_PARSEC),
                           .theta = turn(rng),
                           .size = 3.0f,
                           .kind = STAR,
                           .temperature = coordinate(heat(rng)),
                           .brightness = glow(rng),
                           .partner = 0.0f });
    }

    // Dust: wide, dim sprites that make the glow of the disk.
    for (std::uint32_t i = 0u; i < DUST_COUNT; ++i)
    {
        const float x = across(rng);
        const float y = along(rng);
        const float r = std::sqrt((x * x) + (y * y));
        bodies.push_back({ .semi_major = r,
                           .semi_minor = r * eccentricity(r),
                           .tilt = r * TILT_PER_PARSEC,
                           .theta = turn(rng),
                           .size = 64.0f,
                           .kind = DUST,
                           .temperature = coordinate(6000.0f + (r / 4.0f)),
                           .brightness = dim(rng),
                           .partner = 0.0f });
    }

    // Hydrogen regions: a cloud and its core on two ellipses 1000 pc apart.
    for (std::uint32_t i = 0u; i < CLOUDS; ++i)
    {
        const float x = along(rng);
        const float y = along(rng);
        const float r = std::sqrt((x * x) + (y * y));
        Body cloud{ .semi_major = r,
                    .semi_minor = r * eccentricity(r),
                    .tilt = r * TILT_PER_PARSEC,
                    .theta = turn(rng),
                    .size = 0.0f,
                    .kind = CLOUD,
                    .temperature = coordinate(heat(rng)),
                    .brightness = faint(rng),
                    .partner = 1000.0f };
        Body core = cloud;
        core.semi_major += 1000.0f;
        core.kind = CORE;
        core.partner = -1000.0f;
        bodies.push_back(cloud);
        bodies.push_back(core);
    }
    return bodies;
}

// ----------------------------------------------------------------------------
compages::Status SpiralGalaxy::makeColormap()
{
    constexpr std::uint32_t count = sizeof(BLACKBODY) / sizeof(BLACKBODY[0]);
    compages::gpu::TextureDesc desc;
    desc.kind = compages::gpu::TextureKind::Texture1D;
    desc.format = compages::gpu::PixelFormat::RGBA8;
    desc.width = count;
    desc.filter(compages::gpu::Filter::Linear)
        .wrap(compages::gpu::Wrap::ClampToEdge);
    COMPAGES_TRY_ASSIGN(m_colormap, compages::gpu::Texture::create(desc));
    return m_colormap.write(std::as_bytes(std::span(BLACKBODY)));
}

// ----------------------------------------------------------------------------
compages::Status SpiralGalaxy::makeSprite()
{
    COMPAGES_TRY_ASSIGN(m_sprite,
                        compages::gpu::Texture::create(
                            compages::gpu::TextureDesc::image(
                                SPRITE, SPRITE, compages::gpu::PixelFormat::R8)
                                .filter(compages::gpu::Filter::Linear)
                                .wrap(compages::gpu::Wrap::ClampToEdge)));

    // Bright in the middle, gone at the edge: an additive point reads as a
    // glow rather than a square.
    std::vector<std::uint8_t> pixels(std::size_t{ SPRITE } * SPRITE);
    for (std::uint32_t y = 0u; y < SPRITE; ++y)
    {
        for (std::uint32_t x = 0u; x < SPRITE; ++x)
        {
            const float u = ((float(x) + 0.5f) / float(SPRITE)) - 0.5f;
            const float v = ((float(y) + 0.5f) / float(SPRITE)) - 0.5f;
            const float falloff = std::exp(-22.0f * ((u * u) + (v * v)));
            pixels[(std::size_t{ y } * SPRITE) + x] =
                static_cast<std::uint8_t>(std::round(falloff * 255.0f));
        }
    }
    return m_sprite.write(std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

// ----------------------------------------------------------------------------
compages::Status SpiralGalaxy::setUp()
{
    COMPAGES_TRY(m_points.loadSources(VERTEX, FRAGMENT));
    COMPAGES_TRY(makeColormap());
    COMPAGES_TRY(makeSprite());

    // Sent once: the ellipses never change, only the year does.
    m_points.setVertices(seed());
    m_points["years"] = m_years;
    m_points["spin"] = SPIN;
    m_points["radius"] = GALAXY_RADIUS;
    m_points["colormap"] = m_colormap;
    m_points["sprite"] = m_sprite;
    // The disk lies in z = 0; the camera looks at it from five units away.
    m_points["view"] = compages::lookAt(compages::Vector3f(0.0f, 0.0f, 5.0f),
                                        compages::Vector3f(0.0f, 0.0f, 0.0f),
                                        compages::Vector3f(0.0f, 1.0f, 0.0f));
    m_points.state().primitive = compages::gpu::Primitive::Points;
    m_points.state().blend = compages::gpu::Blend::additive();
    return m_points.prepare();
}

// ----------------------------------------------------------------------------
void SpiralGalaxy::draw(compages::world::ViewFrame const& p_frame)
{
    // The only thing written each frame.
    m_years = std::fmod(m_years + (YEARS_PER_SECOND * p_frame.elapsed),
                        YEARS_PER_TURN);
    m_points["years"] = m_years;
    m_points["projection"] =
        compages::perspective(45.0_deg, aspect(p_frame), 1.0f, 1000.0f);

    compages::gpu::clear({ 0.0f, 0.0f, 0.03f });
    m_points.draw();
}

} // namespace examples
