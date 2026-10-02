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

#include "Compages/Renderer/Assets/UrdfLoader.hpp"
#include "Compages/Renderer/Assets/StlLoader.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Entity.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <limits>
#include <numbers>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace compages::renderer
{

namespace
{

namespace fs = std::filesystem;

enum class GeometryKind
{
    Mesh,
    Box,
    Cylinder,
    Sphere,
};

struct UrdfVisual
{
    compages::world::LocalTransform origin{};
    GeometryKind kind = GeometryKind::Box;
    std::string filename;
    compages::core::Vector3f size{ 1.0f, 1.0f, 1.0f };
    float radius = 0.5f;
    float length = 1.0f;
    std::optional<compages::core::Vector4f> rgba;
};

struct UrdfLink
{
    std::string name;
    std::vector<UrdfVisual> visuals;
};

enum class JointKind
{
    Fixed,
    Revolute,
    Continuous,
    Prismatic,
};

struct UrdfJoint
{
    std::string name;
    JointKind kind = JointKind::Fixed;
    std::string parent;
    std::string child;
    compages::world::LocalTransform origin{};
    compages::core::Vector3f axis{ 1.0f, 0.0f, 0.0f };
    std::optional<double> lower;
    std::optional<double> upper;
    double velocity = 0.0;
};

struct UrdfRobot
{
    std::string name;
    std::vector<UrdfLink> links;
    std::vector<UrdfJoint> joints;
};

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
Result<compages::world::LocalTransform> parseOrigin(pugi::xml_node p_parent)
{
    compages::world::LocalTransform origin;
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
Result<UrdfVisual> parseVisual(
    pugi::xml_node p_visual,
    std::unordered_map<std::string, compages::core::Vector4f> const& p_colors)
{
    UrdfVisual visual;
    COMPAGES_TRY_ASSIGN(visual.origin, parseOrigin(p_visual));

    pugi::xml_node geometry = p_visual.child("geometry");
    if (pugi::xml_node mesh = geometry.child("mesh"))
    {
        visual.kind = GeometryKind::Mesh;
        visual.filename = mesh.attribute("filename").value();
        COMPAGES_TRY_ASSIGN(
            visual.origin.scale,
            triple(mesh, "scale", compages::core::Vector3f(1.0f, 1.0f, 1.0f)));
    }
    else if (pugi::xml_node box = geometry.child("box"))
    {
        visual.kind = GeometryKind::Box;
        COMPAGES_TRY_ASSIGN(
            visual.size,
            triple(box, "size", compages::core::Vector3f(1.0f, 1.0f, 1.0f)));
    }
    else if (pugi::xml_node cylinder = geometry.child("cylinder"))
    {
        visual.kind = GeometryKind::Cylinder;
        visual.radius = cylinder.attribute("radius").as_float(0.5f);
        visual.length = cylinder.attribute("length").as_float(1.0f);
    }
    else if (pugi::xml_node sphere = geometry.child("sphere"))
    {
        visual.kind = GeometryKind::Sphere;
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

//------------------------------------------------------------------------------
Result<JointKind> parseJointKind(std::string_view p_type)
{
    if (p_type == "fixed")
    {
        return JointKind::Fixed;
    }
    if (p_type == "revolute")
    {
        return JointKind::Revolute;
    }
    if (p_type == "continuous")
    {
        return JointKind::Continuous;
    }
    if (p_type == "prismatic")
    {
        return JointKind::Prismatic;
    }
    return failure("unsupported URDF joint type '" + std::string(p_type) +
                   "': only fixed, revolute, continuous and "
                   "prismatic are");
}

//------------------------------------------------------------------------------
Result<UrdfJoint> parseJoint(pugi::xml_node p_joint)
{
    UrdfJoint joint;
    joint.name = p_joint.attribute("name").value();
    COMPAGES_TRY_ASSIGN(joint.kind,
                        parseJointKind(p_joint.attribute("type").value()));
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
//! \brief Where a mesh of the URDF is on disk: relative to the URDF file,
//! with a "package://name/" or "file://" prefix dropped.
std::string resolveMesh(fs::path const& p_folder, std::string p_filename)
{
    constexpr std::string_view PACKAGE = "package://";
    constexpr std::string_view FILE = "file://";
    std::vector<fs::path> candidates;
    if (p_filename.starts_with(FILE))
    {
        p_filename.erase(0u, FILE.size());
    }
    if (p_filename.starts_with(PACKAGE))
    {
        p_filename.erase(0u, PACKAGE.size());
        const fs::path inside(p_filename);
        candidates.push_back(p_folder / inside);
        candidates.push_back(p_folder.parent_path() / inside);
        fs::path without_package;
        for (auto part = std::next(inside.begin()); part != inside.end();
             ++part)
        {
            without_package /= *part;
        }
        candidates.push_back(p_folder / without_package);
    }
    else
    {
        const fs::path file(p_filename);
        candidates.push_back(file.is_absolute() ? file : (p_folder / file));
    }
    for (fs::path const& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return candidate.string();
        }
    }
    return candidates.front().string();
}

//------------------------------------------------------------------------------
std::string lowerExtension(std::string const& p_path)
{
    std::string extension = fs::path(p_path).extension().string();
    std::transform(extension.begin(),
                   extension.end(),
                   extension.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return extension;
}

// ****************************************************************************
//! \brief Turns a parsed robot into entities.
// ****************************************************************************
class Builder
{
public:

    Builder(compages::world::World& p_world, Scene* p_scene, fs::path p_folder)
        : m_world(p_world), m_scene(p_scene), m_folder(std::move(p_folder))
    {
    }

    Result<compages::world::Entity> build(UrdfRobot const& p_robot,
                                          compages::world::EntityId p_parent)
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

        compages::world::Entity robot = m_world.entity(p_robot.name);
        if (m_world.alive(p_parent))
        {
            robot.parent(compages::world::Entity(m_world, p_parent));
        }
        robot.rotation(Radians(-0.5f * std::numbers::pi_v<float>),
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

    Status addLink(UrdfLink const& p_link, compages::world::Entity p_entity)
    {
        for (UrdfVisual const& visual : p_link.visuals)
        {
            COMPAGES_TRY(addVisual(visual, p_entity));
        }
        auto children = m_children.find(p_link.name);
        if (children == m_children.end())
        {
            return success();
        }
        for (UrdfJoint const* joint : children->second)
        {
            compages::world::Entity child = p_entity.child(joint->child);
            child.position(joint->origin.position)
                .rotation(joint->origin.rotation);
            addJoint(*joint, child);
            COMPAGES_TRY(addLink(*m_links.at(joint->child), child));
        }
        return success();
    }

    static void addJoint(UrdfJoint const& p_joint,
                         compages::world::Entity p_entity)
    {
        const double infinity = std::numeric_limits<double>::infinity();
        switch (p_joint.kind)
        {
            case JointKind::Fixed:
                return;
            case JointKind::Continuous:
                p_entity.revolute(p_joint.axis);
                break;
            case JointKind::Revolute:
                p_entity.revolute(
                    p_joint.axis,
                    units::angle::radian_t(p_joint.lower.value_or(-infinity)),
                    units::angle::radian_t(p_joint.upper.value_or(infinity)));
                break;
            case JointKind::Prismatic:
                p_entity.prismatic(
                    p_joint.axis,
                    units::length::meter_t(p_joint.lower.value_or(-infinity)),
                    units::length::meter_t(p_joint.upper.value_or(infinity)));
                break;
        }
        if (p_joint.velocity > 0.0)
        {
            if (auto* revolute =
                    p_entity.find<compages::world::RevoluteJoint>())
            {
                revolute->state.velocity.min =
                    units::angular_velocity::radians_per_second_t(
                        -p_joint.velocity);
                revolute->state.velocity.max =
                    units::angular_velocity::radians_per_second_t(
                        p_joint.velocity);
            }
            else if (auto* prismatic =
                         p_entity.find<compages::world::PrismaticJoint>())
            {
                prismatic->state.velocity.min =
                    units::velocity::meters_per_second_t(-p_joint.velocity);
                prismatic->state.velocity.max =
                    units::velocity::meters_per_second_t(p_joint.velocity);
            }
        }
    }

    Status addVisual(UrdfVisual const& p_visual, compages::world::Entity p_link)
    {
        if (m_scene == nullptr)
        {
            return success();
        }
        Look look;
        if (p_visual.rgba)
        {
            look = color(p_visual.rgba->x, p_visual.rgba->y, p_visual.rgba->z);
            look.opacity = p_visual.rgba->w;
        }

        compages::world::Entity visual;
        compages::core::Vector3f scale = p_visual.origin.scale;
        compages::core::Quatf rotation = p_visual.origin.rotation;
        switch (p_visual.kind)
        {
            case GeometryKind::Mesh:
            {
                COMPAGES_TRY_ASSIGN(visual,
                                    meshVisual(p_visual.filename, look));
                break;
            }
            case GeometryKind::Box:
                visual = m_scene->box("visual", look);
                scale = p_visual.size;
                break;
            case GeometryKind::Sphere:
                visual = m_scene->sphere("visual", look);
                scale = compages::core::Vector3f(2.0f * p_visual.radius,
                                                 2.0f * p_visual.radius,
                                                 2.0f * p_visual.radius);
                break;
            case GeometryKind::Cylinder:
                // The shapes of the Scene stand along Y, a URDF cylinder
                // along Z.
                visual = m_scene->cylinder("visual", look);
                scale = compages::core::Vector3f(2.0f * p_visual.radius,
                                                 p_visual.length,
                                                 2.0f * p_visual.radius);
                rotation = rotation *
                           compages::core::Quatf::fromAngleAxis(
                               units::angle::radian_t(0.5 * std::numbers::pi),
                               compages::core::Vector3f(1.0f, 0.0f, 0.0f));
                break;
        }
        visual.parent(p_link)
            .position(p_visual.origin.position)
            .rotation(rotation)
            .scale(scale);
        return success();
    }

    Result<compages::world::Entity> meshVisual(std::string const& p_filename,
                                               Look const& p_look)
    {
        const std::string path = resolveMesh(m_folder, p_filename);
        auto drawn = m_meshes.find(path);
        if (drawn != m_meshes.end())
        {
            compages::world::Entity copy =
                m_scene->copy(drawn->second, "visual");
            m_scene->look(copy, p_look);
            return copy;
        }

        const std::string extension = lowerExtension(path);
        compages::world::Entity visual;
        if (extension == ".stl")
        {
            auto mesh = loadStl(path);
            if (!mesh)
            {
                return failure(mesh.error());
            }
            visual = m_scene->mesh(mesh.take(), "visual", p_look);
        }
        else if ((extension == ".glb") || (extension == ".gltf"))
        {
            visual = m_world.entity("visual");
            COMPAGES_TRY(m_scene->load(path, visual));
            return visual;
        }
        else
        {
            return failure("mesh '" + p_filename +
                           "': only STL and glTF meshes are read");
        }
        m_meshes.try_emplace(path, visual.id());
        return visual;
    }

    compages::world::World& m_world;
    Scene* m_scene;
    fs::path m_folder;
    std::unordered_map<std::string, UrdfLink const*> m_links;
    std::unordered_map<std::string, std::vector<UrdfJoint const*>> m_children;
    std::unordered_map<std::string, compages::world::EntityId> m_meshes;
};

//------------------------------------------------------------------------------
Result<compages::world::Entity> load(compages::world::World& p_world,
                                     Scene* p_scene,
                                     std::string const& p_path,
                                     compages::world::EntityId p_parent)
{
    auto robot = parseUrdf(p_path);
    if (!robot)
    {
        return failure(robot.error());
    }
    Builder builder(p_world, p_scene, fs::path(p_path).parent_path());
    auto root = builder.build(robot.value(), p_parent);
    if (!root)
    {
        return failure("'" + p_path + "': " + root.error());
    }
    return root;
}

} // namespace

//------------------------------------------------------------------------------
Result<compages::world::Entity> loadUrdf(compages::world::World& p_world,
                                         std::string const& p_path,
                                         compages::world::EntityId p_parent)
{
    return load(p_world, nullptr, p_path, p_parent);
}

//------------------------------------------------------------------------------
Result<compages::world::Entity> loadUrdf(Scene& p_scene,
                                         std::string const& p_path,
                                         compages::world::EntityId p_parent)
{
    return load(p_scene.world(), &p_scene, p_path, p_parent);
}

} // namespace compages::renderer
