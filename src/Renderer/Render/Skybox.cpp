// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Skybox.hpp"

#include <initializer_list>

namespace compages::renderer
{

namespace
{

constexpr char const* SKY_VERTEX = R"(#version 450 core
in vec3 position;
uniform mat4 view;
uniform mat4 projection;
out vec3 vDirection;

void main()
{
    vDirection = position;
    // The camera turns but never moves away from the sky: the translation
    // of the view is dropped.
    gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0);
}
)";

constexpr char const* SKY_FRAGMENT = R"(#version 450 core
in vec3 vDirection;
uniform samplerCube sky;
out vec4 oColor;

void main()
{
    oColor = texture(sky, normalize(vDirection));
}
)";

} // namespace

Status Skybox::loadFiles(std::array<std::filesystem::path, 6u> const& p_faces)
{
    compages::gpu::LoadOptions options;
    options.flip_vertically = false;
    options.srgb = true;
    options.wrap = compages::gpu::Wrap::ClampToEdge;
    COMPAGES_TRY(m_texture.loadCubeFiles(p_faces, options));
    COMPAGES_TRY(m_cube.loadSources(SKY_VERTEX, SKY_FRAGMENT));
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

void Skybox::draw(CameraFrame const& p_camera)
{
    if (!m_texture.valid())
    {
        return;
    }
    m_cube["view"] = p_camera.view;
    m_cube["projection"] = p_camera.projection;
    m_cube.draw();
}

} // namespace compages::renderer
