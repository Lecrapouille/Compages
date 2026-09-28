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
