// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_Workshop/Workshop/Workshop.hpp"

#include "Compages/World/Routines/Routine.hpp"

#include <cmath>

namespace examples::workshop
{

namespace
{

// ****************************************************************************
//! \brief Moves the three joints from the total time: the arm sweeps the
//! table, and its tool goes down to touch it and up again.
// ****************************************************************************
struct Sweep: compages::world::Routine
{
    Sweep(compages::world::Entity p_shoulder, compages::world::Entity p_elbow)
        : shoulder(p_shoulder), elbow(p_elbow)
    {
    }

    void update(float) override
    {
        using units::angle::radian_t;
        const double t = frame().total;
        entity().joint().angle(radian_t(0.7 * std::sin(0.5 * t)));
        shoulder.joint().angle(radian_t(-1.0 - 0.12 * std::sin(0.9 * t)));
        elbow.joint().angle(radian_t(-1.5 - 0.18 * std::sin(1.3 * t)));
    }

    compages::world::Entity shoulder;
    compages::world::Entity elbow;
};

} // namespace

// ----------------------------------------------------------------------------
Arm buildArm(compages::renderer::Scene& p_scene)
{
    using compages::renderer::color;
    const compages::renderer::Look orange = color(0.95f, 0.45f, 0.08f);
    const compages::renderer::Look grey = color(0.25f, 0.25f, 0.27f);

    // The joints are empty entities, each a child of the one before: turning
    // one turns everything after it. A revolute joint turns round one axis.
    Arm arm;
    arm.base = p_scene.world().entity("Arm").position(-0.45f, TABLE_TOP, 0.0f).revolute({ 0.0f, 1.0f, 0.0f });
    arm.shoulder = arm.base.child("Shoulder").position(0.0f, 0.12f, 0.0f).revolute({ 0.0f, 0.0f, 1.0f });
    arm.elbow = arm.shoulder.child("Elbow").position(0.0f, 0.5f, 0.0f).revolute({ 0.0f, 0.0f, 1.0f });
    arm.tool = arm.elbow.child("Tool").position(0.0f, 0.44f, 0.0f);

    // The shapes hang under their joint. A link is moved half its length up,
    // so that it turns round its end and not round its middle.
    arm.links.push_back(p_scene.cylinder("Foot", grey).parent(arm.base).position(0.0f, 0.05f, 0.0f).scale(0.24f, 0.1f, 0.24f));
    arm.links.push_back(p_scene.box("Upper arm", orange).parent(arm.shoulder).position(0.0f, 0.25f, 0.0f).scale(0.08f, 0.5f, 0.08f));
    arm.links.push_back(p_scene.box("Forearm", orange).parent(arm.elbow).position(0.0f, 0.2f, 0.0f).scale(0.06f, 0.4f, 0.06f));
    p_scene.sphere("Elbow joint", grey).parent(arm.elbow).scale(0.1f);
    p_scene.cone("Tip", grey).parent(arm.tool).position(0.0f, -0.02f, 0.0f).scale(0.05f, -0.06f, 0.05f);

    // The routine runs at each update of the World, inside Scene::draw().
    arm.base.add<Sweep>(arm.shoulder, arm.elbow);
    return arm;
}

} // namespace examples::workshop
