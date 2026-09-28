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

#include "Common/ColoredCube.hpp"
#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A solid in three dimensions: indices, depth, culling and matrices.
//!
//! Everything the previous four examples left out arrives at once, because these
//! four things are what turn a flat picture into a solid and they are useless one
//! at a time.
//!
//! Indices, so that a corner shared by several triangles is stored once and named
//! several times. Depth testing, so that a face nearer the eye covers one further
//! away whatever order they were drawn in. Back face culling, so that half the
//! triangles of a closed shape are dropped before they cost anything. And three
//! matrices, so that a shape described once can be placed, looked at, and
//! projected.
//!
//! The depth test and the culling live in the render state of the drawable:
//! \code
//! m_cube.depthTest().cull(compages::gpu::CullMode::Back);
//! \endcode
//! They are not switched on before the draw and forgotten afterwards: two
//! drawables wanting different ones cannot leave the device in a state the
//! other did not ask for.
//!
//! The cube itself, twenty four corners and thirty six indices, is built by
//! createCube() in Common/ColoredCube.hpp, shared with the 05 examples.
// ****************************************************************************
class IndexedCube: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "04_DepthAndTransforms";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::gpu::Drawable m_cube;
};

} // namespace examples
