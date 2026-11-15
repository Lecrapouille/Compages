// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Workshop.hpp"

#include "Compages/Renderer/Procedural/ShaderLib.hpp"

namespace examples::workshop
{

namespace
{

// The hook "surface" receives the surface as the shape describes it (its
// colour, its normal...) and changes what it wants. Here: the paint of a
// machine, worn down to the steel along the edges of each face, where tools
// and hands rub it. v2f.uv goes from 0 to 1 across each face of a box, so the
// distance to the nearest edge of the face is the smallest of four numbers.
// Only the colour changes: s.metallic and s.roughness count only for an
// instance that uses the metallic-roughness model of glTF.
constexpr char const* WORN_PAINT = R"(
void surface(inout Surface s)
{
    float edge = min(min(v2f.uv.x, 1.0 - v2f.uv.x), min(v2f.uv.y, 1.0 - v2f.uv.y));
    float paint = smoothstep(0.02, 0.08, edge);
    s.albedo = mix(vec3(0.55, 0.56, 0.58), s.albedo, paint);
}
)";

} // namespace

// ----------------------------------------------------------------------------
compages::Status paintArm(compages::renderer::Scene& p_scene, std::vector<compages::world::Entity> const& p_links)
{
    compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
    shaders.fragment().hook("surface", WORN_PAINT);
    compages::renderer::MaterialId paint;
    COMPAGES_TRY_ASSIGN(paint,
                        p_scene.assets().createMaterial(
                            "Worn paint", { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                            .shaders = std::move(shaders),
                                            .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                            .see_through = std::nullopt }));
    // Each link keeps its own colour: it arrives in s.albedo.
    for (compages::world::Entity const& link : p_links)
    {
        p_scene.look(link, paint);
    }
    return compages::success();
}

} // namespace examples::workshop
