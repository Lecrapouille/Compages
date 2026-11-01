// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Import/UrdfLoader.hpp"
#include "Compages/Renderer/Assets/StlLoader.hpp"
#include "Compages/Renderer/Scene.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <numbers>
#include <unordered_map>

namespace compages::renderer
{

namespace
{

namespace fs = std::filesystem;

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
//! \brief Adds the visuals of each link, as the World builds the chain.
// ****************************************************************************
class Visuals
{
public:

    Visuals(Scene& p_scene, fs::path p_folder)
        : m_scene(p_scene), m_folder(std::move(p_folder))
    {
    }

    Status addLink(compages::world::UrdfLink const& p_link,
                   compages::world::Entity p_entity)
    {
        for (compages::world::UrdfVisual const& visual : p_link.visuals)
        {
            COMPAGES_TRY(addVisual(visual, p_entity));
        }
        return success();
    }

private:

    Status addVisual(compages::world::UrdfVisual const& p_visual, compages::world::Entity p_link)
    {
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
            case compages::world::UrdfGeometry::Mesh:
            {
                COMPAGES_TRY_ASSIGN(visual,
                                    meshVisual(p_visual.filename, look));
                break;
            }
            case compages::world::UrdfGeometry::Box:
                visual = m_scene.box("visual", look);
                scale = p_visual.size;
                break;
            case compages::world::UrdfGeometry::Sphere:
                visual = m_scene.sphere("visual", look);
                scale = compages::core::Vector3f(2.0f * p_visual.radius,
                                                 2.0f * p_visual.radius,
                                                 2.0f * p_visual.radius);
                break;
            case compages::world::UrdfGeometry::Cylinder:
                // The shapes of the Scene stand along Y, a URDF cylinder
                // along Z.
                visual = m_scene.cylinder("visual", look);
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
                m_scene.copy(drawn->second, "visual");
            m_scene.look(copy, p_look);
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
            visual = m_scene.mesh(mesh.take(), "visual", p_look);
        }
        else if ((extension == ".glb") || (extension == ".gltf"))
        {
            visual = m_scene.world().entity("visual");
            COMPAGES_TRY(m_scene.load(path, visual));
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

    Scene& m_scene;
    fs::path m_folder;
    std::unordered_map<std::string, compages::world::EntityId> m_meshes;
};

} // namespace

Result<compages::world::Entity> loadUrdf(Scene& p_scene,
                                         std::string const& p_path,
                                         compages::world::EntityId p_parent)
{
    auto robot = compages::world::parseUrdf(p_path);
    if (!robot)
    {
        return failure(robot.error());
    }
    Visuals visuals(p_scene, fs::path(p_path).parent_path());
    auto root = compages::world::buildUrdf(
        p_scene.world(), robot.value(), p_parent,
        [&visuals](compages::world::UrdfLink const& p_link,
                   compages::world::Entity p_entity)
        { return visuals.addLink(p_link, p_entity); });
    if (!root)
    {
        return failure("'" + p_path + "': " + root.error());
    }
    return root;
}

} // namespace compages::renderer
