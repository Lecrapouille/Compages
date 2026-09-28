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
//! \brief A picture with no mesh at all.
//!
//! The vertex shader builds a triangle covering the target from the index of
//! the vertex it is running for. There is no buffer, no layout and no upload,
//! which is what every fullscreen effect wants: the work is in the fragment
//! shader, and a vertex would only be something to forget to update.
//!
//! The mouse is the point the zoom keeps still. Moving it chooses a place in
//! the complex plane; time closes in on it. That is a uniform changing every
//! frame, not a vertex moving.
// ****************************************************************************
class Mandelbrot: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06a_Mandelbrot";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    //! \brief A shader and no vertices: draw(3u) runs the vertex shader three
    //! times, and it makes the corners itself.
    compages::gpu::Drawable m_screen;

    //! \brief Where the view is looking in the complex plane. Updated so that
    //! the point under the mouse stays there as the scale shrinks.
    Vector2f m_center{ -0.5f, 0.0f };
    float m_scale = 1.5f;
};

} // namespace examples
