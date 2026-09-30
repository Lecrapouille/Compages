// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Three robots of cubes: a hierarchy of transforms, posed by a
//! behavior.
//!
//! A robot is a tree. Its joints are empty entities placed where a part
//! turns, a shoulder or a neck, and the boxes hang under them; turning a
//! joint turns everything under it, and the head does not know it is on a
//! turning body:
//! \code
//! compages::world::Entity shoulder = body.child("LeftShoulder").position(-13,
//! 15, 0); m_scene.box("LeftArm", dark).parent(shoulder).position(0, -12,
//! 0).scale(6, 24, 6);
//! \endcode
//!
//! Only the boxes are scaled, never the joints: a scale on a joint would
//! squash every part under it. The motion is a behavior on the root of each
//! robot, written from the total time so that it never drifts.
// ****************************************************************************
class MovingRobot final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "31_MovingRobot";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Walk;

    void makeRobot(char const* p_name, float p_x, float p_phase);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
