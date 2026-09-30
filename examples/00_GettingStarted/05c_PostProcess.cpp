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

#include "00_GettingStarted/05c_PostProcess.hpp"

#include "Common/DataPath.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"

#include <array>

using namespace units::literals;

namespace examples
{

constexpr char const* SCENE_VS = R"(#version 450 core
in vec3 position;
in vec2 uv;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 vUV;
void main()
{
    vUV = uv;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr char const* SCENE_FS = R"(#version 450 core
in vec2 vUV;
uniform sampler2D texID;
out vec4 oColor;
void main()
{
    oColor = texture(texID, vUV);
}
)";

constexpr char const* SCREEN_VS = R"(#version 450 core
in vec2 position;
in vec2 uv;
out vec2 vUV;
void main()
{
    vUV = uv;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr char const* SCREEN_FS = R"(#version 450 core
in vec2 vUV;
uniform sampler2D texID;
uniform float time;
uniform float screen_width;
uniform float screen_height;
out vec4 oColor;
void main()
{
    // The sample is shifted by a sine of time and of the pixel, which is
    // the wave. The scene itself is already in the texture.
    oColor = vec4(texture(texID, vUV +
                          0.005 * vec2(sin(time + screen_width * vUV.x),
                                       cos(time + screen_height * vUV.y))).xyz,
                  1.0);
}
)";

//------------------------------------------------------------------------------
static compages::Status loadTexture(char const* p_file,
                                    compages::gpu::Texture& p_out)
{
    // The pictures live in Compages-data, not next to the source.
    const std::string path = dataPath(p_file);
    if (path.empty())
    {
        return compages::failure(std::string("missing texture: ") + p_file);
    }
    // sRGB so the sampled colour is linear before the wavy pass.
    compages::gpu::LoadOptions options;
    options.srgb = true;
    return p_out.load(path, options);
}

//------------------------------------------------------------------------------
static compages::renderer::MeshVertex
meshVertex(compages::core::Vector3f p_position, compages::core::Vector2f p_uv)
{
    return compages::renderer::MeshVertex{
        p_position, compages::core::Vector3f(0.0f, 1.0f, 0.0f), p_uv
    };
}

//------------------------------------------------------------------------------
std::string PostProcess::description() const
{
    return "Legacy 13_PostProdFrameBuffer: textured cube and floor rendered "
           "offscreen with depth, then a wavy fullscreen post pass.";
}

//------------------------------------------------------------------------------
compages::Status PostProcess::ensureTarget(std::uint32_t p_width,
                                           std::uint32_t p_height)
{
    if ((p_width == m_target_width) && (p_height == m_target_height) &&
        m_fbo.valid())
    {
        return compages::success();
    }

    // Allocated again in place: whatever samples these textures still does.
    COMPAGES_TRY(
        m_color_target.allocate({ .format = compages::gpu::PixelFormat::RGBA8,
                                  .width = p_width,
                                  .height = p_height }));
    COMPAGES_TRY(m_depth_target.allocate(
        { .format = compages::gpu::PixelFormat::Depth32F,
          .width = p_width,
          .height = p_height }));
    COMPAGES_TRY(m_fbo.attach(m_color_target, m_depth_target));
    m_target_width = p_width;
    m_target_height = p_height;
    return compages::success();
}

compages::Status PostProcess::setUp()
{
    COMPAGES_TRY(loadTexture("wooden-crate.jpg", m_crate_texture));
    COMPAGES_TRY(loadTexture("path.png", m_floor_texture));

    // Thirty six vertices, six per face: no index buffer, each corner is
    // stored once per triangle that uses it.
    const std::array<compages::renderer::MeshVertex, 36u> cube{
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(-1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, -1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, -1.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(1.0f, 1.0f, 1.0f),
                   compages::core::Vector2f(1.0f, 1.0f))
    };

    const std::array<compages::renderer::MeshVertex, 6u> floor{
        meshVertex(compages::core::Vector3f(5.0f, -1.5f, 5.0f),
                   compages::core::Vector2f(0.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-5.0f, -1.5f, 5.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-5.0f, -1.5f, -5.0f),
                   compages::core::Vector2f(0.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(5.0f, -1.5f, 5.0f),
                   compages::core::Vector2f(1.0f, 0.0f)),
        meshVertex(compages::core::Vector3f(-5.0f, -1.5f, -5.0f),
                   compages::core::Vector2f(1.0f, 1.0f)),
        meshVertex(compages::core::Vector3f(5.0f, -1.5f, -5.0f),
                   compages::core::Vector2f(0.0f, 1.0f))
    };

    // The cube and the floor are two drawables of the same shader, each with
    // its own model matrix and texture.
    for (compages::gpu::Drawable* scene : { &m_cube, &m_floor })
    {
        COMPAGES_TRY(scene->load(SCENE_VS, SCENE_FS));
        scene->depthTest().cull(compages::gpu::CullMode::Back);
    }
    m_cube.vertices<compages::renderer::MeshVertex>(cube);
    m_cube["texID"] = m_crate_texture;
    m_floor.vertices<compages::renderer::MeshVertex>(floor);
    m_floor["texID"] = m_floor_texture;
    m_floor["model"] =
        compages::core::Matrix44f(compages::core::matrix::Identity);

    // Two triangles covering the screen, by name.
    COMPAGES_TRY(m_screen.load(SCREEN_VS, SCREEN_FS));
    m_screen["position"] = { { -1, 1 }, { -1, -1 }, { 1, -1 },
                             { -1, 1 }, { 1, -1 },  { 1, 1 } };
    m_screen["uv"] = { { 0, 1 }, { 0, 0 }, { 1, 0 },
                       { 0, 1 }, { 1, 0 }, { 1, 1 } };
    m_screen["texID"] = m_color_target;
    return compages::success();
}

//------------------------------------------------------------------------------
void PostProcess::draw(compages::world::ViewFrame const& p_frame)
{
    if (!compages::gpu::check(ensureTarget(p_frame.width, p_frame.height)))
    {
        return;
    }

    // Shared by the cube and the floor; only the cube's model turns.
    const compages::core::Matrix44f projection =
        compages::core::perspective(50.0_deg, aspect(p_frame), 0.1f, 10.0f);
    const compages::core::Matrix44f view =
        compages::core::lookAt(compages::core::Vector3f(3.0f, 3.0f, 3.0f),
                               compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                               compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    for (compages::gpu::Drawable* scene : { &m_cube, &m_floor })
    {
        (*scene)["view"] = view;
        (*scene)["projection"] = projection;
    }
    m_cube["model"] = compages::core::rotate(
        compages::core::Matrix44f(compages::core::matrix::Identity),
        units::angle::radian_t(p_frame.total * 0.7f),
        compages::core::Vector3f(0.0f, 1.0f, 0.0f));

    // Into the framebuffer, over the window pass.
    {
        compages::gpu::RenderPass offscreen(
            m_fbo, { .color = { 0.0f, 0.0f, 0.4f, 1.0f } });
        m_floor.draw();
        m_cube.draw();
    }

    // Onto the window, through the effect.
    compages::gpu::clear({ 1.0f, 1.0f, 1.0f });
    m_screen["time"] = p_frame.total;
    m_screen["screen_width"] = static_cast<float>(p_frame.width);
    m_screen["screen_height"] = static_cast<float>(p_frame.height);
    m_screen.draw();
}

} // namespace examples
