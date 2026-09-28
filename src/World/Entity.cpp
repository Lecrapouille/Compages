// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"

namespace compages::world
{

Entity Entity::child(std::string p_name) const
{
    Entity kid = world().entity(std::move(p_name));
    kid.parent(*this);
    return kid;
}

} // namespace compages::world
