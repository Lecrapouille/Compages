// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Renderer/Scene/Environment.hpp"
#include "Compages/Renderer/Render/Raycast.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"
#include "Compages/Renderer/Render/DebugDraw.hpp"
#include "Compages/Renderer/Scene/RenderSettings.hpp"
#include "Compages/Renderer/Scene/Looks.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"
#include "Compages/World/Entity.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{

// ****************************************************************************
//! \brief Shows a compages::world::World: shapes, cameras, lights, and the
//! frame that draws them.
//!
//! The compages::world::World holds the entities and does not know it is drawn:
//! it runs the same in a test or on a server. The Scene adds what drawing
//! needs, the meshes, the materials, the pictures and the renderer, and makes
//! the common cases one line each:
//!
//! \code
//! compages::world::World m_world;
//! compages::renderer::Scene m_scene{ m_world };
//!
//! Status setUp()
//! {
//!     m_scene.background(0.1f, 0.1f, 0.15f);
//!     m_scene.camera().position(0, 2, 6).add<compages::world::Orbit>();
//!     m_scene.sun();
//!     m_scene.box("crate",
//!     compages::renderer::texture("textures/wooden-crate.jpg")); return
//!     m_scene.prepare();
//! }
//!
//! void draw(compages::core::Frame const& p_frame)
//! {
//!     m_scene.draw(p_frame);
//! }
//! \endcode
//!
//! A mistake made while building, a missing picture for instance, does not
//! stop the building: it is kept and prepare() returns it.
//!
//! Two Scenes may show the same compages::world::World, with their own cameras
//! and their own backgrounds; the second one then shares the assets of the
//! first so that nothing is loaded twice.
// ****************************************************************************
class Scene
{
public:

    // ------------------------------------------------------------------------
    //! \brief Show \c p_world, with assets of its own.
    // ------------------------------------------------------------------------
    explicit Scene(compages::world::World& p_world);

    // ------------------------------------------------------------------------
    //! \brief Show \c p_world with assets shared with someone else, who keeps
    //! them alive longer than this Scene.
    // ------------------------------------------------------------------------
    Scene(compages::world::World& p_world, AssetManager& p_assets);

    ~Scene();
    Scene(Scene const&) = delete;
    Scene& operator=(Scene const&) = delete;

    //! \brief The compages::world::World shown.
    [[nodiscard]] compages::world::World& world() const
    {
        return m_world;
    }
    //! \brief The meshes, the materials, the pictures, the prefabs.
    [[nodiscard]] AssetManager& assets() const
    {
        return *m_assets;
    }

    // --- Shapes --------------------------------------------------------------

    //! \brief A cube one unit wide, centred on its origin.
    compages::world::Entity box(std::string p_name = {},
                                Look const& p_look = {});
    //! \brief A sphere one unit across.
    compages::world::Entity sphere(std::string p_name = {},
                                   Look const& p_look = {});
    //! \brief A square one unit wide standing in XY, facing +Z.
    compages::world::Entity plane(std::string p_name = {},
                                  Look const& p_look = {});
    //! \brief A cylinder one unit across and one unit tall.
    compages::world::Entity cylinder(std::string p_name = {},
                                     Look const& p_look = {});
    //! \brief A cone one unit across and one unit tall.
    compages::world::Entity cone(std::string p_name = {},
                                 Look const& p_look = {});
    //! \brief A pyramid one unit across and one unit tall.
    compages::world::Entity pyramid(std::string p_name = {},
                                    Look const& p_look = {});
    //! \brief Any built-in shape. The mesh is built once per shape and shared.
    compages::world::Entity
    shape(Shape p_shape, std::string p_name = {}, Look const& p_look = {});

    // ------------------------------------------------------------------------
    //! \brief A mesh of your own, with a look.
    //! \param[in] p_mesh built by makeBox(), makeTube()... or by hand.
    // ------------------------------------------------------------------------
    compages::world::Entity
    mesh(MeshAsset p_mesh, std::string p_name = {}, Look const& p_look = {});

    // ------------------------------------------------------------------------
    //! \brief Another entity drawn as \c p_entity is, sharing its mesh and its
    //! look: what a thousand identical shapes are made with. It starts at the
    //! same place, with the same orientation and size.
    // ------------------------------------------------------------------------
    compages::world::Entity copy(compages::world::EntityId p_entity,
                                 std::string p_name = {});

    // ------------------------------------------------------------------------
    //! \brief The mesh of a built-in shape, built on first use and kept under
    //! the name of the shape: "box", "sphere", "plane", "cylinder", "cone",
    //! "pyramid". What a prefab refers to it by.
    // ------------------------------------------------------------------------
    MeshAssetId shapeMesh(Shape p_shape);

    // ------------------------------------------------------------------------
    //! \brief Keep a look under a name, so that a prefab or a saved file can
    //! refer to it.
    // ------------------------------------------------------------------------
    MaterialInstanceId material(std::string p_name, Look const& p_look);

    // ------------------------------------------------------------------------
    //! \brief Change what an entity drawn by this Scene looks like. Its
    //! copies keep the look they had.
    // ------------------------------------------------------------------------
    void look(compages::world::EntityId p_entity, Look const& p_look);

    // ------------------------------------------------------------------------
    //! \brief Draw an entity with a dynamic albedo map already in the catalog.
    //!
    //! Used with \c CanvasTexture after \ref CanvasTexture::bind: the texture
    //! is sampled as \c PbrMinimal base colour.
    // ------------------------------------------------------------------------
    void look(compages::world::EntityId p_entity, TextureAssetId p_albedo);

    // --- Cameras and lights --------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief A perspective camera, at the origin looking down -Z. The first
    //! one becomes the camera render() uses.
    // ------------------------------------------------------------------------
    compages::world::Entity
    camera(std::string p_name = "Camera");

