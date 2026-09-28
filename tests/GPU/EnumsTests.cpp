// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/GPU/Core/Enums.hpp"

#include <set>
#include <string>

// Names end up in error messages, so a stage described by the wrong word sends
// the reader looking at the wrong shader. Every stage must have its own name.
TEST(Enums, NamesEveryShaderStageDistinctly)
{
    const compages::gpu::ShaderStage stages[] = {
        compages::gpu::ShaderStage::Vertex,
        compages::gpu::ShaderStage::Fragment,
        compages::gpu::ShaderStage::Geometry,
        compages::gpu::ShaderStage::TessellationControl,
        compages::gpu::ShaderStage::TessellationEvaluation,
        compages::gpu::ShaderStage::Compute,
    };
    ASSERT_EQ(std::size(stages), compages::gpu::SHADER_STAGE_COUNT);

    std::set<std::string> names;
    for (auto const& stage : stages)
    {
        const std::string name = compages::gpu::toString(stage);
        ASSERT_FALSE(name.empty());
        ASSERT_NE(name, "unknown");
        names.insert(name);
    }
    ASSERT_EQ(names.size(), compages::gpu::SHADER_STAGE_COUNT);
}

TEST(Enums, NamesEveryBufferKindDistinctly)
{
    const compages::gpu::BufferKind kinds[] = {
        compages::gpu::BufferKind::Vertex,
        compages::gpu::BufferKind::Index,
        compages::gpu::BufferKind::Uniform,
        compages::gpu::BufferKind::Storage,
    };

    std::set<std::string> names;
    for (auto const& kind : kinds)
    {
        const std::string name = compages::gpu::toString(kind);
        ASSERT_FALSE(name.empty());
        ASSERT_NE(name, "unknown");
        names.insert(name);
    }
    ASSERT_EQ(names.size(), std::size(kinds));
}

// A draw call checks the vertex count against this, so the separate kinds must
// report their own count and the strips must report that the rule does not apply
// to them.
TEST(Enums, KnowsHowManyVerticesAPrimitiveNeeds)
{
    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::Points), 1u);
    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::Lines), 2u);
    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::Triangles), 3u);

    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::LineStrip), 0u);
    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::LineLoop), 0u);
    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::TriangleStrip), 0u);
    ASSERT_EQ(compages::gpu::verticesPerPrimitive(compages::gpu::Primitive::TriangleFan), 0u);
}

// Getting this wrong reads the index buffer at the wrong stride, which draws
// garbage rather than failing, so it is worth pinning down.
TEST(Enums, KnowsTheSizeOfAnIndex)
{
    ASSERT_EQ(compages::gpu::sizeOf(compages::gpu::IndexType::UInt16), 2u);
    ASSERT_EQ(compages::gpu::sizeOf(compages::gpu::IndexType::UInt32), 4u);

    static_assert(compages::gpu::sizeOf(compages::gpu::IndexType::UInt16) == 2u);
    static_assert(compages::gpu::sizeOf(compages::gpu::IndexType::UInt32) == 4u);
}
