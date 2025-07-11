//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "Scene/Geometry/Sphere.hpp"

//------------------------------------------------------------------------------
static void repeat(std::vector<float> const& input,
                   size_t const N,
                   std::vector<float>& output)
{
    output.clear();
    output.reserve(N * input.size());
    for (auto const it : input)
    {
        for (uint32_t i = 0u; i < N; ++i)
        {
            output.push_back(it);
        }
    }
}

//------------------------------------------------------------------------------
static void tile(std::vector<float> const& input,
                 size_t const N,
                 std::vector<float>& output)
{
    output.clear();
    output.reserve(N * input.size());
    for (uint32_t i = 0u; i < N; ++i)
    {
        for (auto const it : input)
        {
            output.push_back(it);
        }
    }
}

//------------------------------------------------------------------------------
bool Sphere::generate(GLVertexBuffer<Vector3f>& vertices,
                      GLVertexBuffer<Vector3f>& normals,
                      GLVertexBuffer<Vector2f>& uv,
                      GLIndex32& index)
{
    std::vector<float> tmp, theta1, theta2;

    // Vertices
    maths::linspace(0.0f, maths::PI<float>, config.stacks, tmp, true);
    repeat(tmp, config.slices, theta1);
    maths::linspace(0.0f, maths::TWO_PI<float>, config.slices, tmp, true);
    tile(tmp, config.stacks, theta2);
    vertices.resize(config.slices * config.stacks);
    for (size_t i = 0u; i < vertices.size(); ++i)
    {
        vertices[i] =
            Vector3f(std::sin(theta1[i]) * std::sin(theta2[i]) * config.radius,
                     std::sin(theta1[i]) * std::cos(theta2[i]) * config.radius,
                     std::cos(theta1[i]) * config.radius);
    }

    // Normals
    normals = vertices;

    // Texture coord
    maths::linspace(0.0f, 1.0f, config.slices, tmp, true);
    repeat(tmp, config.stacks, theta1);
    maths::linspace(1.0f, 0.0f, config.stacks, tmp, true);
    tile(tmp, config.slices, theta2);
    for (size_t i = 0u; i < tmp.size(); ++i)
    {
        uv[i] = Vector2f(theta1[i], theta2[i]);
    }

    // Indices
    for (uint32_t i = 0u; i < config.stacks - 1u; ++i)
    {
        for (uint32_t j = 0u; j < config.slices - 1u; ++j)
        {
            index.append(i * config.slices + j);
            index.append(i * config.slices + j + 1u);
            index.append(i * config.slices + j + config.slices + 1u);
            index.append(i * config.slices + j + config.slices);
            index.append(i * config.slices + j + config.slices + 1u);
            index.append(i * config.slices + j);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool Sphere::generate(GLVAO32& vao, const bool clear)
{
    if (!vao.hasVBO<Vector3f>(shaders::name::position))
    {
        std::cerr << "VBO for vertices is needed" << std::endl;
        return false;
    }

    // Dummy containers.
    GLVertexBuffer<Vector3f> tmp_normals;
    GLVertexBuffer<Vector2f> tmp_uv;

    auto& positions = vao.vector3f(shaders::name::position);
    auto& normals = vao.hasVBO<Vector3f>(shaders::name::normal)
                        ? vao.vector3f(shaders::name::normal)
                        : tmp_normals;
    auto& UVs = vao.hasVBO<Vector2f>(shaders::name::uv)
                    ? vao.vector2f(shaders::name::uv)
                    : tmp_uv;

    if (clear)
    {
        positions.clear();
        normals.clear();
        UVs.clear();
        vao.index().clear();
    }

    return generate(positions, normals, UVs, vao.index());
}
