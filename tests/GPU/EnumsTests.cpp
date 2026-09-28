//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/GPU/Core/Enums.hpp"

#include <set>
#include <string>

//------------------------------------------------------------------------------
// Names end up in error messages, so a stage described by the wrong word sends
// the reader looking at the wrong shader. Every stage must have its own name.
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// A draw call checks the vertex count against this, so the separate kinds must
// report their own count and the strips must report that the rule does not apply
// to them.
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Getting this wrong reads the index buffer at the wrong stride, which draws
// garbage rather than failing, so it is worth pinning down.
//------------------------------------------------------------------------------
TEST(Enums, KnowsTheSizeOfAnIndex)
{
    ASSERT_EQ(compages::gpu::sizeOf(compages::gpu::IndexType::UInt16), 2u);
    ASSERT_EQ(compages::gpu::sizeOf(compages::gpu::IndexType::UInt32), 4u);

    static_assert(compages::gpu::sizeOf(compages::gpu::IndexType::UInt16) == 2u);
    static_assert(compages::gpu::sizeOf(compages::gpu::IndexType::UInt32) == 4u);
}
