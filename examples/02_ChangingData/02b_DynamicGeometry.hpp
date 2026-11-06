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
//! \brief Corners that move, and only what moved travels.
//!
//! The same struct as 01d_InterleavedTriangle. The drawable keeps the
//! corners on the CPU and notices which ones were edited; those, and only
//! those, travel to the graphics card at the next draw. Here the top corner
//! follows the mouse and the two others never move, so one corner out of
//! three is sent per frame:
//! \code
//! m_triangle.setVertices<Vertex>({ ... });
//! m_triangle.editVertex<Vertex>(APEX).position = mouse;   // marked
//! m_triangle.draw();   // sends that corner, then draws
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
        return "02b_DynamicGeometry";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Vertex
    {
        compages::Vector2f position;
        compages::Vector3f color;
    };

    compages::gpu::Drawable m_triangle;
};

} // namespace examples