    // ------------------------------------------------------------------------
    //! \brief A directional light, like the sun: only its orientation counts.
    //! It comes from above, in front and to the right; lookAt() turns it.
    // ------------------------------------------------------------------------
    compages::world::Entity sun(std::string p_name = "Sun",
                                compages::core::Vector3f p_color =
                                    compages::core::Vector3f(1.0f, 1.0f, 1.0f),
                                float p_intensity = 1.0f);

    // ------------------------------------------------------------------------
    //! \brief A light bulb: it lights around its position, up to \c p_range.
    // ------------------------------------------------------------------------
    compages::world::Entity lamp(std::string p_name = "Lamp",
                                 compages::core::Vector3f p_color =
                                     compages::core::Vector3f(1.0f, 1.0f, 1.0f),
                                 float p_intensity = 1.0f,
                                 float p_range = 10.0f);

    //! \brief The camera render() uses when given none.
    void activeCamera(compages::world::EntityId p_camera);
    [[nodiscard]] compages::world::Entity activeCamera() const;

    // ------------------------------------------------------------------------
    //! \brief Put a camera and a sun where they see everything drawn. For a
    //! loaded model whose size is not known in advance.
    //! \return the middle of what is drawn: what an Orbit turns around.
    // ------------------------------------------------------------------------
    compages::core::Vector3f frameAll();

    // --- Files and prefabs ---------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief Load a model (glTF, a saved prefab, an STL mesh or a URDF
    //! robot) and place it in the compages::world::World. A URDF robot comes
    //! with its joints, see loadUrdf().
    //! \return the root of what was placed, or why the file could not be read.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<compages::world::Entity>
    load(std::string const& p_path, compages::world::EntityId p_parent = {});

    //! \brief Place one more copy of an already loaded prefab.
    [[nodiscard]] Result<compages::world::Entity>
    instantiate(PrefabId p_prefab,
                compages::world::EntityId p_parent = {},
                compages::world::LocalTransform p_offset = {});

    // ------------------------------------------------------------------------
    //! \brief Play one of the clips a loaded model came with, from its start.
    //! \param[in] p_model what load() returned.
    //! \param[in] p_clip the name of the clip in the file, "Walk" say.
    //! \return false when the model has no clip of that name.
    // ------------------------------------------------------------------------
    bool play(compages::world::EntityId p_model, std::string_view p_clip);

    //! \brief The names of the clips a loaded model came with, in the order
    //! of the file: what a menu offers to play().
    [[nodiscard]] std::vector<std::string>
    clips(compages::world::EntityId p_model) const;

    //! \brief The name of the clip \c p_model plays, or empty.
    [[nodiscard]] std::string playing(compages::world::EntityId p_model) const;

    // --- Environment ---------------------------------------------------------

    //! \brief What the picture starts from.
    Scene& background(float p_red, float p_green, float p_blue);
    //! \brief The dim light coming from everywhere.
    Scene& ambient(float p_red, float p_green, float p_blue);

    // ------------------------------------------------------------------------
    //! \brief Surround everything with six pictures, drawn behind all the
    //! rest, as far away as the sky.
    //! \param[in] p_faces the files of the faces looking along +X, -X, +Y,
    //! -Y, +Z and -Z.
    // ------------------------------------------------------------------------
    Scene& skybox(std::array<std::string, 6u> const& p_faces);

    [[nodiscard]] RenderSettings& renderSettings();
    [[nodiscard]] RenderSettings const& renderSettings() const;
    [[nodiscard]] Environment& environment();
    [[nodiscard]] Environment const& environment() const;

    // --- compages::core::Frame
    // ---------------------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief Send everything built so far to the GPU, and report the first
    //! mistake made while building. What setUp() returns.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status prepare();

    // ------------------------------------------------------------------------
    //! \brief Move the compages::world::World forward one frame: the routines,
    //! then the animations, then the transforms.
    // ------------------------------------------------------------------------
    void update(compages::world::ViewFrame const& p_frame);

    // ------------------------------------------------------------------------
    //! \brief Draw what \c p_camera sees into the open render pass.
    //!
    //! The picture is first cleared to the background. A camera whose
    //! viewport is a part of the picture only clears and draws there, which is
    //! how a split screen or a minimap is made. A mistake is reported to the
    //! frame error channel.
    // ------------------------------------------------------------------------
    void render(compages::world::EntityId p_camera);
    //! \brief Same, with the active camera.
    void render();

    //! \brief update() then render(): the usual frame.
    void draw(compages::world::ViewFrame const& p_frame);

    // --- After a render ------------------------------------------------------

    //! \brief The camera as the last render() saw it: its matrices, its
    //! position, its viewport.
    [[nodiscard]] CameraFrame const& lastCamera() const;

    // ------------------------------------------------------------------------
    //! \brief What is under a pixel, as the last render() saw it.
    //! \param[in] p_pixel from the bottom left of the picture, like
    //! \c Input::mouse.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::optional<compages::renderer::RayHit>
    pick(compages::core::Vector2f p_pixel) const;

    //! \brief Lines drawn over the next render(), then forgotten.
    [[nodiscard]] DebugDraw& debug();

private:

    compages::world::Entity
    drawn(MeshAssetId p_mesh, std::string p_name, Look const& p_look);

private:

    struct Impl;

    compages::world::World& m_world;
    AssetManager* m_assets;
    //! \brief What drawing needs and the user does not see: the renderer, the
    //! sky, the looks, the settings. Behind a pointer so that Scene.hpp does
    //! not bring the drawing machinery with it.
    std::unique_ptr<Impl> m_impl;
};

} // namespace compages::renderer
