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
//! \brief The whole of a frame, with nothing drawn in it.
//!
//! What this shows is a pass: where in the window a frame goes, and what it starts
//! from. Two of them, in fact, one into each half of the window, because the
//! second one is what makes it clear that a pass is a region and not "the screen".
//!
//! What it does not show is any resource at all. Watching the counters in the
//! overlay stay at zero while this runs is the point: clearing the window costs
//! nothing and holds nothing.
// ****************************************************************************
class ClearScreen: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "01a_ClearScreen";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;
};

} // namespace examples
