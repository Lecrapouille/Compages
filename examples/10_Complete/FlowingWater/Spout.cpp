// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/FlowingWater/Spout.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace examples::flowing
{

using compages::Vector2f;
using compages::Vector3f;
using compages::renderer::MeshAsset;
using compages::renderer::MeshVertex;

namespace
{

constexpr float GRAVITY = 9.81f;
constexpr int AROUND = 14;

// ----------------------------------------------------------------------------
//! \brief Join ring \c p_ring to the ring after it (AROUND + 1 points each:
//! the last one repeats the first with U = 1, so the picture has no seam).
// ----------------------------------------------------------------------------
void joinRings(MeshAsset& p_mesh, std::uint32_t p_ring)
{
    const std::uint32_t a = p_ring * std::uint32_t(AROUND + 1);
    const std::uint32_t b = a + std::uint32_t(AROUND + 1);
    for (std::uint32_t k = 0u; k < std::uint32_t(AROUND); ++k)
    {
        p_mesh.source_indices.insert(p_mesh.source_indices.end(),
                                     { a + k, b + k, a + k + 1u, a + k + 1u, b + k, b + k + 1u });
    }
}

void finish(MeshAsset& p_mesh)
{
    Vector3f low = p_mesh.source_vertices.front().position;
    Vector3f high = low;
    for (MeshVertex const& vertex : p_mesh.source_vertices)
    {
        low = Vector3f(std::min(low.x, vertex.position.x), std::min(low.y, vertex.position.y),
                       std::min(low.z, vertex.position.z));
        high = Vector3f(std::max(high.x, vertex.position.x), std::max(high.y, vertex.position.y),
                        std::max(high.z, vertex.position.z));
    }
    p_mesh.index_count = p_mesh.source_indices.size();
    p_mesh.local_bounds = compages::AABB::fromCorners(low, high);
}

} // namespace

// ----------------------------------------------------------------------------
float Jet::fall(float p_level) const
{
    // mouth.y + v t - g t^2 / 2 = level, the root after the start.
    const float drop = mouth.y - p_level;
    return (velocity.y + std::sqrt((velocity.y * velocity.y) + (2.0f * GRAVITY * drop))) / GRAVITY;
}

// ----------------------------------------------------------------------------
Vector3f Jet::at(float p_time) const
{
    return mouth + (velocity * p_time) + Vector3f(0.0f, -0.5f * GRAVITY * p_time * p_time, 0.0f);
}

// ----------------------------------------------------------------------------
//! Rings of points round the path, each turned to face along the jet (its
//! speed at that time); the jet widens a little as it falls.
// ----------------------------------------------------------------------------
MeshAsset jetMesh(Jet const& p_jet, float p_level)
{
    constexpr int ALONG = 24;
    const float end = 1.08f * p_jet.fall(p_level);
    MeshAsset mesh;
    float length = 0.0f;
    Vector3f before = p_jet.mouth;
    for (int i = 0; i <= ALONG; ++i)
    {
        const float t = end * float(i) / float(ALONG);
        const Vector3f centre = p_jet.at(t);
        length += compages::vector::distance(centre, before);
        before = centre;
        const Vector3f along = compages::vector::normalize(p_jet.velocity + Vector3f(0.0f, -GRAVITY * t, 0.0f));
        const Vector3f side = compages::vector::normalize(compages::vector::cross(along, Vector3f(0.0f, 1.0f, 0.0f)));
        const Vector3f up = compages::vector::cross(side, along);
        const float radius = p_jet.radius * (1.0f + (0.35f * t / end));
        for (int k = 0; k <= AROUND; ++k)
        {
            const float angle = 2.0f * std::numbers::pi_v<float> * float(k) / float(AROUND);
            const Vector3f out = (side * std::cos(angle)) + (up * std::sin(angle));
            MeshVertex vertex;
            vertex.position = centre + (out * radius);
            vertex.normal = out;
            vertex.uv = Vector2f(float(k) / float(AROUND), length);
            mesh.source_vertices.push_back(vertex);
        }
        if (i > 0)
        {
            joinRings(mesh, std::uint32_t(i - 1));
        }
    }
    finish(mesh);
    return mesh;
}

// ----------------------------------------------------------------------------
MeshAsset crownMesh(float p_radius, float p_height)
{
    constexpr int UP = 6;
    MeshAsset mesh;
    for (int i = 0; i <= UP; ++i)
    {
        const float v = float(i) / float(UP);
        // Out and up, curling outward near the top.
        const float radius = p_radius * (0.6f + (0.4f * v) + (0.5f * v * v));
        for (int k = 0; k <= AROUND; ++k)
        {
            const float angle = 2.0f * std::numbers::pi_v<float> * float(k) / float(AROUND);
            const Vector3f out(std::cos(angle), 0.0f, std::sin(angle));
            MeshVertex vertex;
            vertex.position = (out * radius) + Vector3f(0.0f, p_height * v, 0.0f);
            vertex.normal = compages::vector::normalize(out + Vector3f(0.0f, 0.4f, 0.0f));
            vertex.uv = Vector2f(float(k) / float(AROUND), v);
            mesh.source_vertices.push_back(vertex);
        }
        if (i > 0)
        {
            joinRings(mesh, std::uint32_t(i - 1));
        }
    }
    finish(mesh);
    return mesh;
}

} // namespace examples::flowing
