// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/World/EntityId.hpp"

#include <filesystem>
#include <string>

namespace compages::world
{
class World;
} // namespace compages::world

namespace compages::renderer
{

class AssetManager;

// ****************************************************************************
//! \brief What a shape looks like: a plain colour, a picture, its normals or
//! its depth.
//!
//! Made by the small functions below rather than filled by hand:
//! \code
//! m_scene.box("crate",
//! compages::renderer::texture("textures/wooden-crate.jpg"));
//! m_scene.sphere("ball", compages::renderer::color(1.0f, 0.3f, 0.2f));
//! \endcode
// ****************************************************************************
struct Look
{
    //! \brief Which of the built-in shaders draws it.
    ShaderFamily family = ShaderFamily::Lit;
    //! \brief The colour, or what the picture is multiplied by.
    compages::Vector3f color{ 0.8f, 0.8f, 0.8f };
    //! \brief The picture file, for a textured look. Loaded once whatever the
    //! number of shapes wearing it.
    std::filesystem::path texture;
    //! \brief What the depth look maps to black and to white.
    float depth_near = 1.0f;
    float depth_far = 100.0f;
    //! \brief One is solid.
    float opacity = 1.0f;
};

//! \brief A plain colour, lit by the lights of the scene.
[[nodiscard]] inline Look color(float p_red, float p_green, float p_blue)
{
    Look look;
    look.color = compages::Vector3f(p_red, p_green, p_blue);
    return look;
}

//! \brief A picture, lit by the lights of the scene.
[[nodiscard]] inline Look texture(std::filesystem::path p_path)
{
    Look look;
    look.family = ShaderFamily::PbrMinimal;
    look.color = compages::Vector3f(1.0f, 1.0f, 1.0f);
    look.texture = std::move(p_path);
    return look;
}

//! \brief The normals as colours: what shows that a mesh is built right.
[[nodiscard]] inline Look normals()
{
    Look look;
    look.family = ShaderFamily::Normals;
    return look;
}

//! \brief The distance to the camera, black at \c p_near, white at \c p_far.
[[nodiscard]] inline Look depth(float p_near = 1.0f, float p_far = 100.0f)
{
    Look look;
    look.family = ShaderFamily::Depth;
    look.depth_near = p_near;
    look.depth_far = p_far;
    return look;
}

// ****************************************************************************
//! \brief The built-in shapes.
// ****************************************************************************
enum class Shape
{
    Box,
    Sphere,
    Plane,
    Cylinder,
    Cone,
    Pyramid,
};

// ****************************************************************************
//! \brief Turns a Look or a Shape into assets, made once and found again.
//!
//! The built-in meshes, the built-in materials and the pictures are kept in
//! the AssetManager under a name, so that a second Scene sharing the assets
//! finds them instead of making them again. Used by Scene; useful alone to
//! dress entities without a Scene.
//!
//! \code
//! compages::renderer::Looks looks(assets);
//! entity.set(compages::renderer::MeshRenderer{
//!     looks.shapeMesh(compages::renderer::Shape::Box),
//!     looks.material({}, compages::renderer::color(1, 0, 0)) });
//! \endcode
//!
//! A failure is recorded as a frame error (compages::gpu::reportError) and
//! gives an empty id, so that building goes on.
// ****************************************************************************
class Looks
{
public:

    explicit Looks(AssetManager& p_assets) : m_assets(&p_assets) {}

    //! \brief The mesh of a built-in shape, made on first use.
    [[nodiscard]] MeshAssetId shapeMesh(Shape p_shape);

    //! \brief The material of a shader family, made on first use.
    [[nodiscard]] MaterialId familyMaterial(ShaderFamily p_family);

    //! \brief A picture file as a texture, loaded on first use.
    [[nodiscard]] TextureAssetId
    pictureTexture(std::filesystem::path const& p_path);

    //! \brief The material instance a Look describes, not yet stored.
    [[nodiscard]] MaterialInstance instanceOf(Look const& p_look);

    //! \brief A stored material instance. With a name, it can be found again
    //! and saved with the scene.
    [[nodiscard]] MaterialInstanceId material(std::string p_name,
                                              Look const& p_look);

    //! \brief Give a drawn entity a new material instance. The old one is
    //! removed when nothing else wears it and it has no name.
    void wear(compages::world::World& p_world,
              compages::world::EntityId p_entity,
              MaterialInstance const& p_instance);

private:

    AssetManager* m_assets;
};

} // namespace compages::renderer
