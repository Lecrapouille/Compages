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
//! \brief A curve that grows a point per step, and only the new points travel.
//!
//! HeightMap rewrote every vertex. This one appends: emplace_back() marks the
//! new end of the vertices, so the next draw sends the points added since the
//! last one rather than the whole trail:
//! \code
//! m_trail.emplace_back(Vertex{ point, color });
//! m_trail.draw();                   // a line strip through every point
//! \endcode
//!
//! The GPU buffer grows by doubling, so twenty thousand points cost about
//! fifteen reallocations, not twenty thousand.
// ****************************************************************************
class Lorenz: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "15_Lorenz";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Vertex
    {
        compages::core::Vector3f position;
        compages::core::Vector3f color;
    };

    //! \brief One step of the attractor, appended to the trail.
    void step();

    compages::gpu::Drawable m_trail;
    compages::core::Vector3f m_state{ 0.1f, 0.0f, 0.0f };
};

} // namespace examples
