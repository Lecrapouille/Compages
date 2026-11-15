// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Skybox.hpp"

#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Offer.hpp"

#include <initializer_list>

namespace compages::renderer
{

namespace
{

constexpr char const* SKY_VERTEX = R"(#version 450 core
in vec3 position;
uniform mat4 view;
uniform mat4 projection;
struct V2F
{
    vec3 direction;
};
out V2F v2f;

void main()
{
    v2f.direction = position;
    // The camera turns but never moves away from the sky: the translation
    // of the view is dropped.
    gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0);
}
)";

constexpr char const* SKY_FRAGMENT = R"(#version 450 core
struct V2F
{
    vec3 direction;
};
in V2F v2f;
uniform samplerCube sky;
#include <compages/present.glsl>
out vec4 oColor;

void main()
{
    oColor = vec4(screen(texture(sky, normalize(v2f.direction)).rgb), 1.0);
}
)";

} // namespace

Status Skybox::loadFiles(std::array<std::filesystem::path, 6u> const& p_faces)
{
    compages::gpu::LoadOptions options;
    options.flip_vertically = false;
    options.srgb = true;
    options.wrap = compages::gpu::Wrap::ClampToEdge;
    COMPAGES_TRY_ASSIGN(
        m_texture, compages::gpu::Texture::cubeFromFiles(p_faces, options));
    compages::gpu::ResolvedSource fragment;
    COMPAGES_TRY_ASSIGN(fragment, shaders::compute("sky.frag", SKY_FRAGMENT).resolve());
    COMPAGES_TRY(m_cube.loadSources(SKY_VERTEX, fragment.text));
    // A cube around the camera, seen from the inside. Drawn first and
    // without the depth test, it is behind whatever comes after.
    // The element type is written out: a bare braced list is ambiguous
    // between the int and the float slots under Clang.
    m_cube["position"] = std::initializer_list<std::initializer_list<float>>{
        { -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 1.f, 1.f, -1.f },
        { -1.f, 1.f, -1.f },  { -1.f, -1.f, 1.f }, { 1.f, -1.f, 1.f },
        { 1.f, 1.f, 1.f },    { -1.f, 1.f, 1.f }
    };
    m_cube.indices({ 0, 1, 2, 2, 3, 0, 4, 6, 5, 6, 4, 7, 0, 3, 7, 7, 4, 0,
                     1, 5, 6, 6, 2, 1, 3, 2, 6, 6, 7, 3, 0, 4, 5, 5, 1, 0 });
    m_cube["sky"] = m_texture;
    m_cube.state().depth_test = false;
    return success();
}

void Skybox::draw(RenderSnapshot const& p_view)
{
    if (!m_texture.valid())
    {
        return;
    }
    m_cube["view"] = p_view.camera.view;
    m_cube["projection"] = p_view.camera.reversed_projection;
    offer(m_cube.program(), "toneMapping", static_cast<int>(p_view.environment.tone_mapping));
    offer(m_cube.program(), "linearTarget", p_view.linear_target);
    m_cube.draw();
}

} // namespace compages::renderer
