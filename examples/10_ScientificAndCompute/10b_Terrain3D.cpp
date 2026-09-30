// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_ScientificAndCompute/10b_Terrain3D.hpp"

#include "Common/DataPath.hpp"
#include "Compages/Core/Transformation.hpp"

#include <algorithm>
#include <random>
#include <vector>

using namespace units::literals;

namespace examples
{

constexpr std::uint32_t SIDE = 40u;

constexpr const char* VERTEX = R"(#version 450 core
in vec3 position;
in vec3 layer_coord;

uniform mat4 view;
uniform mat4 projection;

out vec3 vLayerCoord;

void main()
{
    vLayerCoord = layer_coord;
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec3 vLayerCoord;
uniform sampler3D layers;
out vec4 oColor;

void main()
{
    oColor = texture(layers, vLayerCoord);
}
)";

//! \brief Random heights, blurred five times and stretched back to [0, 1], with
//! a border at zero so the island is surrounded by deep water.
static std::vector<float> makeAltitudes(std::uint32_t p_side)
{
    std::mt19937 rng(42u);
    std::uniform_real_distribution<float> random(0.0f, 1.0f);
    std::vector<float> height(p_side * p_side);
    for (float& h : height)
    {
        h = random(rng);
    }

    const auto at = [p_side](std::uint32_t x, std::uint32_t y)
    { return (x * p_side) + y; };
    std::vector<float> blurred(height.size(), 0.0f);
    for (int pass = 0; pass < 5; ++pass)
    {
        for (std::uint32_t x = 1u; x + 1u < p_side; ++x)
        {
            for (std::uint32_t y = 1u; y + 1u < p_side; ++y)
            {
                float sum = 0.0f;
                for (std::uint32_t dx = 0u; dx < 3u; ++dx)
                {
                    for (std::uint32_t dy = 0u; dy < 3u; ++dy)
                    {
                        sum += height[at(x + dx - 1u, y + dy - 1u)];
                    }
                }
                blurred[at(x, y)] = sum / 9.0f;
            }
        }
        const auto [low, high] =
            std::minmax_element(blurred.begin(), blurred.end());
        const float range = std::max(*high - *low, 1e-6f);
        for (std::size_t i = 0u; i < height.size(); ++i)
        {
            height[i] = (blurred[i] - *low) / range;
        }
    }
    return height;
}

std::string Terrain3D::description() const
{
    return "A random island coloured by a 3D texture: six pictures stacked "
           "from "
           "deep water to snow. The altitude of each vertex is the third "
           "texture coordinate, so the hardware blends the two nearest "
           "pictures and the shore fades into the fields by itself.";
}

void Terrain3D::makeTerrain(std::uint32_t p_side)
{
    constexpr float MAX_HEIGHT = 0.2f;
    // Stop short of the last picture, so that only the highest peaks are snow.
    constexpr float MAX_LAYER = 0.9f;

    const std::vector<float> altitude = makeAltitudes(p_side);
    const auto side = static_cast<float>(p_side);

    // Position in the plane, height from the altitude, and that same altitude
    // as the third texture coordinate so the hardware picks the layer.
    std::vector<Vertex> grid;
    for (std::uint32_t x = 0u; x < p_side; ++x)
    {
        for (std::uint32_t y = 0u; y < p_side; ++y)
        {
            const float a = altitude[(x * p_side) + y];
            const compages::core::Vector3f uv(
                float(x) / side, float(y) / side, a * MAX_LAYER);
            grid.emplace_back(Vertex{ compages::core::Vector3f(uv.x - 0.5f,
                                                               uv.y - 0.5f,
                                                               a * MAX_HEIGHT),
                                      uv });
        }
    }

    // Two triangles per cell. The indices never change after this.
    std::vector<std::uint32_t> indices;
    for (std::uint32_t x = 0u; x + 1u < p_side; ++x)
    {
        for (std::uint32_t y = 0u; y + 1u < p_side; ++y)
        {
            const std::uint32_t here = (x * p_side) + y;
            indices.insert(indices.end(),
                           { here,
                             here + p_side,
                             here + 1u,
                             here + p_side,
                             here + p_side + 1u,
                             here + 1u });
        }
    }

    m_terrain.vertices(grid);
    m_terrain.indices(indices);
}

compages::Status Terrain3D::setUp()
{
    // Six pictures, deep water first and snow last: the order is the depth
    // axis of the 3D texture.
    std::vector<std::string> pictures;
    for (const char* file : { "deep_water.png",
                              "shallow_water.png",
                              "shore.png",
                              "fields.png",
                              "rocks.png",
                              "snow.png" })
    {
        pictures.emplace_back(dataPath(file));
        if (pictures.back().empty())
        {
            return compages::failure(
                "10b_Terrain3D needs the terrain pictures of "
                "external/Compages-data/");
        }
    }
    COMPAGES_TRY(
        m_layers.loadVolume(pictures,
                            { .mipmaps = false,
                              .srgb = true,
                              .wrap = compages::gpu::Wrap::ClampToEdge }));

    COMPAGES_TRY(m_terrain.load(VERTEX, FRAGMENT));
    makeTerrain(SIDE);
    // The sampler and the camera stay put; only the projection follows the
    // window, in draw().
    m_terrain["layers"] = m_layers;
    m_terrain["view"] =
        compages::core::lookAt(compages::core::Vector3f(0.75f, -0.75f, 0.75f),
                               compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                               compages::core::Vector3f(0.0f, 0.0f, 1.0f));
    m_terrain.depthTest();
    return m_terrain.prepare();
}

void Terrain3D::draw(compages::world::ViewFrame const& p_frame)
{
    // Rebuilt each frame: a projection remembered from setUp stretches when
    // the window is resized.
    m_terrain["projection"] =
        compages::core::perspective(60.0_deg, aspect(p_frame), 0.1f, 10.0f);

    compages::gpu::clear({ 0.0f, 0.0f, 0.4f });
    compages::gpu::clearDepth();
    m_terrain.draw();
}

} // namespace examples
