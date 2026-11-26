// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "08_Assets/08e_Walkers.hpp"
#include "Common/DataPath.hpp"

#include "Common/Gui.hpp"

#include "Compages/Renderer/Animation/Animator.hpp"

using namespace compages::si::literals;

namespace examples
{

//! \brief How fast a walker turns to face its next point, at most.
static constexpr compages::si::AngularVelocity TURN_RATE = 3.0_rad_per_s;
//! \brief The tightest circle a walker turns on. A clip made in place has no
//! turn on the spot: the body turns while the legs step, on an arc.
static constexpr compages::si::Length TURN_RADIUS = 0.4_m;
//! \brief The slowest a walker goes while it turns, as a part of its speed:
//! never zero, or the legs stop and the body slides round.
static constexpr float SLOWEST = 0.45f;
//! \brief How long a walker stands at each point.
static constexpr compages::si::Seconds PAUSE = 2.0_s;

// ----------------------------------------------------------------------------
std::string Walkers::description() const
{
    return "Three Soldier.glb walk from point to point: they turn, stop, wait "
           "and go again. play(model, clip, fade) mixes Walk into Idle; "
           "groundSpeed() gives the speed at which the feet do not slide. "
           "Needs Soldier.glb in external/Compages-data/.";
}

// ----------------------------------------------------------------------------
compages::Status Walkers::setUp()
{
    const std::filesystem::path path = dataPath("Soldier.glb");
    if (path.empty())
    {
        return compages::failure("Soldier.glb is missing: run make download in "
                                 "external/, or set COMPAGES_DATA_PATH");
    }
    m_scene.background(0.55f, 0.62f, 0.70f);
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.6f);
    m_scene.font(dataPath("fonts/DejaVuSans.ttf"));
    m_scene.plane("Floor", compages::renderer::color(0.45f, 0.45f, 0.42f))
        .rotation(-90.0_deg, compages::Vector3f(1.0f, 0.0f, 0.0f))
        .scale(16.0f);

    // Who goes where, at which speed and with which clip.
    struct Plan
    {
        char const* name;
        std::vector<compages::Vector3f> route;
        char const* gait;
        compages::si::LinearVelocity speed;
    };
    const std::vector<Plan> plans = {
        { "slow", { { -4.0f, 0.0f, -3.0f }, { 4.0f, 0.0f, -3.0f } }, "Walk", 1.1_mps },
        { "brisk",
          { { -3.0f, 0.0f, 3.0f }, { 3.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } },
          "Walk", 1.6_mps },
        { "runner",
          { { -6.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 6.0f }, { 6.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -6.0f } },
          "Run", 3.2_mps },
    };
    for (Plan const& plan : plans)
    {
        Walker walker;
        COMPAGES_TRY_ASSIGN(walker.body, m_scene.loadFile(path));
        walker.body.name(plan.name);
        walker.body.position(plan.route.back());
        walker.body.set(compages::renderer::Label{ .text = plan.name,
                                                   .color = { 1.0f, 1.0f, 1.0f },
                                                   .pixels = 14.0f,
                                                   .hide_closer_than = 0.0f });
        walker.route = plan.route;
        walker.gait = plan.gait;
        walker.speed = plan.speed;
        // Measured on the clip, once: the speed of the foot on the floor.
        walker.stride = m_scene.groundSpeed(walker.body, plan.gait);
        if (walker.stride <= 0.0_mps)
        {
            return compages::failure(std::string("Soldier.glb: no ground speed for ") + plan.gait);
        }
        m_scene.play(walker.body, plan.gait);
        m_walkers.push_back(std::move(walker));
    }
    m_scene.camera().position(0.0f, 9.0f, 13.0f).lookAt(0.0f, 0.0f, 0.0f).add<compages::world::Orbit>();
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Walkers::step(Walker& p_walker, compages::si::Seconds p_dt)
{
    const compages::si::Seconds fade(static_cast<double>(m_fade));
    auto& animator = p_walker.body.get<compages::renderer::Animator>();
    if (p_walker.waiting > 0.0_s)
    {
        p_walker.waiting -= p_dt;
        if (p_walker.waiting <= 0.0_s)
        {
            m_scene.play(p_walker.body, p_walker.gait, fade);
        }
        return;
    }

    // On the floor, from where it is to where it goes.
    const compages::Vector3f here = p_walker.body.position();
    const compages::Vector3f to = p_walker.route[p_walker.next] - here;
    const float distance = std::sqrt((to.x * to.x) + (to.z * to.z));
    if (distance < 0.15f)
    {
        p_walker.next = (p_walker.next + 1u) % p_walker.route.size();
        p_walker.waiting = PAUSE;
        animator.speed = 1.0f;
        m_scene.play(p_walker.body, "Idle", fade);
        return;
    }

    // The model looks down -Z: a heading h faces (-sin h, 0, -cos h).
    const compages::si::Radians wanted = units::math::atan2(compages::si::Length(-to.x), compages::si::Length(-to.z));
    compages::si::Radians error = wanted - p_walker.heading;
    while (error > 180.0_deg)
    {
        error -= 360.0_deg;
    }
    while (error < -180.0_deg)
    {
        error += 360.0_deg;
    }
    // Slower while it still turns a lot, but never still: the legs keep
    // stepping while the body turns.
    const float facing = std::max(SLOWEST, static_cast<float>(units::math::cos(error)));
    const compages::si::LinearVelocity speed = p_walker.speed * facing;
    // On an arc: the turn follows the steps. Near the point, a tighter turn,
    // or it would circle round a point inside its arc.
    compages::si::AngularVelocity rate =
        units::math::min(TURN_RATE, 1.0_rad * (speed / TURN_RADIUS));
    if (compages::si::Length(distance) < (2.0 * TURN_RADIUS))
    {
        rate = TURN_RATE;
    }
    const compages::si::Radians turn = rate * p_dt;
    p_walker.heading += units::math::min(units::math::max(error, -turn), turn);
    const float step = std::min(distance, compages::toMetres(speed * p_dt));
    const compages::Vector3f ahead(-static_cast<float>(units::math::sin(p_walker.heading)), 0.0f,
                                   -static_cast<float>(units::math::cos(p_walker.heading)));
    p_walker.body.position(here + (ahead * step));
    p_walker.body.rotation(p_walker.heading, compages::Vector3f(0.0f, 1.0f, 0.0f));
    // The feet go back on the floor as fast as the body goes forward.
    animator.speed = m_slide ? 1.0f : static_cast<float>(speed / p_walker.stride);
}

// ----------------------------------------------------------------------------
void Walkers::draw(compages::world::ViewFrame const& p_frame)
{
    for (Walker& walker : m_walkers)
    {
        step(walker, p_frame.elapsed);
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Walkers::controls()
{
    ImGui::SliderFloat("Fade (s)", &m_fade, 0.0f, 1.0f);
    ImGui::Checkbox("Clip at speed 1 (feet slide)", &m_slide);
    for (Walker const& walker : m_walkers)
    {
        ImGui::Text("%s: %s, %.1f m/s, clip x%.2f", walker.body.name().c_str(),
                    m_scene.playing(walker.body).c_str(), walker.speed.value(),
                    static_cast<double>(walker.body.get<compages::renderer::Animator>().speed));
    }
}

} // namespace examples
