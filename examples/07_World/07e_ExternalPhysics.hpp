// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

#include <array>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Plug a physics engine into Compages.
//!
//! Compages has no physics engine. The engine (MuJoCo, Bullet, Jolt, or the
//! tiny one of this demo) keeps its own bodies. A routine steps it in
//! fixedUpdate(), at a fixed pace whatever the frame rate, writes the poses
//! into the entities (marked PoseDriver::External, so that no Compages
//! system writes over them) and sends a Contact message for each shock. A
//! routine on each ball swells at a shock.
//! \code
//! ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });
//!
//! void fixedUpdate(float p_dt) override             // in the Physics routine
//! {
//!     engine.step(p_dt);                             // the engine moves
//!     ball.position(p.x, p.y, p.z);                  // its result goes in
//!     world().routines().send(ball, Contact{ ... }); // and its shocks
//! }
//! \endcode
// ****************************************************************************
class ExternalPhysics final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "07e_ExternalPhysics";
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

        //! \brief A shock of the last step: body \c a against body \c b, or
        //! against the box when \c b is negative.
        struct Hit
        {
            std::size_t a;
            int b;
            std::array<float, 3> point;
            //! \brief Pushes \c a away from what it hit.
            std::array<float, 3> normal;
            float speed;
        };

        void step(float p_dt);
        void collide();
        void hit(std::size_t p_a, int p_b, std::array<float, 3> p_normal, float p_speed);

        std::vector<Body> bodies;
        std::vector<Hit> hits;
        float gravity = -9.81f;
        float bounce = 0.8f;
        float half_box = 3.0f;
    };

    struct Physics;
    struct Squash;

    void throwBalls();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    TinyEngine m_engine;
    //! \brief The entity of each body, at the same index.
    std::vector<compages::world::Entity> m_balls;
    //! \brief Shocks sent as messages since the start.
    int m_shocks = 0;
};

} // namespace examples
