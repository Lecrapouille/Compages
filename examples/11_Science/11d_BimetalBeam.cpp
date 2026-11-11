// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "11_Science/11d_BimetalBeam.hpp"

#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

//! \brief The strip: 1 m long, 2 + 2 mm thick (drawn 10 times thicker, to be
//! seen), 4 cm wide, cut in 96 slices.
static constexpr int SLICES = 96;
static constexpr float LENGTH = 1.0f;
static constexpr float THICKNESS = 0.004f;
static constexpr float DRAWN_THICKNESS = 0.04f;
static constexpr float WIDTH = 0.04f;
//! \brief Brass grows by 19e-6 of its length per degree, steel by 12e-6.
//! Two layers of the same thickness and stiffness bend by
//! 3/2 (a_brass - a_steel) dT / h (Timoshenko, 1925).
static constexpr float CURVATURE_PER_DEGREE = 1.5f * (19.0e-6f - 12.0e-6f) / THICKNESS;
//! \brief How fast heat spreads in the metal (m^2/s), how fast the air cools it (1/s).
static constexpr float DIFFUSIVITY = 3.0e-5f;
static constexpr float COOLING = 0.01f;
static constexpr float ROOM = 20.0f;
//! \brief The flame heats the first 40 cm.
static constexpr float FLAME = 0.4f;

// ----------------------------------------------------------------------------
//! Each corner of the straight strip (x along it, y across its thickness) is
//! put on the middle line at its slice, turned by the angle of the slice.
//! The varyings tell the fragment its layer and its heat.
// ----------------------------------------------------------------------------
static constexpr char const* BEND = R"(layout(std430) readonly buffer Beam
{
    vec4 slices[];   // x, y of the middle line, angle, temperature
};
uniform int sliceCount;
uniform float beamLength;

out float vHeat;
out float vBrass;

void deform(inout vec4 world, inout vec3 normal)
{
    float s = clamp(world.x / beamLength, 0.0, 1.0) * float(sliceCount - 1);
    int k = min(int(s), sliceCount - 2);
    vec4 slice = mix(slices[k], slices[k + 1], s - float(k));
    vec2 across = vec2(-sin(slice.z), cos(slice.z));
    vBrass = (world.y > 0.0) ? 1.0 : 0.0;
    vHeat = slice.w;
    world.xy = slice.xy + across * world.y;
    normal.xy = vec2(normal.x * cos(slice.z) - normal.y * sin(slice.z),
                     normal.x * sin(slice.z) + normal.y * cos(slice.z));
}
)";

// ----------------------------------------------------------------------------
//! Brass on top, steel under it, both glowing red when very hot.
// ----------------------------------------------------------------------------
static constexpr char const* METAL = R"(in float vHeat;
in float vBrass;

void main()
{
    vec3 metal = mix(vec3(0.45, 0.47, 0.5), vec3(0.75, 0.55, 0.2), vBrass);
    vec3 shaded = shade(metal, normalize(vNormal), vWorldPos);
    float glow = smoothstep(350.0, 800.0, vHeat);
    shaded += 0.6 * glow * mix(vec3(1.0, 0.1, 0.0), vec3(1.0, 0.5, 0.1), glow);
    oColor = vec4(present(applyFog(shaded, vWorldPos)), opacity);
}
)";

