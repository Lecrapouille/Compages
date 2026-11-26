// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <string>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Characters that walk from point to point, like the people of a
//! video game: they turn, stop, wait and go again.
//!
//! Two calls make a walk look right. play() with a fade mixes the old clip
//! into the new one, so Walk becomes Idle without a jump of the pose:
//! \code
//! m_scene.play(walker.body, "Idle", 0.3_s);
//! \endcode
//! groundSpeed() says how fast the feet of a clip made in place go back on
//! the ground; the clip plays faster or slower so that the feet stay where
//! they touch the floor while the body moves:
//! \code
//! walker.stride = m_scene.groundSpeed(walker.body, "Walk");
//! animator.speed = walker.speed / walker.stride;
//! \endcode
// ****************************************************************************
class Walkers final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "08e_Walkers";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief One character and where it goes.
    struct Walker
    {
        compages::world::Entity body;
        //! \brief Points on the floor, visited in a loop.
        std::vector<compages::Vector3f> route;
        std::size_t next = 0u;
        //! \brief The clip that moves it: "Walk" or "Run".
        std::string gait;
        //! \brief The speed it moves at.
        compages::si::LinearVelocity speed{ 0.0 };
        //! \brief The ground speed of its gait clip, measured once.
        compages::si::LinearVelocity stride{ 0.0 };
        //! \brief Where it faces, around the vertical axis.
        compages::si::Radians heading{ 0.0 };
        //! \brief Time left to stand at a point.
        compages::si::Seconds waiting{ 0.0 };
    };

    void step(Walker& p_walker, compages::si::Seconds p_dt);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<Walker> m_walkers;
    //! \brief Fade between clips; zero shows the jump it hides.
    float m_fade = 0.3f;
    //! \brief Plays the clips at speed 1 whatever the walking speed: the feet
    //! slide.
    bool m_slide = false;
};

} // namespace examples
