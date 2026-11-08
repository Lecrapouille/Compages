// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Meshes.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <array>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using compages::renderer::MeshAsset;
using compages::renderer::MeshVertex;
using units::literals::operator""_km;
using units::literals::operator""_m;

//! \brief The sea bed under the meshes: the sea is drawn over it.
static constexpr float BED = SEABED + compages::toMetres(6.0_m);

//! \brief Two triangles of a quad (a, b, c, d counterclockwise from above),
//! seen from above only, or from both sides.
static void quad(MeshAsset& p_mesh,
                 std::uint32_t p_a,
                 std::uint32_t p_b,
                 std::uint32_t p_c,
                 std::uint32_t p_d)
{
    p_mesh.source_indices.insert(p_mesh.source_indices.end(),
                                 { p_a, p_d, p_b, p_b, p_d, p_c });
}

static void bothSides(MeshAsset& p_mesh,
                      std::uint32_t p_a,
                      std::uint32_t p_b,
                      std::uint32_t p_c,
                      std::uint32_t p_d)
{
    p_mesh.source_indices.insert(
        p_mesh.source_indices.end(),
        { p_a, p_b, p_d, p_b, p_c, p_d, p_a, p_d, p_b, p_b, p_d, p_c });
}

// ----------------------------------------------------------------------------
//! \brief Each edge of the square again, \p p_depth lower, and a strip
//! between the two, seen from both sides.
// ----------------------------------------------------------------------------
static void addSkirt(MeshAsset& p_mesh, int p_side, float p_depth)
{
    const std::array<std::array<int, 4>, 4> edges = { { { 0, 0, 1, 0 },
                                                        { p_side - 1, 0, 0, 1 },
                                                        { 0, p_side - 1, 1, 0 },
                                                        { 0, 0, 0, 1 } } };
    for (std::array<int, 4> const& edge : edges)
    {
        auto top = [&](int p_k)
        {
            return std::uint32_t(((edge[1] + (p_k * edge[3])) * p_side) +
                                 edge[0] + (p_k * edge[2]));
        };
        const auto first = std::uint32_t(p_mesh.source_vertices.size());
        for (int k = 0; k < p_side; ++k)
        {
            MeshVertex low = p_mesh.source_vertices[top(k)];
            low.position.y -= p_depth;
            p_mesh.source_vertices.push_back(low);
        }
        for (int k = 0; k + 1 < p_side; ++k)
        {
            const std::uint32_t low = first + std::uint32_t(k);
            bothSides(p_mesh, top(k), top(k + 1), low + 1u, low);
        }
    }
}

// ----------------------------------------------------------------------------
MeshAsset meshOf(Patch const& p_patch, Square const& p_square, float p_skirt)
{
    MeshAsset mesh;
    const float step = p_patch.size / float(p_patch.cells);
    auto ground = [&p_patch](int p_a, int p_b)
    { return std::max(-BED, p_patch.height(p_a, p_b)); };
    const int side = p_square.cells + 1;
    float top = 0.0f;
    for (int j = 0; j < side; ++j)
    {
        for (int i = 0; i < side; ++i)
        {
            const int a = p_square.i + i;
            const int b = p_square.j + j;
            MeshVertex vertex;
            vertex.position = Vector3f(p_patch.x + (float(a) * step),
                                       ground(a, b),
                                       p_patch.z + (float(b) * step));
            vertex.normal = compages::vector::normalize(
                Vector3f(ground(a - 1, b) - ground(a + 1, b),
                         2.0f * step,
                         ground(a, b - 1) - ground(a, b + 1)));
            vertex.uv =
                p_square.uv + (Vector2f(float(i), float(j)) * p_square.uv_step);
            mesh.source_vertices.push_back(vertex);
            top = std::max(top, vertex.position.y);
        }
    }
    for (int j = 0; j < p_square.cells; ++j)
    {
        for (int i = 0; i < p_square.cells; ++i)
        {
            const auto a = std::uint32_t((j * side) + i);
            const auto d = a + std::uint32_t(side);
            quad(mesh, a, a + 1u, d + 1u, d);
        }
    }
    if (p_skirt > 0.0f)
    {
        addSkirt(mesh, side, p_skirt);
    }
    mesh.index_count = mesh.source_indices.size();
    const float x0 = p_patch.x + (float(p_square.i) * step);
    const float z0 = p_patch.z + (float(p_square.j) * step);
    const float width = float(p_square.cells) * step;
    mesh.local_bounds =
        compages::AABB::fromCorners(Vector3f(x0, -BED - p_skirt, z0),
                                    Vector3f(x0 + width, top, z0 + width));
    return mesh;
}

// ----------------------------------------------------------------------------
compages::renderer::TextureAsset pictureOf(std::string p_name,
                                           Patch const& p_patch)
{
    compages::renderer::TextureAsset picture;
    picture.name = std::move(p_name);
    picture.description =
        compages::gpu::TextureDesc::image(std::uint32_t(p_patch.pixels),
                                          std::uint32_t(p_patch.pixels),
                                          compages::gpu::PixelFormat::SRGB8A8);
    picture.description.levels = 0u;
    picture.pixels = p_patch.picture;
    return picture;
}

