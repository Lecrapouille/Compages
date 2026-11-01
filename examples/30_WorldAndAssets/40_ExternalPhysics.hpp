// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"

#include <array>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Plug a physics engine into Compages.
//!
//! Compages has no physics engine. The engine (MuJoCo, Bullet, Jolt, or the
//! tiny one of this demo) keeps its own bodies; at each step it writes their
//! pose into the entities, which are marked PoseDriver::External so that no
//! Compages system writes over them. The scene then computes the world
//! matrices and draws them.
//! \code
//! ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });
//!
//! engine.step(dt);                                // the engine moves
//! ball.position(toCompages(engine.position(i)));  // its result goes in
//! scene.draw(frame);                              // Compages shows it
//! \endcode
// ****************************************************************************
class ExternalPhysics final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "40_ExternalPhysics";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief The "external engine": it knows nothing about Compages.
    struct TinyEngine
    {
        struct Body
        {
            std::array<float, 3> position;
            std::array<float, 3> velocity;
            float radius;
        };

        void step(float p_dt);

        std::vector<Body> bodies;
        float gravity = -9.81f;
        float bounce = 0.8f;
        float half_box = 3.0f;
    };

    void throwBalls();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    TinyEngine m_engine;
    //! \brief The entity of each body, at the same index.
    std::vector<compages::world::Entity> m_balls;
};

} // namespace examples
