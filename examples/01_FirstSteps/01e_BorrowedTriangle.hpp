// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief The triangle of 01d_InterleavedTriangle, in a Buffer that two
//! drawables read without copying it.
//!
//! Until now the drawable owned its corners. Here we own them: they live in a
//! compages::gpu::Buffer, a block of graphics card memory that we keep as a
//! member. Each drawable only borrows it, so the corners exist once on the
//! card however many drawables read them:
//! \code
//! COMPAGES_TRY_ASSIGN(m_corners, compages::gpu::VertexBuffer<Vertex>::from(
//!     corners, compages::gpu::BufferUsage::Static));
//! m_fill.borrowVertices(m_corners);      // the coloured triangle
//! m_outline.borrowVertices(m_corners);   // its white edges, same corners
//! \endcode
//!
//! Borrow only in two cases: the corners are shared, as here, or the graphics
//! card makes them itself (05d_ComputeParticles). Otherwise let the drawable
//! own them, as in 01c. A borrowed Buffer must live longer than the drawables
//! that read it.
// ****************************************************************************
class BorrowedTriangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "01e_BorrowedTriangle";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief One corner, as in 01c.
    struct Vertex
    {
        compages::Vector2f position;
        compages::Vector3f color;
    };

    //! \brief Ours, read by both drawables. Declared first, so that it is
    //! destroyed last, after the drawables that borrow it.
    compages::gpu::VertexBuffer<Vertex> m_corners;
    compages::gpu::Drawable m_fill;
    compages::gpu::Drawable m_outline;
};

} // namespace examples
