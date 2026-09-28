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
//! \brief One triangle, from a shader and its attributes given by name.
//!
//! A compages::gpu::Drawable is a shader with its data. The shader declares what a
//! vertex is, so the values are given attribute by attribute:
//! \code
//! m_triangle["position"] = { {-0.8f, -0.6f}, {0.8f, -0.6f}, {0.0f, 0.8f} };
//! \endcode
//! and stored interleaved, one whole vertex after the other, which is what the
//! GPU reads fastest. No buffer, layout or pipeline to write: the drawable
//! derives them from the shader and sends the vertices at the first draw.
//!
//! 01c_InterleavedTriangle draws the same triangle from a C++ struct.
// ****************************************************************************
class Triangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "01b_Triangle";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::gpu::Drawable m_triangle;
};

} // namespace examples
