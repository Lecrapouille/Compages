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
//! \brief Vertices that move, sent as cheaply as they can be.
//!
//! A compages::gpu::Drawable keeps its vertices on the CPU and notices which
//! ones were written: those, and only those, travel to the device on the next
//! draw. Here the apex of the triangle follows the mouse and the two feet never
//! move, so one vertex out of three is sent per frame:
//! \code
//! m_triangle.vertex<Vertex>(APEX).position = compages::world::mouseInClipSpace(p_frame);
//! m_triangle.draw();   // sends that vertex, then draws
//! \endcode
//!
//! That is the shape of every demo computing something on the CPU and showing
//! it: a curve being integrated, a mesh being deformed, a plot growing a point
//! at a time (emplace_back() works too).
// ****************************************************************************
class DynamicTriangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "02_DynamicGeometry";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Vertex
    {
        compages::core::Vector2f position;
        compages::core::Vector3f color;
    };

    compages::gpu::Drawable m_triangle;
};

} // namespace examples
