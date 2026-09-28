// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "30_WorldAndAssets/35_PrefabAndSave.hpp"

#include "Compages/Renderer/Assets/Prefabs.hpp"
#include "Compages/Renderer/Serialization/SceneSerializer.hpp"

#include <cmath>

namespace examples
{

constexpr char const* SAVED = "/tmp/compages_prefab_scene.json";

//! \brief Swings the arms and turns the head of a placed robot.
struct Wave: compages::world::Behavior
{
    explicit Wave(float p_phase) : phase(p_phase) {}

    void start() override
    {
        head = entity().lookup("Body/Head");
        left = entity().lookup("Body/LeftShoulder");
        right = entity().lookup("Body/RightShoulder");
    }

    void update(float) override
    {
        // Each placed robot keeps the phase it was given, so the three waves
        // do not line up.
        const float t = frame().total + phase;
        const float swing = 0.55f * std::sin(1.8f * t);
        left.rotation(swing, { 0.0f, 0.0f, 1.0f });
        right.rotation(-swing, { 0.0f, 0.0f, 1.0f });
        head.rotation(0.25f * std::sin(t), { 0.0f, 1.0f, 0.0f });
    }

    float phase;
    compages::world::Entity head;
    compages::world::Entity left;
    compages::world::Entity right;
};

std::string PrefabAndSave::description() const
{
    return "A robot prefab placed three times, each waving on its own. After "
           "a moment the World is saved to /tmp/compages_prefab_scene.json, "
           "with asset names rather than GPU ids.";
}

compages::gpu::Status PrefabAndSave::setUp()
{
    m_scene.background(0.05f, 0.07f, 0.12f).ambient(0.14f, 0.15f, 0.18f);
    m_scene.camera()
        .position(0.0f, 35.0f, 120.0f)
        .lookAt(0.0f, 25.0f, 0.0f)
        .add<compages::world::Orbit>(Vector3f(0.0f, 25.0f, 0.0f));
    m_scene.sun();

    // What the prefab names, registered under those names.
    m_scene.shapeMesh(compages::renderer::Shape::Box);
    m_scene.material("wood", compages::renderer::color(0.62f, 0.42f, 0.24f));
    m_scene.material("dark", compages::renderer::color(0.35f, 0.24f, 0.14f));
    m_scene.material("light", compages::renderer::color(0.92f, 0.90f, 0.82f));
    compages::renderer::PrefabId robot;
    COMPAGES_TRY_ASSIGN(
        robot, m_scene.assets().addPrefab("robot", compages::renderer::makeRobotPrefab()));

    // Three instances of the one prefab, each with its own Wave.
    for (int i = 0; i < 3; ++i)
    {
        compages::world::Entity placed;
        COMPAGES_TRY_ASSIGN(placed, m_scene.instantiate(robot));
        placed.position(float(i - 1) * 40.0f, 0.0f, 0.0f)
            .add<Wave>(float(i) * 1.2f);
    }
    return m_scene.prepare();
}

void PrefabAndSave::draw(Frame const& p_frame)
{
    m_scene.draw(p_frame);
    // Once, after the first frames, so the file holds a scene that has run.
    if (!m_saved && (p_frame.total > 0.1f))
    {
        compages::gpu::check(compages::renderer::saveScene(m_scene, SAVED));
        m_saved = true;
    }
}

} // namespace examples
