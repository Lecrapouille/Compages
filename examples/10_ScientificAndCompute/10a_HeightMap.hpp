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
//! \brief A surface the CPU recomputes, a hundred thousand vertices at a time.
//!
//! The height of every point is a function of its place and of time, decided
//! on the CPU each frame. The vertices are changed where they are, and the
//! whole array travels at the next draw because the whole array changed:
//! \code
//! for (Vertex& v : m_surface.vertices<Vertex>())
//! {
//!     v.position.y = heightAt(v.position.x, v.position.z, time);
//! }
//! m_surface.draw();
//! \endcode
//!
//! The triangles naming those points never move: the indices are given once
//! and stay on the GPU. 02_DynamicGeometry changed one vertex; this changes all
//! of them. Only the size of what is sent differs, not the code.
// ****************************************************************************
class HeightMap: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "10a_HeightMap";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Vertex
    {
        compages::core::Vector3f position;
        compages::core::Vector3f normal;
    };

    compages::gpu::Drawable m_surface;
};

} // namespace examples
