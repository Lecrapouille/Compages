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

// ****************************************************************************
//! \brief Base for unique owners (World, stores, GPU handles): no copy, move OK.
//!
//! Inherit privately so the restriction does not leak to the public interface.
//! \code
//! class World : private NotCopiable { ... };
//! \endcode
// ****************************************************************************
class NotCopiable
{
protected:

    NotCopiable() = default;
    NotCopiable(NotCopiable const&) = delete;
    NotCopiable& operator=(NotCopiable const&) = delete;
    NotCopiable(NotCopiable&&) = default;
    NotCopiable& operator=(NotCopiable&&) = default;
    ~NotCopiable() = default;
};
