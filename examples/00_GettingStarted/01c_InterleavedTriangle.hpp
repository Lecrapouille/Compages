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

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief The triangle of 01b_Triangle, from a C++ struct.
//!
//! When vertices are computed rather than typed, a struct is their natural
//! shape. Nothing has to describe it: its fields are read off the struct by
//! the compiler, name, type and offset, and matched to the shader attributes
//! by name. A field named otherwise than the attribute it feeds is renamed:
//! \code
//! m_triangle.vertices<Vertex>(corners,
//!     compages::gpu::VertexLayout::of<Vertex>().rename("position",
//!     "aPosition"));
//! \endcode
//!
//! These vertices never change, so they go straight into a buffer made
//! with compages::gpu::BufferUsage::Immutable: filled once when it is created,
//! which lets the driver keep it where the GPU reads fastest, and never written
//! again. The drawable reads that buffer where it is rather than keeping a
//! copy of its own:
//! \code
//! COMPAGES_TRY_ASSIGN(m_vertices, compages::gpu::Buffer<Vertex>::from(corners,
//!     { .usage = compages::gpu::BufferUsage::Immutable, .cpu_mirror = false
//!     }));
//! m_triangle.vertices(m_vertices);
//! \endcode
//! 02_DynamicGeometry does the opposite: vertices that change every frame.
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
        compages::core::Vector2f position;
        compages::core::Vector3f color;
    };

    //! \brief Read by m_triangle, which does not own it: declared first so
    //! that it is destroyed last.
    compages::gpu::Buffer<Vertex> m_vertices;
    compages::gpu::Drawable m_triangle;
};

} // namespace examples
