// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

namespace compages::core
{
// ****************************************************************************
//! \brief Base for unique owners (World, stores, GPU handles): no copy, move
//! OK.
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

} // namespace compages::core

