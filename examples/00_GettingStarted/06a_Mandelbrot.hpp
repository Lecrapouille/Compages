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
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief A shader and no vertices: draw(3u) runs the vertex shader three
    //! times, and it makes the corners itself.
    compages::gpu::Drawable m_screen;

    //! \brief Where the view is looking in the complex plane. Updated so that
    //! the point under the mouse stays there as the scale shrinks.
    compages::core::Vector2f m_center{ -0.5f, 0.0f };
    float m_scale = 1.5f;
};

} // namespace examples
