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
//! \brief The triangle of 01b_Triangle, from a C++ struct.
//!
//! When the corners are computed rather than typed, a struct is their natural
//! shape. Nothing has to describe it: the compiler reads its fields (name,
//! type, place in the struct) and each field feeds the shader input of the
//! same name. A field named differently can be renamed:
//! \code
//! m_triangle.setVertices<Vertex>(corners,
//!     compages::gpu::VertexLayout::of<Vertex>().rename("position",
//!     "aPosition"));
//! \endcode
//!
//! The drawable keeps a copy of the corners and sends it at the first draw.
//! 01d_BorrowedTriangle keeps them in a Buffer of our own instead, and
//! 02a_DynamicGeometry changes one of them every frame.
// ****************************************************************************
class InterleavedTriangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "01c_InterleavedTriangle";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief One corner: the field names are the names the shader declares.
    struct Vertex
    {
        compages::Vector2f position;
        compages::Vector3f color;
    };

    compages::gpu::Drawable m_triangle;
};

} // namespace examples
