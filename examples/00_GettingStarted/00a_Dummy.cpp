// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/00a_Dummy.hpp"

namespace examples
{

std::string Dummy::description() const
{
    return "The smallest example: a name, this sentence, a setUp() that builds "
           "nothing and a draw() that clears the picture. Everything else, the "
           "window, the loop, the panels, belongs to the gallery.";
}

void Dummy::draw(Frame const&)
{
    // The window is already a pass: the gallery opened it before calling draw.
    compages::gpu::clear({ 0.035f, 0.04f, 0.05f });
}

} // namespace examples
