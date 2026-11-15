// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_Workshop/Workshop/Workshop.hpp"

#include "Compages/Renderer/Procedural/ShaderLib.hpp"

namespace examples::workshop
{

namespace
{

// The hook "surface": it receives the surface as the shape describes it (its
// colour, its normal...) and changes what it wants. Here: black stripes
// across the colour of the shape, like the warning paint of a machine, and a
// paint a little shiny. The Renderer then lights it as any other surface.
constexpr char const* STRIPES = R"(
void surface(inout Surface s)
{
    float stripe = step(0.5, fract((vUv.x + vUv.y) * 4.0));
    s.albedo = mix(s.albedo, vec3(0.03), stripe);
    s.metallic = 0.2;
    s.roughness = 0.35;
}
)";

} // namespace

// ----------------------------------------------------------------------------
compages::Status paintArm(compages::renderer::Scene& p_scene, Arm const& p_arm)
{
    compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
    shaders.fragment().hook("surface", STRIPES);
    compages::renderer::MaterialId paint;
    COMPAGES_TRY_ASSIGN(paint,
                        p_scene.assets().createMaterial(
                            "Warning paint", { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                               .shaders = std::move(shaders),
                                               .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                               .see_through = std::nullopt }));
    // The two links of the arm wear it; each keeps its own colour.
    for (std::size_t k = 1u; k < p_arm.links.size(); ++k)
    {
        p_scene.look(p_arm.links[k], paint);
    }
    return compages::success();
}

} // namespace examples::workshop
