// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Fields/Field.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace compages
{

// ****************************************************************************
//! \brief Triangles in space, three corners each, with the normal of each
//! corner: what isosurface() makes.
// ****************************************************************************
struct Surface
{
    std::vector<Vector3f> positions;
    std::vector<Vector3f> normals;
};

namespace detail
{

//! \brief The gradient of a volume at a point of the grid (central
//! differences, one-sided on the sides).
inline Vector3f gradientAt(Field<float, Grid3D> const& p_volume, int p_i, int p_j, int p_k)
{
    Grid3D const& g = p_volume.grid();
    auto value = [&](int p_a, int p_b, int p_c)
    {
        return p_volume(std::clamp(p_a, 0, g.columns - 1), std::clamp(p_b, 0, g.rows - 1),
                        std::clamp(p_c, 0, g.layers - 1));
    };
    return Vector3f(value(p_i + 1, p_j, p_k) - value(p_i - 1, p_j, p_k),
                    value(p_i, p_j + 1, p_k) - value(p_i, p_j - 1, p_k),
                    value(p_i, p_j, p_k + 1) - value(p_i, p_j, p_k - 1));
}

} // namespace detail

// ----------------------------------------------------------------------------
//! \brief The surface where a volume crosses a level: the shape of the air
//! at 30 degrees, of a density, of a probability.
//!
//! Each cube of the grid is cut in six tetrahedra (marching tetrahedra: no
//! table of 256 cases, no hole between two cubes); the surface crosses the
//! edges of a tetrahedron where the values go across the level (linear). The
//! normals point toward the values under the level.
//! \code
//! compages::Surface hot = compages::isosurface(heat, 30.0f);
//! scene.mesh(compages::renderer::meshOf(hot).take(), "Hot air", color(1, 0.4, 0.1));
//! \endcode
//! \param[in] p_volume The volume.
//! \param[in] p_level The value of the surface.
//! \return Its triangles; none when the volume does not cross the level.
// ----------------------------------------------------------------------------
[[nodiscard]] inline Surface isosurface(Field<float, Grid3D> const& p_volume, float p_level)
{
    Grid3D const& grid = p_volume.grid();
    Surface surface;
    // The corners of a cube (i, j, k offsets), and its six tetrahedra around
    // the diagonal from corner 0 to corner 6.
    constexpr std::array<std::array<int, 3>, 8> corners = {
        { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } }
    };
    constexpr std::array<std::array<int, 4>, 6> tetrahedra = {
        { { 0, 6, 1, 2 }, { 0, 6, 2, 3 }, { 0, 6, 3, 7 }, { 0, 6, 7, 4 }, { 0, 6, 4, 5 }, { 0, 6, 5, 1 } }
    };
    std::array<Vector3f, 8> place{};
    std::array<float, 8> value{};
    std::array<Vector3f, 8> gradient{};
    auto cross = [&](int p_a, int p_b, Vector3f& p_point, Vector3f& p_normal)
    {
        const float s = (p_level - value[std::size_t(p_a)]) / (value[std::size_t(p_b)] - value[std::size_t(p_a)]);
        p_point = place[std::size_t(p_a)] + ((place[std::size_t(p_b)] - place[std::size_t(p_a)]) * s);
        p_normal = gradient[std::size_t(p_a)] + ((gradient[std::size_t(p_b)] - gradient[std::size_t(p_a)]) * s);
    };
    auto emit = [&](Vector3f p_a, Vector3f p_b, Vector3f p_c, Vector3f p_na, Vector3f p_nb, Vector3f p_nc)
    {
        // Turn the triangle so that its front faces the values under the
        // level: its normal along minus the gradient.
        const Vector3f u = p_b - p_a;
        const Vector3f v = p_c - p_a;
        const Vector3f face((u.y * v.z) - (u.z * v.y), (u.z * v.x) - (u.x * v.z), (u.x * v.y) - (u.y * v.x));
        const Vector3f down = (p_na + p_nb + p_nc) * -1.0f;
        if (((face.x * down.x) + (face.y * down.y) + (face.z * down.z)) < 0.0f)
        {
            std::swap(p_b, p_c);
            std::swap(p_nb, p_nc);
        }
        for (auto const& [p, n] : { std::pair{ p_a, p_na }, std::pair{ p_b, p_nb }, std::pair{ p_c, p_nc } })
        {
            const float length = std::sqrt((n.x * n.x) + (n.y * n.y) + (n.z * n.z));
            surface.positions.push_back(p);
            surface.normals.push_back((length > 0.0f) ? (n * (-1.0f / length)) : Vector3f(0.0f, 1.0f, 0.0f));
        }
    };
    for (int k = 0; k + 1 < grid.layers; ++k)
    {
        for (int j = 0; j + 1 < grid.rows; ++j)
        {
            for (int i = 0; i + 1 < grid.columns; ++i)
            {
                bool above = false;
                bool below = false;
                for (std::size_t c = 0u; c < 8u; ++c)
                {
                    const int a = i + corners[c][0];
                    const int b = j + corners[c][1];
                    const int d = k + corners[c][2];
                    value[c] = p_volume(a, b, d);
                    above = above || (value[c] >= p_level);
                    below = below || (value[c] < p_level);
                }
                if (!above || !below)
                {
                    continue; // the level does not cross this cube
                }
                for (std::size_t c = 0u; c < 8u; ++c)
                {
                    const int a = i + corners[c][0];
                    const int b = j + corners[c][1];
                    const int d = k + corners[c][2];
                    place[c] = grid.point(a, b, d);
                    gradient[c] = detail::gradientAt(p_volume, a, b, d);
                }
                for (auto const& t : tetrahedra)
                {
                    std::array<int, 4> in{};
                    std::array<int, 4> out{};
                    std::size_t ins = 0u;
                    std::size_t outs = 0u;
                    for (int c : t)
                    {
                        if (value[std::size_t(c)] >= p_level)
                        {
                            in[ins++] = c;
                        }
                        else
                        {
                            out[outs++] = c;
                        }
                    }
                    Vector3f p[4];
                    Vector3f n[4];
                    if ((ins == 1u) || (outs == 1u))
                    {
                        // One corner alone on its side: one triangle around it.
                        const int alone = (ins == 1u) ? in[0] : out[0];
                        std::array<int, 3> others = (ins == 1u) ? std::array<int, 3>{ out[0], out[1], out[2] }
                                                                : std::array<int, 3>{ in[0], in[1], in[2] };
                        for (std::size_t e = 0u; e < 3u; ++e)
                        {
                            cross(alone, others[e], p[e], n[e]);
                        }
                        emit(p[0], p[1], p[2], n[0], n[1], n[2]);
                    }
                    else if (ins == 2u)
                    {
                        // Two and two: a quad, in two triangles.
                        cross(in[0], out[0], p[0], n[0]);
                        cross(in[0], out[1], p[1], n[1]);
                        cross(in[1], out[1], p[2], n[2]);
                        cross(in[1], out[0], p[3], n[3]);
                        emit(p[0], p[1], p[2], n[0], n[1], n[2]);
                        emit(p[0], p[2], p[3], n[0], n[2], n[3]);
                    }
                }
            }
        }
    }
    return surface;
}

} // namespace compages
