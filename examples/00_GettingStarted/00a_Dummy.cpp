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

#include "00_GettingStarted/00a_Dummy.hpp"

namespace examples
{

//------------------------------------------------------------------------------
std::string Dummy::description() const
{
    return "The smallest example: a name, this sentence, a setUp() that builds "
           "nothing and a draw() that clears the picture. Everything else, the "
           "window, the loop, the panels, belongs to the gallery.";
}

//------------------------------------------------------------------------------
void Dummy::draw(Frame const&)
{
    // The window is already a pass: the gallery opened it before calling draw.
    compages::gpu::clear({ 0.035f, 0.04f, 0.05f });
}

} // namespace examples
