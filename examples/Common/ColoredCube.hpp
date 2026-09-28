// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// ****************************************************************************
//! \file
//! \brief The cube with one colour per face that the first 3D examples draw,
//! built on the CPU once rather than typed in each of them.
// ****************************************************************************

namespace examples
{

// ****************************************************************************
//! \brief One corner of the cube, with the direction its face points in.
//!
//! Twenty four of them for eight geometric corners, because a corner of a cube
//! belongs to three faces pointing three different ways, and a vertex holds one
//! normal. Sharing the eight would give a smooth ball of a cube: the sharing
//! that indices allow is sharing of identical vertices, not of positions.
//! The field names are the attribute names the shaders declare.
// ****************************************************************************
struct CubeVertex
{
    Vector3f position;
    Vector3f normal;
    Vector3f color;
};

// ****************************************************************************
//! \brief What createCube() makes: the vertices and the triangles naming them.
// ****************************************************************************
struct CubeMesh
{
    std::vector<CubeVertex> vertices;
    std::vector<std::uint16_t> indices;
};

// ----------------------------------------------------------------------------
//! \brief A cube centred on the origin, \c p_size wide, one colour per face.
//!
//! Twenty four vertices and thirty six sixteen bit indices: four corners and two
//! triangles per face, counter clockwise seen from outside, which is what lets
//! back face culling drop the far faces rather than the near ones.
// ----------------------------------------------------------------------------
[[nodiscard]] inline CubeMesh createCube(float p_size = 1.0f)
{
    struct Face
    {
        Vector3f normal;
        Vector3f color;
    };

    const std::array<Face, 6u> faces{
        Face{ Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.9f, 0.3f, 0.3f) },
        Face{ Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.3f, 0.9f, 0.4f) },
        Face{ Vector3f(1.0f, 0.0f, 0.0f), Vector3f(0.3f, 0.5f, 0.9f) },
        Face{ Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(0.9f, 0.8f, 0.3f) },
        Face{ Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.8f, 0.4f, 0.9f) },
        Face{ Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.4f, 0.9f, 0.9f) }
    };

    const float half = p_size * 0.5f;
    CubeMesh cube;
    cube.vertices.reserve(24u);
    cube.indices.reserve(36u);
    for (Face const& face : faces)
    {
        // Two directions along the face, found from its normal, so that the four
        // corners come out of the normal rather than being typed twenty four times.
        const Vector3f up = (std::abs(face.normal.y) > 0.5f) ? Vector3f(0.0f, 0.0f, 1.0f)
                                                             : Vector3f(0.0f, 1.0f, 0.0f);
        const Vector3f right = compages::vector::cross(up, face.normal);
        const Vector3f top = compages::vector::cross(face.normal, right);

        const auto first = static_cast<std::uint16_t>(cube.vertices.size());
        cube.vertices.emplace_back((face.normal - right - top) * half, face.normal, face.color);
        cube.vertices.emplace_back((face.normal + right - top) * half, face.normal, face.color);
        cube.vertices.emplace_back((face.normal + right + top) * half, face.normal, face.color);
        cube.vertices.emplace_back((face.normal - right + top) * half, face.normal, face.color);

        // Four vertices, six indices: the two triangles of a face share a
        // diagonal, and the shared pair is stored once.
        constexpr std::array<std::uint16_t, 6u> CORNERS{ 0u, 1u, 2u, 0u, 2u, 3u };
        for (const std::uint16_t corner : CORNERS)
        {
            cube.indices.emplace_back(static_cast<std::uint16_t>(first + corner));
        }
    }
    return cube;
}

} // namespace examples
