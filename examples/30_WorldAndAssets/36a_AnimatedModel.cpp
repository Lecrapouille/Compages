//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "30_WorldAndAssets/36a_AnimatedModel.hpp"
#include "Compages/World/Entity.hpp"

#include <cmath>

namespace examples
{

const compages::renderer::Look LIMB =
    compages::renderer::color(0.28f, 0.30f, 0.36f);
const compages::renderer::Look SKIN =
    compages::renderer::color(0.92f, 0.74f, 0.58f);

//------------------------------------------------------------------------------
//! \brief Walks its entity around a circle, swinging legs and arms in
//! opposite phase.
struct AnimatedModel::Walk: compages::world::Behavior
{
    Walk(float p_radius, float p_phase) : radius(p_radius), phase(p_phase) {}

    void start() override
    {
        head = entity().lookup("Torso/Head");
        left_hip = entity().lookup("Torso/LeftHip");
        right_hip = entity().lookup("Torso/RightHip");
        left_shoulder = entity().lookup("Torso/LeftShoulder");
        right_shoulder = entity().lookup("Torso/RightShoulder");
    }

    void update(float) override
    {
        // Around the circle, facing the way it walks. Arms and legs swing in
        // opposite phase, which is what reads as a step.
        const float time = frame().total + phase;
        const float step = std::sin(2.4f * time);
        const float path = 0.55f * time;
        entity()
            .position(radius * std::sin(path),
                      0.04f * std::abs(step),
                      radius * std::cos(path))
            .rotation(path + 1.5707963f, { 0.0f, 1.0f, 0.0f });
        left_hip.rotation(0.70f * step, { 1.0f, 0.0f, 0.0f });
        right_hip.rotation(-0.70f * step, { 1.0f, 0.0f, 0.0f });
        left_shoulder.rotation(-0.55f * step, { 1.0f, 0.0f, 0.0f });
        right_shoulder.rotation(0.55f * step, { 1.0f, 0.0f, 0.0f });
        head.rotation(0.15f * std::sin(1.2f * time), { 0.0f, 1.0f, 0.0f });
    }

    float radius;
    float phase;
    compages::world::Entity head;
    compages::world::Entity left_hip;
    compages::world::Entity right_hip;
    compages::world::Entity left_shoulder;
    compages::world::Entity right_shoulder;
};

//------------------------------------------------------------------------------
std::string AnimatedModel::description() const
{
    return "Two walkers going round: hips and shoulders are joints, the limbs "
           "hang under them, and one Walk behavior per walker swings them. The "
           "camera turns slowly by itself.";
}

//------------------------------------------------------------------------------
compages::world::Entity
AnimatedModel::makeWalker(char const* p_name,
                          compages::renderer::Look const& p_shirt)
{
    // Joints first, meshes hung under them. Turning a joint turns the limb.
    compages::world::Entity walker = m_world.entity(p_name);
    compages::world::Entity torso =
        walker.child("Torso").position(0.0f, 0.5f, 0.0f);
    m_scene.box("TorsoMesh", p_shirt)
        .parent(torso)
        .position(0.0f, 0.23f, 0.0f)
        .scale(0.34f, 0.46f, 0.20f);
    compages::world::Entity head =
        torso.child("Head").position(0.0f, 0.62f, 0.0f);
    m_scene.sphere("HeadMesh", SKIN).parent(head).scale(0.28f);

    // A limb hangs half its length under its joint, so that it swings from
    // the joint and not from its middle.
    for (float side : { -1.0f, 1.0f })
    {
        compages::world::Entity shoulder =
            torso.child(side < 0.0f ? "LeftShoulder" : "RightShoulder")
                .position(0.24f * side, 0.42f, 0.0f);
        m_scene.cylinder("Arm", LIMB)
            .parent(shoulder)
            .position(0.0f, -0.2f, 0.0f)
            .scale(0.1f, 0.4f, 0.1f);

        compages::world::Entity hip =
            torso.child(side < 0.0f ? "LeftHip" : "RightHip")
                .position(0.1f * side, 0.0f, 0.0f);
        m_scene.cylinder("Leg", LIMB)
            .parent(hip)
            .position(0.0f, -0.25f, 0.0f)
            .scale(0.13f, 0.5f, 0.13f);
        m_scene.box("Foot", LIMB)
            .parent(hip)
            .position(0.0f, -0.5f, 0.08f)
            .scale(0.12f, 0.06f, 0.20f);
    }
    return walker;
}

//------------------------------------------------------------------------------
compages::Status AnimatedModel::setUp()
{
    // The camera turns by itself, around the walkers' height.
    m_scene.background(0.08f, 0.10f, 0.13f).ambient(0.20f, 0.20f, 0.22f);
    m_scene.camera()
        .position(0.0f, 3.6f, 8.5f)
        .add<compages::world::Orbit>(
            compages::core::Vector3f(0.0f, 0.6f, 0.0f));
    m_scene.activeCamera().get<compages::world::Orbit>().spin = 0.22f;
    m_scene.sun();

    m_scene.plane("Ground", compages::renderer::color(0.40f, 0.44f, 0.40f))
        .rotation(-1.5707963f, { 1.0f, 0.0f, 0.0f })
        .scale(14.0f);
    // Two radii, out of step. The box, sphere and cylinder meshes are shared.
    makeWalker("WalkerA", compages::renderer::color(0.22f, 0.48f, 0.78f))
        .add<Walk>(3.1f, 0.0f);
    makeWalker("WalkerB", compages::renderer::color(0.82f, 0.32f, 0.24f))
        .add<Walk>(2.2f, 1.7f);
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void AnimatedModel::draw(compages::world::ViewFrame const& p_frame)
{
    // The Walk behaviors run inside the draw.
    m_scene.draw(p_frame);
}

} // namespace examples
