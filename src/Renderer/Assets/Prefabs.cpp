// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Assets/Prefabs.hpp"

namespace compages::renderer
{

namespace
{

PrefabMeshRenderer mesh(std::string p_material)
{
    PrefabMeshRenderer renderer;
    renderer.mesh = "box";
    renderer.material_instance = std::move(p_material);
    return renderer;
}

PrefabNode meshChild(std::string p_name,
                     std::string p_material,
                     compages::core::Vector3f const& p_position,
                     compages::core::Vector3f const& p_scale)
{
    PrefabNode node;
    node.name = std::move(p_name);
    node.transform.position = p_position;
    node.transform.scale = p_scale;
    node.mesh_renderer = mesh(std::move(p_material));
    return node;
}

} // namespace

Prefab makeRobotPrefab()
{
    const compages::core::Vector3f body{ 20.0f, 30.0f, 10.0f };
    const compages::core::Vector3f head{ 10.0f, 10.0f, 10.0f };
    const compages::core::Vector3f arm{ 6.0f, 24.0f, 6.0f };
    const compages::core::Vector3f leg{ 6.0f, 26.0f, 6.0f };
    const float body_half_y = body.y * 0.5f;
    const float arm_half_y = arm.y * 0.5f;
    const float leg_half_y = leg.y * 0.5f;

    Prefab prefab;
    prefab.name = "robot";
    prefab.root.name = "Robot";
    prefab.root.transform.position = compages::core::Vector3f(0.0f, 0.0f, 0.0f);

    PrefabNode body_joint;
    body_joint.name = "Body";
    body_joint.transform.position = compages::core::Vector3f(0.0f, body_half_y + leg.y, 0.0f);
    body_joint.children.emplace_back(
        meshChild("BodyMesh", "wood", compages::core::Vector3f(0.0f), body));

    PrefabNode head_joint;
    head_joint.name = "Head";
    head_joint.transform.position =
        compages::core::Vector3f(0.0f, body_half_y + (head.y * 0.5f), 0.0f);
    head_joint.children.emplace_back(
        meshChild("HeadMesh", "light", compages::core::Vector3f(0.0f), head));

    PrefabNode left_shoulder;
    left_shoulder.name = "LeftShoulder";
    left_shoulder.transform.position =
        compages::core::Vector3f(-((body.x * 0.5f) + (arm.x * 0.5f)), body_half_y, 0.0f);
    left_shoulder.children.emplace_back(meshChild(
        "LeftArmMesh", "dark", compages::core::Vector3f(0.0f, -arm_half_y, 0.0f), arm));

    PrefabNode right_shoulder;
    right_shoulder.name = "RightShoulder";
    right_shoulder.transform.position =
        compages::core::Vector3f(((body.x * 0.5f) + (arm.x * 0.5f)), body_half_y, 0.0f);
    right_shoulder.children.emplace_back(meshChild(
        "RightArmMesh", "dark", compages::core::Vector3f(0.0f, -arm_half_y, 0.0f), arm));

    PrefabNode left_leg;
    left_leg.name = "LeftLeg";
    left_leg.transform.position =
        compages::core::Vector3f(-(body.x * 0.25f), -(body_half_y + leg_half_y), 0.0f);
    left_leg.transform.scale = leg;
    left_leg.mesh_renderer = mesh("dark");

    PrefabNode right_leg;
    right_leg.name = "RightLeg";
    right_leg.transform.position =
        compages::core::Vector3f((body.x * 0.25f), -(body_half_y + leg_half_y), 0.0f);
    right_leg.transform.scale = leg;
    right_leg.mesh_renderer = mesh("dark");

    body_joint.children.emplace_back(std::move(head_joint));
    body_joint.children.emplace_back(std::move(left_shoulder));
    body_joint.children.emplace_back(std::move(right_shoulder));
    body_joint.children.emplace_back(std::move(left_leg));
    body_joint.children.emplace_back(std::move(right_leg));
    prefab.root.children.emplace_back(std::move(body_joint));
    return prefab;
}

} // namespace compages::renderer
