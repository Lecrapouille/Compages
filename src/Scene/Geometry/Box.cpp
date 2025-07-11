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

#include "Scene/Geometry/Box.hpp"

//------------------------------------------------------------------------------
bool Box::generate(GLVAO32& vao, const bool clear)
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

//------------------------------------------------------------------------------
bool Box::generate(GLVertexBuffer<Vector3f>& vertices,
                   GLVertexBuffer<Vector3f>& normals,
                   GLVertexBuffer<Vector2f>& uv,
                   GLIndex32& index)
{
    uint32_t numberOfVertices = 0;

    buildPlane(Z,
               Y,
               X,
               -1.0f,
               -1.0f,
               config.depth,
               config.height,
               config.width,
               config.depthSegments,
               config.heightSegments,
               vertices,
               normals,
               uv,
               index,
               numberOfVertices); // px

    buildPlane(Z,
               Y,
               X,
               1.0f,
               -1.0f,
               config.depth,
               config.height,
               -config.width,
               config.depthSegments,
               config.heightSegments,
               vertices,
               normals,
               uv,
               index,
               numberOfVertices); // nx

    buildPlane(X,
               Z,
               Y,
               1.0f,
               1.0f,
               config.width,
               config.depth,
               config.height,
               config.widthSegments,
               config.depthSegments,
               vertices,
               normals,
               uv,
               index,
               numberOfVertices); // py

    buildPlane(X,
               Z,
               Y,
               1.0f,
               -1.0f,
               config.width,
               config.depth,
               -config.height,
               config.widthSegments,
               config.depthSegments,
               vertices,
               normals,
               uv,
               index,
               numberOfVertices); // ny

    buildPlane(X,
               Y,
               Z,
               1.0f,
               -1.0f,
               config.width,
               config.height,
               config.depth,
               config.widthSegments,
               config.heightSegments,
               vertices,
               normals,
               uv,
               index,
               numberOfVertices); // pz

    buildPlane(X,
               Y,
               Z,
               -1.0f,
               -1.0f,
               config.width,
               config.height,
               -config.depth,
               config.widthSegments,
               config.heightSegments,
               vertices,
               normals,
               uv,
               index,
               numberOfVertices); // nz

    return true;
}

//------------------------------------------------------------------------------
// Original code https://github.com/mrdoob/three.js/
// three.js/src/geometries/BoxBufferGeometry.js
//------------------------------------------------------------------------------
void Box::buildPlane(Side u,
                     Side v,
                     Side w,
                     float udir,
                     float vdir,
                     float width,
                     float height,
                     float depth,
                     uint32_t gridX,
                     uint32_t gridY,
                     GLVertexBuffer<Vector3f>& vertices,
                     GLVertexBuffer<Vector3f>& normals,
                     GLVertexBuffer<Vector2f>& uv,
                     GLIndex32& index,
                     uint32_t& numberOfVertices)
{
    const float segmentWidth = width / float(gridX);
    const float segmentHeight = height / float(gridY);

    const float widthHalf = width / 2.0f;
    const float heightHalf = height / 2.0f;
    const float depthHalf = depth / 2.0f;

    const uint32_t gridX1 = gridX + 1u;
    const uint32_t gridY1 = gridY + 1u;

    Vector3f vec;
    uint32_t vertexCounter = 0u;

    for (uint32_t iy = 0u; iy < gridY1; ++iy)
    {
        const float y = float(iy) * segmentHeight - heightHalf;
        for (uint32_t ix = 0u; ix < gridX1; ++ix)
        {
            const float x = float(ix) * segmentWidth - widthHalf;

            // vertices buffer
            vec[u] = x * udir;
            vec[v] = y * vdir;
            vec[w] = depthHalf;
            vertices.append(vec);

            // normals buffer
            vec[u] = 0;
            vec[v] = 0;
            vec[w] = depth > 0.0f ? 1.0f : -1.0f;
            normals.append(vec);

            // UV buffer
            uv.append(Vector2f(float(ix) / float(gridX),
                               1.0f - (float(iy) / float(gridY))));

            vertexCounter += 1;
        }
    }

    // Indices
    // const uint32_t numberOfVertices = uint32_t(vertices.size());
    for (uint32_t iy = 0u; iy < gridY; ++iy)
    {
        for (uint32_t ix = 0u; ix < gridX; ++ix)
        {
            uint32_t a = numberOfVertices + ix + gridX1 * iy;
            uint32_t b = numberOfVertices + ix + gridX1 * (iy + 1u);
            uint32_t c = numberOfVertices + (ix + 1u) + gridX1 * (iy + 1u);
            uint32_t d = numberOfVertices + (ix + 1u) + gridX1 * iy;

            // Faces
            index.append(a).append(b).append(d);
            index.append(b).append(c).append(d);
        }
    }

    numberOfVertices += vertexCounter;
}
