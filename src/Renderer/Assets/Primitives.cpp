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

#include "Compages/Renderer/Assets/Primitives.hpp"

#include "Compages/Core/Maths.hpp"
#include "Compages/GPU/Core/Layout.hpp"
#include "Compages/Renderer/Assets/ShaderLib.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace compages::renderer
{

namespace
{

constexpr std::uint32_t kMaxVertices = 65535u;

[[nodiscard]] std::uint16_t u16(std::size_t p_index)
{
    return static_cast<std::uint16_t>(p_index);
}

void addTriangle(std::vector<std::uint16_t>& p_indices,
                 std::uint16_t p_a,
                 std::uint16_t p_b,
                 std::uint16_t p_c)
{
    p_indices.emplace_back(p_a);
    p_indices.emplace_back(p_b);
    p_indices.emplace_back(p_c);
}

void addQuad(std::vector<std::uint16_t>& p_indices,
             std::uint16_t p_a,
             std::uint16_t p_b,
             std::uint16_t p_c,
             std::uint16_t p_d)
{
    addTriangle(p_indices, p_a, p_b, p_c);
    addTriangle(p_indices, p_a, p_c, p_d);
}

void addGridCell(std::vector<std::uint16_t>& p_indices,
                 std::uint16_t p_row0,
                 std::uint16_t p_row1,
                 std::uint16_t p_col0,
                 std::uint16_t p_col1,
                 std::uint32_t p_stride)
{
    const std::uint16_t i0 = u16((p_row0 * p_stride) + p_col0);
    const std::uint16_t i1 = u16((p_row0 * p_stride) + p_col1);
    const std::uint16_t i2 = u16((p_row1 * p_stride) + p_col0);
    const std::uint16_t i3 = u16((p_row1 * p_stride) + p_col1);
    addTriangle(p_indices, i0, i2, i1);
    addTriangle(p_indices, i1, i2, i3);
}

[[nodiscard]] compages::core::Vector3f
scaled(compages::core::Vector3f const& p_unit,
       compages::core::Vector3f const& p_half)
{
    return compages::core::Vector3f(
        p_unit.x * p_half.x, p_unit.y * p_half.y, p_unit.z * p_half.z);
}

void addBoxFace(std::vector<MeshVertex>& p_corners,
                std::vector<std::uint16_t>& p_indices,
                compages::core::Vector3f const& p_normal,
                compages::core::Vector3f const& p_half)
{
    const compages::core::Vector3f up =
        (std::abs(p_normal.y) > 0.5f)
            ? compages::core::Vector3f(0.0f, 0.0f, 1.0f)
            : compages::core::Vector3f(0.0f, 1.0f, 0.0f);
    const compages::core::Vector3f right =
        compages::core::vector::cross(up, p_normal);
    const compages::core::Vector3f top =
        compages::core::vector::cross(p_normal, right);
    const std::uint16_t first = u16(p_corners.size());
    p_corners.emplace_back(scaled(p_normal - right - top, p_half),
                           p_normal,
                           compages::core::Vector2f(0.0f, 0.0f));
    p_corners.emplace_back(scaled(p_normal + right - top, p_half),
                           p_normal,
                           compages::core::Vector2f(1.0f, 0.0f));
    p_corners.emplace_back(scaled(p_normal + right + top, p_half),
                           p_normal,
                           compages::core::Vector2f(1.0f, 1.0f));
    p_corners.emplace_back(scaled(p_normal - right + top, p_half),
                           p_normal,
                           compages::core::Vector2f(0.0f, 1.0f));
    addQuad(
        p_indices, first, u16(first + 1u), u16(first + 2u), u16(first + 3u));
}

void addTubeRing(std::vector<MeshVertex>& p_corners,
                 std::vector<float> const& p_angle,
                 std::vector<float> const& p_u,
                 float p_radius,
                 float p_z,
                 float p_side_xy,
                 float p_side_z,
                 float p_v)
{
    for (std::size_t i = 0u; i < p_angle.size(); ++i)
    {
        const float c = std::cos(p_angle[i]);
        const float s = std::sin(p_angle[i]);
        p_corners.emplace_back(
            compages::core::Vector3f(p_radius * c, p_radius * s, p_z),
            compages::core::Vector3f(p_side_xy * c, p_side_xy * s, p_side_z),
            compages::core::Vector2f(p_u[i], p_v));
    }
}

void addDiskCap(std::vector<MeshVertex>& p_corners,
                std::vector<std::uint16_t>& p_indices,
                std::vector<float> const& p_angle,
                std::uint32_t p_slices,
                float p_radius,
                float p_z,
                compages::core::Vector3f const& p_normal,
                bool p_ccw_from_outside)
{
    const std::uint16_t ring_start = u16(p_corners.size());
    for (float a : p_angle)
    {
        const float c = std::cos(a);
        const float s = std::sin(a);
        p_corners.emplace_back(
            compages::core::Vector3f(p_radius * c, p_radius * s, p_z),
            p_normal,
            compages::core::Vector2f(0.5f + (0.5f * c), 0.5f + (0.5f * s)));
    }
    const std::uint16_t center = u16(p_corners.size());
    p_corners.emplace_back(compages::core::Vector3f(0.0f, 0.0f, p_z),
                           p_normal,
                           compages::core::Vector2f(0.5f, 0.5f));
    for (std::uint32_t i = 0u; i < p_slices; ++i)
    {
        const std::uint16_t a = u16(ring_start + i);
        const std::uint16_t b = u16(ring_start + i + 1u);
        if (p_ccw_from_outside)
        {
            addTriangle(p_indices, center, a, b);
        }
        else
        {
            addTriangle(p_indices, center, b, a);
        }
    }
}

Result<MeshAsset> uploadMesh(std::vector<MeshVertex> p_corners,
                             std::vector<std::uint16_t> p_indices)
{
    compages::core::AABB bounds;
    for (MeshVertex const& corner : p_corners)
    {
        bounds.expand(corner.position);
    }

    MeshAsset mesh;
    mesh.local_bounds = bounds;
    mesh.source_vertices = std::move(p_corners);
    mesh.source_indices.assign(p_indices.begin(), p_indices.end());
    COMPAGES_TRY(mesh.upload());
    return mesh;
}

Result<Material> makeMaterial(std::string_view p_name,
                              ShaderFamily p_family,
                              std::string_view p_vertex,
                              std::string_view p_fragment,
                              bool p_with_uv)
{
    auto program_result =
        compages::gpu::Program::fromSources(p_vertex, p_fragment);
    if (!program_result)
    {
        return failure(program_result.error());
    }
    auto program = program_result.take();
    // A shader that reads no uv is fine: a field the shader does not read is
    // simply not sent.
    (void)p_with_uv;
    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<MeshVertex>();
    compages::gpu::RenderState state;
    state.depth_test = true;
    state.cull = compages::gpu::CullMode::Back;
    auto pipeline_result =
        compages::gpu::Pipeline::create<MeshVertex>(program, layout, state);
    if (!pipeline_result)
    {
        return failure(pipeline_result.error());
    }
    auto pipeline = pipeline_result.take();

    Material material;
    material.name = std::string(p_name);
    material.family = p_family;
    material.program = std::move(program);
    material.pipeline = std::move(pipeline);
    return material;
}

} // namespace

//------------------------------------------------------------------------------
Result<MeshAsset> makeCube()
{
    return makeBox(1.0f, 1.0f, 1.0f);
}

//------------------------------------------------------------------------------
Result<MeshAsset> makePrimitive(PrimitiveShape p_shape)
{
    using enum PrimitiveShape;
    switch (p_shape)
    {
        case Cube:
            return makeCube();
        case Sphere:
            return makeSphere();
        case Plane:
            return makePlane();
    }
    return failure("unknown primitive shape");
}

//------------------------------------------------------------------------------
Result<MeshAsset> makeBox(float p_width, float p_height, float p_depth)
{
    if ((p_width <= 0.0f) || (p_height <= 0.0f) || (p_depth <= 0.0f))
    {
        return failure("makeBox needs positive width, height and depth");
    }

    const compages::core::Vector3f half(
        p_width * 0.5f, p_height * 0.5f, p_depth * 0.5f);
    const std::array<compages::core::Vector3f, 6u> normals{
        compages::core::Vector3f(0.0f, 0.0f, 1.0f),
        compages::core::Vector3f(0.0f, 0.0f, -1.0f),
        compages::core::Vector3f(1.0f, 0.0f, 0.0f),
        compages::core::Vector3f(-1.0f, 0.0f, 0.0f),
        compages::core::Vector3f(0.0f, 1.0f, 0.0f),
        compages::core::Vector3f(0.0f, -1.0f, 0.0f)
    };

    std::vector<MeshVertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve(24u);
    indices.reserve(36u);
    for (compages::core::Vector3f const& normal : normals)
    {
        addBoxFace(corners, indices, normal, half);
    }
    return uploadMesh(std::move(corners), std::move(indices));
}

//------------------------------------------------------------------------------
Result<MeshAsset>
makeSphere(float p_radius, std::uint32_t p_stacks, std::uint32_t p_slices)
{
    if ((p_radius <= 0.0f) || (p_stacks < 2u) || (p_slices < 3u))
    {
        return failure("makeSphere needs a positive radius, at least two "
                       "stacks and three slices");
    }

    const std::uint32_t stride = p_slices + 1u;
    const std::uint32_t vertex_count = (p_stacks + 1u) * stride;
    if (vertex_count > kMaxVertices)
    {
        return failure("makeSphere would exceed 65535 vertices; reduce "
                       "stacks or slices");
    }

    std::vector<MeshVertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve(vertex_count);
    indices.reserve(p_stacks * p_slices * 6u);

    for (std::uint32_t stack = 0u; stack <= p_stacks; ++stack)
    {
        const float v =
            static_cast<float>(stack) / static_cast<float>(p_stacks);
        const float phi =
            (v * compages::core::PI<float>)-compages::core::HALF_PI<float>;
        const float cos_phi = std::cos(phi);
        const float sin_phi = std::sin(phi);
        for (std::uint32_t slice = 0u; slice <= p_slices; ++slice)
        {
            const float u =
                static_cast<float>(slice) / static_cast<float>(p_slices);
            const float theta = u * compages::core::TWO_PI<float>;
            const compages::core::Vector3f normal(
                cos_phi * std::cos(theta), sin_phi, cos_phi * std::sin(theta));
            corners.emplace_back(normal * p_radius,
                                 normal,
                                 compages::core::Vector2f(u, 1.0f - v));
        }
    }

    for (std::uint32_t stack = 0u; stack < p_stacks; ++stack)
    {
        for (std::uint32_t slice = 0u; slice < p_slices; ++slice)
        {
            addGridCell(indices,
                        u16(stack),
                        u16(stack + 1u),
                        u16(slice),
                        u16(slice + 1u),
                        stride);
        }
    }

    return uploadMesh(std::move(corners), std::move(indices));
}

//------------------------------------------------------------------------------
Result<MeshAsset> makePlane(float p_width,
                            float p_height,
                            std::uint32_t p_x_segments,
                            std::uint32_t p_y_segments)
{
    if ((p_width <= 0.0f) || (p_height <= 0.0f) || (p_x_segments < 1u) ||
        (p_y_segments < 1u))
    {
        return failure(
            "makePlane needs positive size and at least one segment per axis");
    }

    const std::uint32_t cols = p_x_segments + 1u;
    const std::uint32_t rows = p_y_segments + 1u;
    if ((cols * rows) > kMaxVertices)
    {
        return failure("makePlane would exceed 65535 vertices");
    }

    const float half_w = p_width * 0.5f;
    const float half_h = p_height * 0.5f;
    const compages::core::Vector3f normal(0.0f, 0.0f, 1.0f);

    std::vector<MeshVertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve(cols * rows);
    indices.reserve(p_x_segments * p_y_segments * 6u);

    for (std::uint32_t y = 0u; y < rows; ++y)
    {
        const float v =
            static_cast<float>(y) / static_cast<float>(p_y_segments);
        const float py = (v * p_height) - half_h;
        for (std::uint32_t x = 0u; x < cols; ++x)
        {
            const float u =
                static_cast<float>(x) / static_cast<float>(p_x_segments);
            corners.emplace_back(
                compages::core::Vector3f((u * p_width) - half_w, py, 0.0f),
                normal,
                compages::core::Vector2f(u, v));
        }
    }

    for (std::uint32_t y = 0u; y < p_y_segments; ++y)
    {
        for (std::uint32_t x = 0u; x < p_x_segments; ++x)
        {
            // Rows swapped: y grows upward here, so this is what winds the
            // cells counter-clockwise seen from +Z, the side the normal faces.
            addGridCell(
                indices, u16(y + 1u), u16(y), u16(x), u16(x + 1u), cols);
        }
    }

    return uploadMesh(std::move(corners), std::move(indices));
}

//------------------------------------------------------------------------------
Result<MeshAsset> makeTube(float p_top_radius,
                           float p_bottom_radius,
                           float p_height,
                           std::uint32_t p_slices,
                           bool p_tip_along_negative_z)
{
    if ((p_height <= 0.0f) || (p_slices < 3u))
    {
        return failure(
            "makeTube needs a positive height and at least three slices");
    }
    if ((p_bottom_radius < 0.0f) || (p_top_radius < 0.0f))
    {
        return failure("makeTube radii must be non-negative");
    }

    const float abs_top = std::abs(p_top_radius);
    const float abs_base = std::abs(p_bottom_radius);
    const bool top_cap = abs_top > 1.0e-6f;
    const bool base_cap = abs_base > 1.0e-6f;
    const std::uint32_t ring = p_slices + 1u;

    std::uint32_t vertex_count = 2u * ring;
    if (top_cap)
    {
        vertex_count += ring + 1u;
    }
    if (base_cap)
    {
        vertex_count += ring + 1u;
    }
    if (vertex_count > kMaxVertices)
    {
        return failure("makeTube would exceed 65535 vertices");
    }

    std::vector<float> angle;
    std::vector<float> tex_u;
    compages::core::linspace(
        0.0f, compages::core::TWO_PI<float>, ring, angle, true);
    compages::core::linspace(0.0f, 1.0f, ring, tex_u, true);

    const float half_h = p_height * 0.5f;
    const float top_z = p_tip_along_negative_z ? -half_h : half_h;
    const float base_z = -top_z;
    const float delta_r = abs_top - abs_base;
    const float hypotenuse =
        std::sqrt((delta_r * delta_r) + (p_height * p_height));
    const float side_xy = p_height / hypotenuse;
    const float side_z = p_tip_along_negative_z ? (delta_r / hypotenuse)
                                                : (-delta_r / hypotenuse);

    std::vector<MeshVertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve(vertex_count);
    indices.reserve((6u * p_slices) +
                    (((top_cap ? 3u : 0u) + (base_cap ? 3u : 0u)) * p_slices));

    const std::uint16_t top_ring = 0u;
    addTubeRing(corners, angle, tex_u, abs_top, top_z, side_xy, side_z, 0.0f);
    const std::uint16_t base_ring = u16(corners.size());
    addTubeRing(corners, angle, tex_u, abs_base, base_z, side_xy, side_z, 1.0f);

    for (std::uint32_t i = 0u; i < p_slices; ++i)
    {
        const std::uint16_t t0 = u16(top_ring + i);
        const std::uint16_t t1 = u16(top_ring + i + 1u);
        const std::uint16_t b0 = u16(base_ring + i);
        const std::uint16_t b1 = u16(base_ring + i + 1u);
        // CCW from the outside: a +Z cylinder and a -Z cone are mirrors.
        if (p_tip_along_negative_z)
        {
            addTriangle(indices, t0, t1, b0);
            addTriangle(indices, b0, t1, b1);
        }
        else
        {
            addTriangle(indices, t0, b0, t1);
            addTriangle(indices, t1, b0, b1);
        }
    }

    if (top_cap)
    {
        const compages::core::Vector3f normal(
            0.0f, 0.0f, p_tip_along_negative_z ? -1.0f : 1.0f);
        addDiskCap(corners,
                   indices,
                   angle,
                   p_slices,
                   abs_top,
                   top_z,
                   normal,
                   !p_tip_along_negative_z);
    }
    if (base_cap)
    {
        const compages::core::Vector3f normal(
            0.0f, 0.0f, p_tip_along_negative_z ? 1.0f : -1.0f);
        addDiskCap(corners,
                   indices,
                   angle,
                   p_slices,
                   abs_base,
                   base_z,
                   normal,
                   p_tip_along_negative_z);
    }

    return uploadMesh(std::move(corners), std::move(indices));
}

//------------------------------------------------------------------------------
Result<MeshAsset> makeCone(float p_bottom_radius,
                           float p_top_radius,
                           float p_height,
                           std::uint32_t p_slices)
{
    return makeTube(p_top_radius, p_bottom_radius, p_height, p_slices, true);
}

//------------------------------------------------------------------------------
Result<MeshAsset>
makeCylinder(float p_radius, float p_height, std::uint32_t p_slices)
{
    return makeTube(p_radius, p_radius, p_height, p_slices, false);
}

//------------------------------------------------------------------------------
Result<MeshAsset> makePyramid(float p_radius, float p_height)
{
    return makeCone(p_radius, 0.0f, p_height, 4u);
}

//------------------------------------------------------------------------------
Result<Material> makeLitMaterial()
{
    return makeMaterial("Lit",
                        ShaderFamily::Lit,
                        shaders::litVertex(),
                        shaders::litFragment(),
                        false);
}

//------------------------------------------------------------------------------
Result<Material> makePbrMaterial()
{
    return makeMaterial("PBR",
                        ShaderFamily::PbrMinimal,
                        shaders::pbrVertex(),
                        shaders::pbrFragment(),
                        true);
}

//------------------------------------------------------------------------------
Result<Material> makeDepthMaterial()
{
    return makeMaterial("Depth",
                        ShaderFamily::Depth,
                        shaders::depthVertex(),
                        shaders::depthFragment(),
                        false);
}

//------------------------------------------------------------------------------
Result<Material> makeNormalsMaterial()
{
    return makeMaterial("Normals",
                        ShaderFamily::Normals,
                        shaders::normalsVertex(),
                        shaders::normalsFragment(),
                        false);
}

} // namespace compages::renderer