// ----------------------------------------------------------------------------
//! \brief A straight strip from x = 0 to LENGTH, its slices as rings of four
//! faces, closed at both ends.
// ----------------------------------------------------------------------------
static compages::renderer::MeshAsset stripMesh()
{
    compages::renderer::MeshAsset mesh;
    const float y = 0.5f * DRAWN_THICKNESS;
    const float z = 0.5f * WIDTH;
    struct Side
    {
        compages::Vector3f normal;
        compages::Vector2f a; // (y, z) of the two edges of the side
        compages::Vector2f b;
    };
    const Side sides[4] = { { { 0, 1, 0 }, { y, z }, { y, -z } },
                            { { 0, -1, 0 }, { -y, -z }, { -y, z } },
                            { { 0, 0, 1 }, { -y, z }, { y, z } },
                            { { 0, 0, -1 }, { y, -z }, { -y, -z } } };
    for (Side const& side : sides)
    {
        const auto first = std::uint32_t(mesh.source_vertices.size());
        for (int k = 0; k < SLICES; ++k)
        {
            const float x = LENGTH * float(k) / float(SLICES - 1);
            for (compages::Vector2f edge : { side.a, side.b })
            {
                compages::renderer::MeshVertex corner;
                corner.position = compages::Vector3f(x, edge.x, edge.y);
                corner.normal = side.normal;
                mesh.source_vertices.push_back(corner);
            }
        }
        for (int k = 0; k + 1 < SLICES; ++k)
        {
            const std::uint32_t a = first + std::uint32_t(2 * k);
            mesh.source_indices.insert(mesh.source_indices.end(),
                                       { a, a + 2u, a + 1u, a + 1u, a + 2u, a + 3u });
        }
    }
    for (const float x : { 0.0f, LENGTH })
    {
        const auto first = std::uint32_t(mesh.source_vertices.size());
        for (compages::Vector2f edge : { compages::Vector2f(-y, -z), compages::Vector2f(-y, z),
                                         compages::Vector2f(y, z), compages::Vector2f(y, -z) })
        {
            compages::renderer::MeshVertex corner;
            corner.position = compages::Vector3f(x, edge.x, edge.y);
            corner.normal = compages::Vector3f((x > 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
            mesh.source_vertices.push_back(corner);
        }
        mesh.source_indices.insert(mesh.source_indices.end(),
                                   { first, first + 1u, first + 2u, first, first + 2u, first + 3u });
    }
    mesh.index_count = mesh.source_indices.size();
    // Room for every bend: the strip may curl a whole turn.
    mesh.local_bounds = compages::AABB::fromCorners(compages::Vector3f(-LENGTH, -LENGTH, -z),
                                                    compages::Vector3f(LENGTH, LENGTH, z));
    return mesh;
}

// ----------------------------------------------------------------------------
std::string BimetalBeam::description() const
{
    return "A model of the CPU bends a mesh on the GPU. A strip of brass on "
           "steel is held at its right end; a flame heats its left end. The "
           "heat runs along the strip (a Field<float>, the heat equation with "
           "the air cooling it); brass grows more than steel, so each slice "
           "bends by 3/2 (a_brass - a_steel) dT / h (Timoshenko). The CPU adds "
           "the bends up into the middle line, a storage buffer that the hook "
           "deform of the PBR shader reads to bend a straight mesh. Try it: "
           "the flame, its temperature, the speed of time.";
}

// ----------------------------------------------------------------------------
compages::Status BimetalBeam::setUp()
{
    const compages::Grid2D line{ .columns = SLICES,
                                 .rows = 1,
                                 .origin = { 0.0f, 0.0f },
                                 .spacing = LENGTH / float(SLICES - 1) };
    m_heat = compages::Field<float>(line, ROOM);
    m_next = m_heat;
    m_slices.assign(std::size_t(SLICES), compages::Vector4f(0.0f, 0.0f, 0.0f, ROOM));
    heat(90.0f); // a minute and a half on the flame before the first picture
    bend();
    COMPAGES_TRY_ASSIGN(m_line,
                        compages::gpu::StorageBuffer<compages::Vector4f>::from(
                            m_slices, compages::gpu::BufferUsage::Stream));

    compages::renderer::shaders::ShaderPair pbr = compages::renderer::shaders::pbr();
    pbr.vertex().hook("deform", BEND);
    pbr.fragment().hook("main", METAL);
    compages::renderer::Material material;
    COMPAGES_TRY_ASSIGN(
        material,
        compages::renderer::Material::create(
            "Bimetal",
            { .family = compages::renderer::ShaderFamily::PbrMinimal,
              .shaders = std::move(pbr),
              .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
              .see_through = std::nullopt }));
    material.program.set("sliceCount", SLICES);
    material.program.set("beamLength", LENGTH);
    material.buffers = { { .block = "Beam", .buffer = m_line.handle() } };
    compages::renderer::MaterialId id;
    COMPAGES_TRY_ASSIGN(id, m_scene.assets().addMaterial("Bimetal", std::move(material)));

    compages::world::Entity strip = m_scene.mesh(stripMesh(), "Strip");
    m_scene.look(strip, id);
    // The hook is not in the shadow pass yet: no shadow of a straight strip.
    strip.get<compages::renderer::MeshRenderer>().flags =
        compages::renderer::RenderFlags::NoShadowCaster;

    // The clamp on the right, the flame on the left.
    m_scene.box("Clamp", compages::renderer::color(0.25f, 0.25f, 0.28f))
        .position(LENGTH + 0.04f, 0.0f, 0.0f)
        .scale(0.1f, 0.12f, 0.1f);
    m_flame = m_scene.cone("Flame", compages::renderer::unlit(1.0f, 0.45f, 0.05f))
                  .position(0.5f * FLAME, -0.12f, 0.0f)
                  .scale(0.3f, 0.12f, 0.08f);
    m_scene.lamp("Flame light", { 1.0f, 0.5f, 0.2f }, 1.5f, 1.0f)
        .position(0.5f * FLAME, -0.1f, 0.0f);

    m_scene.background(0.55f, 0.6f, 0.66f);
    m_scene.camera().position(0.5f, 0.3f, 1.6f).add<compages::world::Orbit>(
        compages::Vector3f(0.5f, 0.0f, 0.0f));
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.1f);
    m_scene.grid(3.0f, 30u).position(0.5f, -0.25f, 0.0f);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! Explicit steps of dT/dt = D d2T/dx2 - cooling (T - room), the flame
//! holding its part at its temperature; the free end and the clamp let no
//! heat through.
// ----------------------------------------------------------------------------
void BimetalBeam::heat(float p_seconds)
{
    const float h = m_heat.grid().spacing;
    const float limit = 0.4f * h * h / DIFFUSIVITY;
    const int steps = std::max(1, int(std::ceil(p_seconds / limit)));
    const float dt = p_seconds / float(steps);
    for (int step = 0; step < steps; ++step)
    {
        for (int i = 0; i < SLICES; ++i)
        {
            const float t = m_heat(i, 0);
            const float left = m_heat(std::max(i - 1, 0), 0);
            const float right = m_heat(std::min(i + 1, SLICES - 1), 0);
            const float x = h * float(i);
            m_next(i, 0) = (m_burning && (x < FLAME))
                               ? m_flame_temperature
                               : t + (dt * ((DIFFUSIVITY * (left + right - (2.0f * t)) / (h * h)) -
                                            (COOLING * (t - ROOM))));
        }
        std::swap(m_heat, m_next);
    }
}

// ----------------------------------------------------------------------------
//! From the clamp (x = LENGTH, flat) toward the free end: each slice turns by
//! its curvature times its length, and the line follows the turn.
// ----------------------------------------------------------------------------
void BimetalBeam::bend()
{
    const float h = m_heat.grid().spacing;
    float angle = 0.0f;
    compages::Vector2f at(LENGTH, 0.0f);
    for (int i = SLICES - 1; i >= 0; --i)
    {
        m_slices[std::size_t(i)] = compages::Vector4f(at.x, at.y, angle, m_heat(i, 0));
        // Brass, on top, grows more: the strip bends down, away from it.
        angle += CURVATURE_PER_DEGREE * (m_heat(i, 0) - ROOM) * h;
        at = at - (h * compages::Vector2f(std::cos(angle), std::sin(angle)));
    }
}

// ----------------------------------------------------------------------------
void BimetalBeam::draw(compages::world::ViewFrame const& p_frame)
{
    heat(m_speed * std::min(p_frame.elapsed, 0.05f));
    bend();
    m_line.write(m_slices);
    m_flame.enable(m_burning);
    m_flame.position(0.5f * FLAME, std::min(-0.12f, m_slices[std::size_t(SLICES / 5)].y - 0.12f), 0.0f);
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void BimetalBeam::controls()
{
    ImGui::Checkbox("Flame", &m_burning);
    ImGui::SliderFloat("Flame (degrees)", &m_flame_temperature, 100.0f, 900.0f, "%.0f");
    ImGui::SliderFloat("Time (s per s)", &m_speed, 0.0f, 100.0f, "%.0f");
    ImGui::Text("Free end: %.0f degrees, %.0f mm down", double(m_heat(0, 0)),
                double(-1000.0f * m_slices.front().y));
}

} // namespace examples