// ----------------------------------------------------------------------------
MeshAsset oceanMesh()
{
    const float far = compages::toMetres(150.0_km);
    const float near = 0.5f * MAP;
    MeshAsset ocean;
    const std::array<Vector2f, 4> inner = { Vector2f(-near, -near),
                                            Vector2f(near, -near),
                                            Vector2f(near, near),
                                            Vector2f(-near, near) };
    for (Vector2f const& corner : inner)
    {
        for (const float reach : { near, far })
        {
            MeshVertex vertex;
            const Vector2f p = corner * (reach / near);
            vertex.position = Vector3f(p.x, -BED, p.y);
            vertex.normal = Vector3f::UNIT_Y;
            vertex.uv = Vector2f((corner.x > 0.0f) ? 1.0f : 0.0f,
                                 (corner.y > 0.0f) ? 1.0f : 0.0f);
            ocean.source_vertices.push_back(vertex);
        }
    }
    for (std::uint32_t k = 0u; k < 4u; ++k)
    {
        const std::uint32_t a = 2u * k;
        const std::uint32_t b = 2u * ((k + 1u) % 4u);
        ocean.source_indices.insert(ocean.source_indices.end(),
                                    { a, b, a + 1u, b, b + 1u, a + 1u });
    }
    ocean.index_count = ocean.source_indices.size();
    ocean.local_bounds = compages::AABB::fromCorners(
        Vector3f(-far, -100.0f, -far), Vector3f(far, 1.0f, far));
    return ocean;
}

// ----------------------------------------------------------------------------
MeshAsset gridMesh(int p_cells)
{
    MeshAsset grid;
    for (int j = 0; j <= p_cells; ++j)
    {
        for (int i = 0; i <= p_cells; ++i)
        {
            MeshVertex vertex;
            vertex.position = Vector3f((float(i) / float(p_cells)) - 0.5f,
                                       0.0f,
                                       (float(j) / float(p_cells)) - 0.5f);
            vertex.normal = Vector3f::UNIT_Y;
            grid.source_vertices.push_back(vertex);
        }
    }
    for (int j = 0; j < p_cells; ++j)
    {
        for (int i = 0; i < p_cells; ++i)
        {
            const auto a = std::uint32_t((j * (p_cells + 1)) + i);
            const auto d = a + std::uint32_t(p_cells + 1);
            quad(grid, a, a + 1u, d + 1u, d);
        }
    }
    grid.index_count = grid.source_indices.size();
    grid.local_bounds = compages::AABB::fromCorners(
        Vector3f(-0.5f, -10.0f, -0.5f), Vector3f(0.5f, 10.0f, 0.5f));
    return grid;
}

// ----------------------------------------------------------------------------
MeshAsset hiddenMesh()
{
    MeshAsset mesh;
    for (Vector3f const& corner : { Vector3f(0.0f, -500.0f, 0.0f),
                                    Vector3f(1.0f, -500.0f, 0.0f),
                                    Vector3f(0.0f, -500.0f, 1.0f) })
    {
        MeshVertex vertex;
        vertex.position = corner;
        vertex.normal = Vector3f::UNIT_Y;
        mesh.source_vertices.push_back(vertex);
    }
    mesh.source_indices = { 0u, 1u, 2u };
    mesh.index_count = 3u;
    mesh.local_bounds = compages::AABB::fromCorners(
        Vector3f(0.0f, -501.0f, 0.0f), Vector3f(1.0f, -499.0f, 1.0f));
    return mesh;
}

// ----------------------------------------------------------------------------
compages::Status replaceMesh(compages::renderer::Scene const& p_scene,
                             compages::world::Entity p_entity,
                             compages::renderer::MeshAsset p_mesh)
{
    MeshAsset* current = p_scene.assets().mesh(
        p_entity.get<compages::renderer::MeshRenderer>().mesh);
    if (current == nullptr)
    {
        return compages::failure("An entity of the archipelago lost its mesh");
    }
    current->source_vertices = std::move(p_mesh.source_vertices);
    current->source_indices = std::move(p_mesh.source_indices);
    current->index_count = p_mesh.index_count;
    current->local_bounds = p_mesh.local_bounds;
    current->releaseGpu();
    return current->upload();
}

// ----------------------------------------------------------------------------
compages::Status wear(compages::renderer::Scene const& p_scene,
                      compages::world::Entity p_entity,
                      compages::renderer::MaterialId p_material)
{
    compages::renderer::MaterialInstance* instance =
        p_scene.assets().materialInstance(
            p_entity.get<compages::renderer::MeshRenderer>().material_instance);
    if (instance == nullptr)
    {
        return compages::failure(
            "An entity of the archipelago has no material instance");
    }
    instance->material = p_material;
    return compages::success();
}

} // namespace examples::archipelago
