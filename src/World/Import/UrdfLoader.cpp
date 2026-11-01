// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Import/UrdfLoader.hpp"

#include <pugixml.hpp>

#include <limits>
#include <numbers>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace compages::world
{

namespace
{

std::vector<float> numbers(char const* p_text)
{
    std::vector<float> values;
    std::istringstream in(p_text);
    float value = 0.0f;
    while (in >> value)
    {
        values.push_back(value);
    }
    return values;
}

Result<compages::core::Vector3f> triple(pugi::xml_node p_node,
                                        char const* p_attribute,
                                        compages::core::Vector3f p_default)
{
    pugi::xml_attribute attribute = p_node.attribute(p_attribute);
    if (!attribute)
    {
        return p_default;
    }
    const std::vector<float> values = numbers(attribute.value());
    if (values.size() != 3u)
    {
        return failure(std::string("<") + p_node.name() + " " + p_attribute +
                       "> needs three numbers, not '" + attribute.value() +
                       "'");
    }
    return compages::core::Vector3f(values[0], values[1], values[2]);
}

//! \brief URDF roll, pitch, yaw are turns around the fixed X, Y then Z axes:
//! R = Rz(yaw) * Ry(pitch) * Rx(roll).
compages::core::Quatf fromRpy(compages::core::Vector3f const& p_rpy)
{
    using units::angle::radian_t;
    const compages::core::Quatf roll = compages::core::Quatf::fromAngleAxis(
        radian_t(p_rpy.x), compages::core::Vector3f(1.0f, 0.0f, 0.0f));
    const compages::core::Quatf pitch = compages::core::Quatf::fromAngleAxis(
        radian_t(p_rpy.y), compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Quatf yaw = compages::core::Quatf::fromAngleAxis(
        radian_t(p_rpy.z), compages::core::Vector3f(0.0f, 0.0f, 1.0f));
    compages::core::Quatf rotation = yaw * pitch * roll;
    rotation.normalize();
    return rotation;
}

Result<LocalTransform> parseOrigin(pugi::xml_node p_parent)
{
    LocalTransform origin;
    pugi::xml_node node = p_parent.child("origin");
    if (!node)
    {
        return origin;
    }
    COMPAGES_TRY_ASSIGN(
        origin.position,
        triple(node, "xyz", compages::core::Vector3f(0.0f, 0.0f, 0.0f)));
    compages::core::Vector3f rpy;
    COMPAGES_TRY_ASSIGN(
        rpy, triple(node, "rpy", compages::core::Vector3f(0.0f, 0.0f, 0.0f)));
    origin.rotation = fromRpy(rpy);
    return origin;
}

std::optional<compages::core::Vector4f> parseColor(pugi::xml_node p_material)
{
    pugi::xml_node color = p_material.child("color");
    if (!color)
    {
        return std::nullopt;
    }
    const std::vector<float> values = numbers(color.attribute("rgba").value());
    if (values.size() != 4u)
    {
        return std::nullopt;
    }
    return compages::core::Vector4f(values[0], values[1], values[2], values[3]);
}

//! \brief Materials are defined once, at the top of the file or inside the
//! first visual using them, then referred to by name.
std::unordered_map<std::string, compages::core::Vector4f>
namedColors(pugi::xml_node p_robot)
{
    std::unordered_map<std::string, compages::core::Vector4f> colors;
    for (pugi::xpath_node found : p_robot.select_nodes("//material[@name]"))
    {
        pugi::xml_node material = found.node();
        if (std::optional<compages::core::Vector4f> rgba = parseColor(material))
        {
            colors.try_emplace(material.attribute("name").value(), *rgba);
        }
    }
    return colors;
}

Result<UrdfVisual> parseVisual(
    pugi::xml_node p_visual,
    std::unordered_map<std::string, compages::core::Vector4f> const& p_colors)
{
    UrdfVisual visual;
    COMPAGES_TRY_ASSIGN(visual.origin, parseOrigin(p_visual));

    pugi::xml_node geometry = p_visual.child("geometry");
    if (pugi::xml_node mesh = geometry.child("mesh"))
    {
        visual.kind = UrdfGeometry::Mesh;
        visual.filename = mesh.attribute("filename").value();
        COMPAGES_TRY_ASSIGN(
            visual.origin.scale,
            triple(mesh, "scale", compages::core::Vector3f(1.0f, 1.0f, 1.0f)));
    }
    else if (pugi::xml_node box = geometry.child("box"))
    {
        visual.kind = UrdfGeometry::Box;
        COMPAGES_TRY_ASSIGN(
            visual.size,
            triple(box, "size", compages::core::Vector3f(1.0f, 1.0f, 1.0f)));
    }
    else if (pugi::xml_node cylinder = geometry.child("cylinder"))
    {
        visual.kind = UrdfGeometry::Cylinder;
        visual.radius = cylinder.attribute("radius").as_float(0.5f);
        visual.length = cylinder.attribute("length").as_float(1.0f);
    }
    else if (pugi::xml_node sphere = geometry.child("sphere"))
    {
        visual.kind = UrdfGeometry::Sphere;
        visual.radius = sphere.attribute("radius").as_float(0.5f);
    }
    else
    {
        return failure("a <visual> without a mesh, box, cylinder "
                       "or sphere geometry");
    }

    if (pugi::xml_node material = p_visual.child("material"))
    {
        visual.rgba = parseColor(material);
        if (!visual.rgba)
        {
            auto named = p_colors.find(material.attribute("name").value());
            if (named != p_colors.end())
            {
                visual.rgba = named->second;
            }
        }
    }
    return visual;
}

Result<UrdfJointKind> parseUrdfJointKind(std::string_view p_type)
{
    if (p_type == "fixed")
    {
        return UrdfJointKind::Fixed;
    }
    if (p_type == "revolute")
    {
        return UrdfJointKind::Revolute;
    }
    if (p_type == "continuous")
    {
        return UrdfJointKind::Continuous;
    }
    if (p_type == "prismatic")
    {
        return UrdfJointKind::Prismatic;
    }
    return failure("unsupported URDF joint type '" + std::string(p_type) +
                   "': only fixed, revolute, continuous and "
                   "prismatic are");
}

Result<UrdfJoint> parseJoint(pugi::xml_node p_joint)
{
    UrdfJoint joint;
    joint.name = p_joint.attribute("name").value();
    COMPAGES_TRY_ASSIGN(joint.kind,
                        parseUrdfJointKind(p_joint.attribute("type").value()));
    joint.parent = p_joint.child("parent").attribute("link").value();
    joint.child = p_joint.child("child").attribute("link").value();
    if (joint.parent.empty() || joint.child.empty())
    {
        return failure("joint '" + joint.name +
                       "' lacks its parent or child link");
    }
    COMPAGES_TRY_ASSIGN(joint.origin, parseOrigin(p_joint));
    COMPAGES_TRY_ASSIGN(joint.axis,
                        triple(p_joint.child("axis"),
                               "xyz",
                               compages::core::Vector3f(1.0f, 0.0f, 0.0f)));
    if (compages::core::vector::squaredNorm(joint.axis) <= 0.0f)
    {
        return failure("joint '" + joint.name + "' has a null axis");
    }
    if (pugi::xml_node limit = p_joint.child("limit"))
    {
        if (pugi::xml_attribute lower = limit.attribute("lower"))
        {
            joint.lower = lower.as_double();
        }
        if (pugi::xml_attribute upper = limit.attribute("upper"))
        {
            joint.upper = upper.as_double();
        }
        joint.velocity = limit.attribute("velocity").as_double(0.0);
    }
    return joint;
}

// ****************************************************************************
//! \brief Turns a parsed robot into entities.
// ****************************************************************************
class Builder
{
public:

    Builder(World& p_world, UrdfLinkHook const& p_on_link)
        : m_world(p_world), m_on_link(p_on_link)
    {
    }

    Result<Entity> build(UrdfRobot const& p_robot,
                                          EntityId p_parent)
    {
        for (UrdfLink const& link : p_robot.links)
        {
            m_links.try_emplace(link.name, &link);
        }
        std::unordered_map<std::string, bool> is_child;
        for (UrdfJoint const& joint : p_robot.joints)
        {
            if ((m_links.count(joint.parent) == 0u) ||
                (m_links.count(joint.child) == 0u))
            {
                return failure("joint '" + joint.name +
                               "' refers to an unknown link");
            }
            if (is_child[joint.child])
            {
                return failure("link '" + joint.child +
                               "' is the child of two joints");
            }
            is_child[joint.child] = true;
            m_children[joint.parent].push_back(&joint);
        }

        UrdfLink const* root_link = nullptr;
        for (UrdfLink const& link : p_robot.links)
        {
            if (!is_child[link.name])
            {
                if (root_link != nullptr)
                {
                    return failure("the URDF has several root links: '" +
                                   root_link->name + "' and '" + link.name +
                                   "'");
                }
                root_link = &link;
            }
        }
        if (root_link == nullptr)
        {
            return failure("the URDF has no root link");
        }

        Entity robot = m_world.entity(p_robot.name);
        if (m_world.alive(p_parent))
        {
            robot.parent(Entity(m_world, p_parent));
        }
        robot.rotation(-0.5f * std::numbers::pi_v<float>,
                       compages::core::Vector3f(1.0f, 0.0f, 0.0f));
        Status built = addLink(*root_link, robot.child(root_link->name));
        if (!built)
        {
            robot.destroy();
            return failure(built.error());
        }
        return robot;
    }

private:

    Status addLink(UrdfLink const& p_link, Entity p_entity)
    {
        if (m_on_link)
        {
            COMPAGES_TRY(m_on_link(p_link, p_entity));
        }
        auto children = m_children.find(p_link.name);
        if (children == m_children.end())
        {
            return success();
        }
        for (UrdfJoint const* joint : children->second)
        {
            Entity child = p_entity.child(joint->child);
            child.position(joint->origin.position)
                .rotation(joint->origin.rotation);
            addJoint(*joint, child);
            COMPAGES_TRY(addLink(*m_links.at(joint->child), child));
        }
        return success();
    }

    static void addJoint(UrdfJoint const& p_joint,
                         Entity p_entity)
    {
        const double infinity = std::numeric_limits<double>::infinity();
        switch (p_joint.kind)
        {
            case UrdfJointKind::Fixed:
                return;
            case UrdfJointKind::Continuous:
                p_entity.revolute(p_joint.axis);
                break;
            case UrdfJointKind::Revolute:
                p_entity.revolute(
                    p_joint.axis,
                    units::angle::radian_t(p_joint.lower.value_or(-infinity)),
                    units::angle::radian_t(p_joint.upper.value_or(infinity)));
                break;
            case UrdfJointKind::Prismatic:
                p_entity.prismatic(
                    p_joint.axis,
                    units::length::meter_t(p_joint.lower.value_or(-infinity)),
                    units::length::meter_t(p_joint.upper.value_or(infinity)));
                break;
        }
        if (p_joint.velocity > 0.0)
        {
            if (auto* revolute =
                    p_entity.find<RevoluteJoint>())
            {
                revolute->state.velocity.min =
                    units::angular_velocity::radians_per_second_t(
                        -p_joint.velocity);
                revolute->state.velocity.max =
                    units::angular_velocity::radians_per_second_t(
                        p_joint.velocity);
            }
            else if (auto* prismatic =
                         p_entity.find<PrismaticJoint>())
            {
                prismatic->state.velocity.min =
                    units::velocity::meters_per_second_t(-p_joint.velocity);
                prismatic->state.velocity.max =
                    units::velocity::meters_per_second_t(p_joint.velocity);
            }
        }
    }

    World& m_world;
    UrdfLinkHook const& m_on_link;
    std::unordered_map<std::string, UrdfLink const*> m_links;
    std::unordered_map<std::string, std::vector<UrdfJoint const*>> m_children;
};

} // namespace

Result<UrdfRobot> parseUrdf(std::string const& p_path)
{
    pugi::xml_document document;
    const pugi::xml_parse_result parsed = document.load_file(p_path.c_str());
    if (!parsed)
    {
        return failure("cannot read the URDF file '" + p_path +
                       "': " + parsed.description());
    }
    pugi::xml_node root = document.child("robot");
    if (!root)
    {
        return failure("'" + p_path + "' has no <robot> element");
    }

    UrdfRobot robot;
    robot.name = root.attribute("name").value();
    const auto colors = namedColors(root);
    for (pugi::xml_node node : root.children("link"))
    {
        UrdfLink link;
        link.name = node.attribute("name").value();
        for (pugi::xml_node visual_node : node.children("visual"))
        {
            auto visual = parseVisual(visual_node, colors);
            if (!visual)
            {
                return failure("link '" + link.name + "': " + visual.error());
            }
            link.visuals.push_back(visual.take());
        }
        robot.links.push_back(std::move(link));
    }
    for (pugi::xml_node node : root.children("joint"))
    {
        auto joint = parseJoint(node);
        if (!joint)
        {
            return failure(joint.error());
        }
        robot.joints.push_back(joint.take());
    }
    return robot;
}

Result<Entity> buildUrdf(World& p_world,
                         UrdfRobot const& p_robot,
                         EntityId p_parent,
                         UrdfLinkHook const& p_on_link)
{
    Builder builder(p_world, p_on_link);
    return builder.build(p_robot, p_parent);
}

Result<Entity> loadUrdf(World& p_world,
                        std::string const& p_path,
                        EntityId p_parent)
{
    auto robot = parseUrdf(p_path);
    if (!robot)
    {
        return failure(robot.error());
    }
    auto root = buildUrdf(p_world, robot.value(), p_parent);
    if (!root)
    {
        return failure("'" + p_path + "': " + root.error());
    }
    return root;
}

} // namespace compages::world
