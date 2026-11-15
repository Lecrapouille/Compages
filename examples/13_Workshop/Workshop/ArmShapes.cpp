// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Workshop.hpp"

namespace examples::workshop
{

// ----------------------------------------------------------------------------
//! The joints have no shape. The shapes hang under them: a link is moved up
//! by half its length, so that it turns round its end and not round its
//! middle.
// ----------------------------------------------------------------------------
std::vector<compages::world::Entity> dressArm(compages::renderer::Scene& p_scene, Arm const& p_arm)
{
    using compages::renderer::color;
    const compages::renderer::Look orange = color(0.95f, 0.42f, 0.06f);
    const compages::renderer::Look steel = color(0.30f, 0.31f, 0.33f);

    p_scene.cylinder("Foot", steel).parent(p_arm.base).position(0.0f, 0.05f, 0.0f).scale(0.24f, 0.1f, 0.24f);
    p_scene.sphere("Shoulder joint", steel).parent(p_arm.shoulder).scale(0.11f);
    p_scene.sphere("Elbow joint", steel).parent(p_arm.elbow).scale(0.09f);
    const float tip = 0.06f;
    p_scene.cone("Tip", steel).parent(p_arm.elbow).position(0.0f, FOREARM - (0.5f * tip), 0.0f).scale(0.04f, tip, 0.04f);

    std::vector<compages::world::Entity> links;
    links.push_back(p_scene.box("Upper arm", orange)
                        .parent(p_arm.shoulder)
                        .position(0.0f, 0.5f * UPPER_ARM, 0.0f)
                        .scale(0.08f, UPPER_ARM, 0.08f));
    links.push_back(p_scene.box("Forearm", orange)
                        .parent(p_arm.elbow)
                        .position(0.0f, 0.5f * (FOREARM - tip), 0.0f)
                        .scale(0.06f, FOREARM - tip, 0.06f));
    return links;
}

} // namespace examples::workshop
