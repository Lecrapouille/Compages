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

#include "00_GettingStarted/04_DepthAndTransforms.hpp"

#include "Common/ColoredCube.hpp"
#include "Compages/Core/Transformation.hpp"

//! \brief For the 60.0_deg the projection is asked for. An angle with its unit
//! written next to it cannot be passed in the wrong one, which is the whole reason
//! the maths here takes a radian_t rather than a float.
using namespace units::literals;

namespace examples
{

//! \brief The three matrices are separate uniforms and multiplied in the shader,
//! in the order every text on the subject uses. Combining them on the CPU would be
//! fewer multiplications, and is what a real renderer does; kept apart here because
//! this example is about what each one means.
constexpr const char* VERTEX_SHADER = R"(#version 450 core

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
    // The rotation applies to the normal as it does to the position, but the
    // translation does not: a direction has no place. Taking the upper three by
    // three part of the matrix is what says so.
    vNormal = mat3(model) * normal;
    vColor = color;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vNormal;
in vec3 vColor;

out vec4 oColor;

void main()
{
    // One fixed light, so that the faces are told apart by more than their colour
    // and the shape reads as a solid.
    const vec3 toLight = normalize(vec3(0.4, 0.8, 0.6));
    float lit = 0.25 + (0.75 * max(dot(normalize(vNormal), toLight), 0.0));
    oColor = vec4(vColor * lit, 1.0);
}
)";

//------------------------------------------------------------------------------
std::string IndexedCube::description() const
{
    return "Indices, so a corner is stored once and named several times. A depth "
           "test, so a near face covers a far one whatever the order. Back face "
           "culling, so half of a closed shape costs nothing. And three matrices, "
           "to place it, look at it and project it. The test and the culling belong "
           "to the render state of the drawable, so nothing has to be switched off "
           "afterwards.";
}

//------------------------------------------------------------------------------
compages::gpu::Status IndexedCube::setUp()
{
    COMPAGES_TRY(m_cube.load(VERTEX_SHADER, FRAGMENT_SHADER));

    // Twenty four corners and thirty six indices, built once on the CPU: see
    // Common/ColoredCube.hpp. Sixteen bit indices, because twenty four vertices
    // are a long way below sixty five thousand. The cube is one and a bit across
    // so that it fits in front of the camera.
    const CubeMesh cube = createCube(1.2f);
    m_cube.vertices(cube.vertices);
    m_cube.indices(cube.indices);
    m_cube.depthTest().cull(compages::gpu::CullMode::Back);
    return m_cube.prepare();
}

//------------------------------------------------------------------------------
void IndexedCube::draw(Frame const& p_frame)
{
    const Matrix44f identity(compages::matrix::Identity);

    const Matrix44f model = compages::matrix::rotate(identity,
                                     units::angle::radian_t(p_frame.total * 0.7f),
                                     Vector3f(0.3f, 1.0f, 0.2f));

    const Matrix44f view = compages::matrix::lookAt(Vector3f(0.0f, 0.0f, 3.0f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));

    // Read from the frame and not remembered from set up: the window can be
    // resized, and a projection built once is how a cube ends up looking like a
    // brick.
    const Matrix44f projection =
        compages::matrix::perspective(60.0_deg, aspect(p_frame), 0.1f, 20.0f);

    m_cube["model"] = model;
    m_cube["view"] = view;
    m_cube["projection"] = projection;

    // The recorded distances are forgotten as well as the colour. Leaving them
    // from the previous frame is what makes a rotating solid disappear behind
    // itself.
    compages::gpu::clear({ 0.07f, 0.07f, 0.1f });
    compages::gpu::clearDepth();

    // Every index, since the drawable has some.
    m_cube.draw();
}

} // namespace examples
