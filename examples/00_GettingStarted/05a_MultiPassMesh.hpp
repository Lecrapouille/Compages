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
//! Written once, read by three shaders. That is the reason a uniform block
//! exists: a plain uniform would have been copied three times, once into each
//! shader.
//!
//! GPU_STD140 is at file scope on purpose. It specialises a template in
//! compages::gpu::std140, which a type hidden in a class or an anonymous
//! namespace cannot do, and the compiler would then treat the struct as never
//! described.
// ****************************************************************************
struct Transforms
{
    compages::Matrix44f projection;
    compages::Matrix44f view;
    compages::Matrix44f model;
};
GPU_STD140(Transforms, projection, view, model);

namespace examples
{

// ****************************************************************************
//! \brief One mesh, three ways of drawing it, one block of matrices.
//!
//! The cube lives once on the graphics card, in a Buffer we own. Three
//! drawables borrow it, each with its own shader: one lights the cube, one
//! paints its normals, one draws only its edges. This is the "shared" case of
//! 01d_BorrowedTriangle, on a real mesh:
//! \code
//! COMPAGES_TRY_ASSIGN(m_vertices, compages::gpu::VertexBuffer<Vertex>::from(
//!     cube.vertices, compages::gpu::BufferUsage::Static));
//! for (compages::gpu::Drawable* pass : { &m_lit, &m_normals, &m_wire })
//! {
//!     pass->borrowVertices(m_vertices);
//! }
//! \endcode
//! Each shader reads only the fields it needs: the lit pass reads all of
//! them, the normals pass skips the colour, the wireframe skips both.
//!
//! The matrices are a typed uniform block. Written once per frame, bound once,
//! read by all three shaders from the same numbered point.
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

    //! \brief Ours, read by the three drawables. Declared first, so that it
    //! is destroyed after them.
    compages::gpu::VertexBuffer<Vertex> m_vertices;

    compages::gpu::Drawable m_lit;
    compages::gpu::Drawable m_normals;
    compages::gpu::Drawable m_wire;
    compages::gpu::TypedUniformBlock<Transforms> m_transforms;
};

} // namespace examples
