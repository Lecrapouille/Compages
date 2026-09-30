// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/ColoredCube.hpp"
#include "Common/Example.hpp"

// ****************************************************************************
//! \brief The three matrices every pass of a frame reads.
//!
//! Written once, read by three programs. That is the reason a uniform block
//! exists: Program::set() would have copied them three times, into three places
//! the driver keeps.
//!
//! GPU_STD140 is at file scope on purpose. It specialises a template in
//! compages::gpu::std140, which a type hidden in a class or an anonymous
//! namespace cannot do, and the compiler would then treat the struct as never
//! described.
// ****************************************************************************
struct Transforms
{
    compages::core::Matrix44f projection;
    compages::core::Matrix44f view;
    compages::core::Matrix44f model;
};
GPU_STD140(Transforms, projection, view, model);

namespace examples
{

// ****************************************************************************
//! \brief One mesh, three ways of drawing it, one block of matrices: the level
//! beneath compages::gpu::Drawable.
//!
//! A Drawable owns its shader and its vertices. When one mesh is drawn by
//! several shaders, the pieces are held apart instead: one Buffer<Vertex>, one
//! Program per way of drawing, and one Pipeline per program, which checks at
//! set up that the vertex can feed that shader:
//! \code
//! COMPAGES_TRY(m_lit_program.load(LIT_VERTEX, LIT_FRAGMENT));
//! COMPAGES_TRY_ASSIGN(m_lit,
//! compages::gpu::Pipeline::create<Vertex>(m_lit_program, solid));
//! ...
//! compages::gpu::drawIndexed(m_lit, m_vertices, m_indices);
//! \endcode
//! The lit pass reads every field, the normals pass drops the colour the
//! linker never needed, the wireframe drops both; the overlay's vertex-reader
//! count is what says the buffer really is shared.
//!
//! The matrices are a typed uniform block. Written once per frame, bound once,
//! read by all three programs from the same numbered point.
// ****************************************************************************
class MultiPassMesh: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "05a_MultiPassMesh";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief The corner of the cube of Common/ColoredCube.hpp.
    using Vertex = CubeVertex;

    // ------------------------------------------------------------------------
    //! \brief Load one of the three programs and build its pipeline, which
    //! checks the vertex can feed it. The program must outlive the pipeline.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::Status
    makePipeline(compages::gpu::Program& p_program,
                 compages::gpu::Pipeline& p_pipeline,
                 char const* p_vertex,
                 char const* p_fragment,
                 compages::gpu::RenderState const& p_state);

    compages::gpu::Program m_lit_program;
    compages::gpu::Program m_normals_program;
    compages::gpu::Program m_wire_program;

    compages::gpu::Pipeline m_lit;
    compages::gpu::Pipeline m_normals;
    compages::gpu::Pipeline m_wire;

    //! \brief Kept on the CPU until the first draw sends it.
    compages::gpu::Buffer<Vertex> m_vertices;
    compages::gpu::Buffer<std::uint16_t> m_indices;
    compages::gpu::TypedUniformBlock<Transforms> m_transforms;
};

} // namespace examples
