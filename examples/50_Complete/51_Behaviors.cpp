// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "50_Complete/51_Behaviors.hpp"

#include "Common/Gui.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

//! \brief Turns its entity around its own vertical axis.
struct Behaviors::Spin : compages::world::Behavior
{
    explicit Spin(float p_speed) : speed(p_speed) {}

    void update(float p_dt) override
    {
        // Around its own vertical axis, at the speed it was given.
        transform().rotateY(speed * p_dt);
    }

    float speed;
};

//! \brief Floats up and down around where it was when it started.
struct Behaviors::Bob : compages::world::Behavior
{
    void start() override
    {
        rest = entity().position();
    }

    void update(float) override
    {
        // From the rest position recorded at start, so it does not drift up.
        const float height = 0.25f * std::sin(frame().total * 2.0f);
        entity().position(rest + Vector3f(0.0f, height, 0.0f));
    }

    Vector3f rest;
};

//! \brief Breathes, and grows while space or the button of the Try it panel
//! is held; shrinks back when it is released.
struct Behaviors::GrowOnSpace : compages::world::Behavior
{
    explicit GrowOnSpace(bool const& p_button) : button(p_button) {}

    void update(float p_dt) override
    {
        // Ease toward the target so the change is a growth, not a jump.
        const float target = (input().down(compages::world::Key::Space) || button) ? 1.6f : 1.0f;
        size += (target - size) * std::min(1.0f, 8.0f * p_dt);
        // A slow breath on top, so that it is alive even when nothing is held.
        const float breath = 1.0f + (0.06f * std::sin(frame().total * 3.0f));
        entity().scale(size * breath);
    }

    bool const& button;
    float size = 1.0f;
};

std::string Behaviors::description() const
{
    return "Three cubes, each driven by small behaviors attached to it: the "
           "blue one spins, the orange one floats and spins, the green one breathes "
           "and grows while SPACE is held (the mouse over the picture) or while "
           "the button of the Try it panel is. The camera control "
           "is a behavior too: right drag turns it.";
}

compages::gpu::Status Behaviors::setUp()
{
    // The camera control is a behavior too: Orbit, hung on the camera.
    m_scene.background(0.04f, 0.05f, 0.09f).ambient(0.12f, 0.13f, 0.16f);
    m_scene.camera().position(0.0f, 1.5f, 5.0f).add<compages::world::Orbit>();
    m_scene.sun();

    // One behavior, two, or one that reads the keyboard.
    m_scene.box("Spinner", compages::renderer::color(0.18f, 0.55f, 0.95f))
        .position(-1.6f, 0.0f, 0.0f)
        .add<Spin>(1.5f);
    m_scene.box("Floater", compages::renderer::color(0.95f, 0.65f, 0.15f))
        .add<Bob>()
        .add<Spin>(-0.5f);
    m_scene.box("Grower", compages::renderer::color(0.35f, 0.85f, 0.35f))
        .position(1.6f, 0.0f, 0.0f)
        .add<GrowOnSpace>(m_grow);

    return m_scene.prepare();
}

void Behaviors::draw(Frame const& p_frame)
{
    // update() of every behavior runs inside the draw.
    m_scene.draw(p_frame);
}

void Behaviors::controls()
{
    // Held rather than clicked: true for as long as the button is pressed.
    ImGui::Button("Hold to grow the green cube");
    m_grow = ImGui::IsItemActive();
}

} // namespace examples
