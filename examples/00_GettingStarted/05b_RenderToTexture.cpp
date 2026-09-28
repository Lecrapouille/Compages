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

#include "00_GettingStarted/05b_RenderToTexture.hpp"

#include "Common/ColoredCube.hpp"
#include "Compages/Core/Transformation.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

constexpr std::uint32_t TARGET = 512u;
constexpr float H = 0.6f;

constexpr const char* SCENE_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vColor;

void main()
{
    vNormal = mat3(model) * normal;
    vColor = color;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* SCENE_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vColor;
out vec4 oColor;

void main()
{
    const vec3 toLight = normalize(vec3(0.4, 0.8, 0.6));
    float lit = 0.25 + (0.75 * max(dot(normalize(vNormal), toLight), 0.0));
    oColor = vec4(vColor * lit, 1.0);
}
)";

//! \brief A triangle covering the target, built from the vertex index. The
//! texture coordinate is the clip position mapped into [0, 1], so sampling the
//! offscreen picture stretches it over the pass.
constexpr const char* SCREEN_VERTEX = R"(#version 450 core
out vec2 vUV;

void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 position = corners[gl_VertexID];
    vUV = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* BLIT_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    oColor = texture(image, vUV);
}
)";

//! \brief The same picture, read three times a few pixels apart. That is what
//! makes the second pass a treatment rather than a copy: the cube is never
//! drawn here, only the texture it left behind.
constexpr const char* PROCESS_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    // Red from the right, blue from the left, green from this pixel.
    vec2 shift = vec2(0.008, 0.0);
    float red = texture(image, vUV + shift).r;
    float green = texture(image, vUV).g;
    float blue = texture(image, vUV - shift).b;
    oColor = vec4(red, green, blue, 1.0);
}
)";

//------------------------------------------------------------------------------
std::string RenderToTexture::description() const
{
    return "The cube is drawn into a framebuffer, never into the window. The "
           "left half shows that picture as it is; the right half shows the "
           "same texture through a fullscreen effect. The offscreen target is "
           "512 pixels on a side whatever the window does, and destroying the "
           "framebuffer would not destroy the textures: they are named, not "
           "owned.";
}

//------------------------------------------------------------------------------
compages::gpu::Status RenderToTexture::makeCube()
{
    COMPAGES_TRY(m_cube.load(SCENE_VERTEX, SCENE_FRAGMENT));

    // The cube of 04_DepthAndTransforms, from Common/ColoredCube.hpp.
    const CubeMesh cube = createCube(2.0f * H);
    m_cube.vertices(cube.vertices);
    m_cube.indices(cube.indices);
    m_cube.depthTest().cull(compages::gpu::CullMode::Back);
    return compages::gpu::success();
}

//------------------------------------------------------------------------------
compages::gpu::Status RenderToTexture::makeTarget()
{
    // A size of its own, whatever the window does.
    COMPAGES_TRY(m_color.allocate({ .format = compages::gpu::PixelFormat::RGBA8,
                                    .width = TARGET,
                                    .height = TARGET }));
    COMPAGES_TRY(m_depth.allocate({ .format = compages::gpu::PixelFormat::Depth32F,
                                    .width = TARGET,
                                    .height = TARGET,
                                    .magnify = compages::gpu::Filter::Nearest,
                                    .minify = compages::gpu::Filter::Nearest }));
    return m_target.attach(m_color, m_depth);
}

//------------------------------------------------------------------------------
compages::gpu::Status RenderToTexture::setUp()
{
    COMPAGES_TRY(makeCube());
    COMPAGES_TRY(makeTarget());

    // Two screen passes over the same colour target: a copy, and the
    // split-colour effect.
    COMPAGES_TRY(m_blit.load(SCREEN_VERTEX, BLIT_FRAGMENT));
    COMPAGES_TRY(m_process.load(SCREEN_VERTEX, PROCESS_FRAGMENT));
    m_blit["image"] = m_color;
    m_process["image"] = m_color;
    return compages::gpu::success();
}

//------------------------------------------------------------------------------
void RenderToTexture::draw(Frame const& p_frame)
{
    const Matrix44f identity(compages::matrix::Identity);
    m_cube["model"] = compages::matrix::rotate(identity,
                                     units::angle::radian_t(p_frame.total * 0.7f),
                                     Vector3f(0.3f, 1.0f, 0.2f));
    m_cube["view"] = compages::matrix::lookAt(Vector3f(0.0f, 0.0f, 3.0f),
                                    Vector3f(0.0f, 0.0f, 0.0f),
                                    Vector3f(0.0f, 1.0f, 0.0f));
    // The aspect of the offscreen picture, not of the window. Stretching is
    // what the screen passes do, on purpose.
    m_cube["projection"] = compages::matrix::perspective(60.0_deg, 1.0f, 0.1f, 20.0f);

    // Into the texture. The pass covers the whole framebuffer and suspends
    // the window pass until the end of the scope.
    {
        compages::gpu::RenderPass offscreen(m_target, { .color = { 0.07f, 0.07f, 0.1f, 1.0f } });
        m_cube.draw();
    }

    // Back on the window: the same texture, as it is on the left, treated on
    // the right. The cube is never drawn here.
    const std::uint32_t half = p_frame.width / 2u;
    {
        compages::gpu::RenderPass left({ .width = half, .height = p_frame.height });
        m_blit.draw(3u);
    }
    {
        compages::gpu::RenderPass right({ .x = half,
                                .width = p_frame.width - half,
                                .height = p_frame.height });
        m_process.draw(3u);
    }
}

} // namespace examples
