// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06o_CelShading.hpp"

#include "Common/Gui.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

// The hook "light": the sun in a few flat tones instead of a smooth slope,
// and a hard white glint. The surface (colour, normal) is the instance's;
// the fog and the tone curve are still added after.
static constexpr char const* BANDS = R"(
uniform float tones;

vec3 light(Surface s)
{
    vec3 toSun = normalize(-lightDir);
    vec3 toEye = normalize(cameraPos - vWorldPos);
    float sun = max(dot(s.normal, toSun), 0.0) * sunVisibility(s.normal, toSun, vWorldPos);
    float band = ceil(sun * tones) / tones;
    float glint = pow(max(dot(s.normal, normalize(toSun + toEye)), 0.0), 80.0);
    return s.albedo * (ambient + lightColor * band) + lightColor * step(0.5, glint) * step(0.0, sun - 0.01);
}
)";

// The outline: a shell a little larger than the shape, drawn inside out
// (its front faces are culled) in one dark colour. Only its rim shows
// round the shape.
static constexpr char const* INK = R"(
void main()
{
    oColor = vec4(0.03, 0.03, 0.05, 1.0);
}
)";

// ----------------------------------------------------------------------------
std::string CelShading::description() const
{
    return "Another model of light, in a few lines of GLSL: the hook \"light\" "
           "of the PBR shader returns the light on a surface. Here the sun "
           "falls in flat tones, as in a cartoon (cel shading), with a hard "
           "glint; the colour of each shape, its shadows and the fog stay "
           "what the Scene gives. The black outline is a shell of each shape, "
           "a little larger and drawn inside out. Try it: the cartoon against "
           "the light of the Scene, the number of tones, the outline.";
}

// ----------------------------------------------------------------------------
compages::Status CelShading::makeMaterials()
{
    compages::renderer::shaders::ShaderPair cel = compages::renderer::shaders::pbr();
    cel.fragment().hook("light", BANDS);
    const compages::gpu::RenderState solid{ .depth_test = true, .cull = compages::gpu::CullMode::Back };
    COMPAGES_TRY_ASSIGN(m_cel,
                        m_scene.assets().createMaterial("Cel",
                                                        { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                                          .shaders = std::move(cel),
                                                          .state = solid,
                                                          .see_through = std::nullopt }));
    COMPAGES_TRY_ASSIGN(m_plain,
                        m_scene.assets().createMaterial("Plain",
                                                        { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                                          .shaders = compages::renderer::shaders::pbr(),
                                                          .state = solid,
                                                          .see_through = std::nullopt }));
    compages::renderer::shaders::ShaderPair ink = compages::renderer::shaders::pbr();
    ink.fragment().hook("main", INK);
    COMPAGES_TRY_ASSIGN(
        m_outline,
        m_scene.assets().createMaterial("Outline",
                                        { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                          .shaders = std::move(ink),
                                          .state = { .depth_test = true, .cull = compages::gpu::CullMode::Front },
                                          .see_through = std::nullopt }));
    return compages::success();
}

// ----------------------------------------------------------------------------
void CelShading::add(compages::renderer::Shape p_shape, compages::Vector3f p_color, compages::Vector3f p_position)
{
    compages::world::Entity shape =
        m_scene.shape(p_shape, "Shape", compages::renderer::color(p_color.x, p_color.y, p_color.z))
            .position(p_position.x, p_position.y, p_position.z)
            .scale(1.4f);
    m_scene.look(shape, m_cel);
    compages::world::Entity shell = m_scene.shape(p_shape, "Outline").parent(shape).scale(1.07f);
    m_scene.look(shell, m_outline);
    shell.get<compages::renderer::MeshRenderer>().flags = compages::renderer::RenderFlags::NoShadowCaster;
    m_shapes.push_back(shape);
    m_shells.push_back(shell);
    m_inks.push_back(shell.get<compages::renderer::MeshRenderer>());
}

// ----------------------------------------------------------------------------
compages::Status CelShading::setUp()
{
    COMPAGES_TRY(makeMaterials());
    m_scene.background(0.55f, 0.75f, 0.95f);
    m_scene.ambient(0.25f, 0.27f, 0.32f);
    m_scene.sun("Sun", compages::Vector3f(1.0f, 0.97f, 0.9f), 1.2f).position(5.0f, 10.0f, 6.0f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene.camera().position(0.0f, 4.0f, 9.0f).add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.8f, 0.0f));

    compages::world::Entity floor =
        m_scene.box("Floor", compages::renderer::color(0.45f, 0.7f, 0.35f)).position(0.0f, -0.1f, 0.0f).scale(14.0f, 0.2f, 14.0f);
    m_scene.look(floor, m_cel);
    m_shapes.push_back(floor);

    using compages::renderer::Shape;
    add(Shape::Sphere, { 0.9f, 0.25f, 0.2f }, { -3.0f, 0.7f, 0.0f });
    add(Shape::Box, { 0.95f, 0.75f, 0.2f }, { -1.0f, 0.7f, 1.2f });
    add(Shape::Cylinder, { 0.25f, 0.5f, 0.9f }, { 1.0f, 0.7f, 0.0f });
    add(Shape::Cone, { 0.7f, 0.35f, 0.85f }, { 3.0f, 0.7f, 1.0f });
    add(Shape::Sphere, { 0.95f, 0.95f, 0.95f }, { 0.5f, 0.7f, -2.5f });
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void CelShading::useCartoon(bool p_cartoon)
{
    for (compages::world::Entity& shape : m_shapes)
    {
        m_scene.look(shape, p_cartoon ? m_cel : m_plain);
    }
    // No outline on the light of the Scene: the shells stop being drawn.
    for (std::size_t i = 0; i < m_shells.size(); ++i)
    {
        if (p_cartoon)
        {
            m_shells[i].set(m_inks[i]);
        }
        else
        {
            m_shells[i].remove<compages::renderer::MeshRenderer>();
        }
    }
}

// ----------------------------------------------------------------------------
void CelShading::draw(compages::world::ViewFrame const& p_frame)
{
    m_time += std::min(p_frame.elapsed, 0.1f);
    for (std::size_t i = 0; i < m_shells.size(); ++i)
    {
        m_shapes[i + 1].rotation(0.5f * m_time + float(i), compages::Vector3f(0.0f, 1.0f, 0.0f));
    }
    m_scene.assets().material(m_cel)->program.set("tones", float(m_tones));
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void CelShading::controls()
{
    if (ImGui::Checkbox("Cartoon (off: the light of the Scene)", &m_cartoon))
    {
        useCartoon(m_cartoon);
    }
    ImGui::SliderInt("Tones", &m_tones, 2, 6);
}

} // namespace examples
