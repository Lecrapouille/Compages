// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Renderer/Components/Text.hpp"
#include "Compages/Fields/Field.hpp"
#include "Compages/Renderer/Scene/Environment.hpp"
#include "Compages/Renderer/Visualization/FieldLook.hpp"
#include "Compages/Renderer/Render/Raycast.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"
#include "Compages/Renderer/Render/DebugDraw.hpp"
#include "Compages/Renderer/Scene/RenderSettings.hpp"
#include "Compages/Renderer/Scene/Looks.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"
#include "Compages/World/Entity.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
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
//! void draw(compages::Frame const& p_frame)
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
class Scene: private compages::NotCopiable
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

    // ------------------------------------------------------------------------
    //! \brief Draw an entity with a material of the catalog (made with
    //! Material::create()), keeping its colour, picture and opacity. Its
    //! copies keep the material they had.
    // ------------------------------------------------------------------------
    void look(compages::world::EntityId p_entity, MaterialId p_material);

    // ------------------------------------------------------------------------
    //! \brief Draw a field: one coloured point per value, on the ground of its
    //! grid (x, z), raised by its value when the look has a relief.
    //!
    //! The values go to the GPU as they are; the colours and the heights are
    //! made there, so a new range or new values cost no new mesh.
    //! \param[in] p_field The values.
    //! \param[in] p_look The colours, the range, the relief, the light.
    //! \param[in] p_name The name of the entity.
    //! \return The entity; an error (a range of zero, an empty field) is
    //! returned by prepare().
    // ------------------------------------------------------------------------
    compages::world::Entity show(compages::Field<float> const& p_field,
                                 FieldLook const& p_look,
                                 std::string p_name);

    // ------------------------------------------------------------------------
    //! \brief Show new values, or the same with another look, on an entity
    //! made by show(): the grid must keep its size.
    //! \return Why it could not: not a field, another size, a range of zero.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status show(compages::world::EntityId p_entity,
                              compages::Field<float> const& p_field,
                              FieldLook const& p_look);

    // ------------------------------------------------------------------------
    //! \brief Draw a field whose values are on the GPU already: a storage
    //! buffer of (at least) p_grid.count() floats, row after row, that a
    //! compute shader writes. Nothing comes back to the CPU.
    //! \param[in] p_values Borrowed: it must live as long as the entity
    //! draws (with a PingPong, give current() again after each swap).
    // ------------------------------------------------------------------------
    compages::world::Entity show(compages::Grid2D const& p_grid,
                                 compages::gpu::StorageBuffer<float> const& p_values,
                                 FieldLook const& p_look,
                                 std::string p_name);

    // ------------------------------------------------------------------------
    //! \brief Another buffer of values, or another look, for an entity made
    //! by show(): the buffer must hold as many floats as the grid.
    //! \return Why it could not: not a field, a range of zero.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status show(compages::world::EntityId p_entity,
                              compages::gpu::StorageBuffer<float> const& p_values,
                              FieldLook const& p_look);

    // ------------------------------------------------------------------------
    //! \brief Draw a field of vectors as arrows on the ground of its grid (the
    //! x and y of each vector go along x and z), coloured by their length.
    //! The arrows are a mesh made on the CPU, made again by each new show().
    // ------------------------------------------------------------------------
    compages::world::Entity show(compages::Field<compages::Vector2f> const& p_field,
                                 ArrowLook const& p_look,
                                 std::string p_name);

    // ------------------------------------------------------------------------
    //! \brief New vectors, or another look, for arrows made by show().
    // ------------------------------------------------------------------------
    [[nodiscard]] Status show(compages::world::EntityId p_entity,
                              compages::Field<compages::Vector2f> const& p_field,
                              ArrowLook const& p_look);

    // ------------------------------------------------------------------------
    //! \brief Draw a volume: a box around its grid, see-through, whose
    //! colours add up along each ray (the colour map of the look; a value at
    //! the top of the range lets 5 % of the light through across the box).
    //! The relief and the light of the look are not used. The values go to
    //! the GPU as a 3D texture.
    //! \return The entity; an error (a range of zero, fewer than 2 points
    //! along an axis) is returned by prepare().
    // ------------------------------------------------------------------------
    compages::world::Entity show(compages::Field<float, compages::Grid3D> const& p_volume,
                                 FieldLook const& p_look,
                                 std::string p_name);

    // ------------------------------------------------------------------------
    //! \brief New values, or another look, for a volume made by show(): the
    //! grid must keep its size.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status show(compages::world::EntityId p_entity,
                              compages::Field<float, compages::Grid3D> const& p_volume,
                              FieldLook const& p_look);

    // ------------------------------------------------------------------------
    //! \brief Read a field under a pixel of the last picture: the point the
    //! ray of the pixel meets on the field drawn with this look, and the value
    //! there. For a field on the ground of its grid, as show() draws it.
    //! \param[in] p_pixel The pixel, as Input::mouse gives it.
    //! \return The point and the value, or none when the ray misses.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::optional<FieldProbe>
    probe(compages::Field<float> const& p_field,
          FieldLook const& p_look,
          compages::Vector2f p_pixel) const;

    // ------------------------------------------------------------------------
    //! \brief Give an entity drawn by this Scene new geometry, in place: its
    //! copies, which share its mesh, change with it.
    //! \return Why the mesh could not be uploaded, or that the entity is not
    //! drawn.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status reshape(compages::world::EntityId p_entity,
                                 MeshAsset p_mesh);

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
                                compages::Vector3f p_color =
                                    compages::Vector3f(1.0f, 1.0f, 1.0f),
                                float p_intensity = 1.0f);

    // ------------------------------------------------------------------------
    //! \brief A light bulb: it lights around its position, up to \c p_range.
    // ------------------------------------------------------------------------
    compages::world::Entity lamp(std::string p_name = "Lamp",
                                 compages::Vector3f p_color =
                                     compages::Vector3f(1.0f, 1.0f, 1.0f),
                                 float p_intensity = 1.0f,
                                 float p_range = 10.0f);

    // --- Helpers -------------------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief A grid on the ground (the XZ plane), centred on the origin:
    //! what shows scale and height at a glance. It is an entity: move it,
    //! hide it, destroy it.
    //! \param[in] p_size width of the whole grid.
    //! \param[in] p_divisions cells along one side.
    // ------------------------------------------------------------------------
    compages::world::Entity grid(float p_size, std::uint32_t p_divisions);

    // ------------------------------------------------------------------------
    //! \brief Three axes, X red, Y green, Z blue. Hang them from an entity to
    //! see how it is placed and turned:
    //! \code
    //! scene.axes(0.3f).parent(arm);
    //! \endcode
    //! \param[in] p_length length of each axis.
    // ------------------------------------------------------------------------
    compages::world::Entity axes(float p_length);

    // ------------------------------------------------------------------------
    //! \brief Show or hide the box around the mesh of an entity. The box
    //! turns and moves with it.
    // ------------------------------------------------------------------------
    void showBounds(compages::world::EntityId p_entity, bool p_show);

    //! \brief The camera render() uses when given none.
    void activeCamera(compages::world::EntityId p_camera);
    [[nodiscard]] compages::world::Entity activeCamera() const;

    // ------------------------------------------------------------------------
    //! \brief Put a camera and a sun where they see everything drawn. For a
    //! loaded model whose size is not known in advance.
    //! \return the middle of what is drawn: what an Orbit turns around.
    // ------------------------------------------------------------------------
    compages::Vector3f frameAll();

    // --- Files and prefabs ---------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief Load a model (glTF, a saved prefab, an STL mesh or a URDF
    //! robot) and place it in the compages::world::World. A URDF robot comes
    //! with its joints, see loadUrdf().
    //! \return the root of what was placed, or why the file could not be read.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<compages::world::Entity>
    loadFile(std::filesystem::path const& p_path,
             compages::world::EntityId p_parent = {});

    //! \brief Place one more copy of an already loaded prefab.
    [[nodiscard]] Result<compages::world::Entity>
    instantiate(PrefabId p_prefab,
                compages::world::EntityId p_parent = {},
                compages::world::LocalTransform p_offset = {});

    // ------------------------------------------------------------------------
    //! \brief Play one of the clips a loaded model came with, from its start.
    //! \param[in] p_model what loadFile() returned.
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
    Scene& skybox(std::array<std::filesystem::path, 6u> const& p_faces);

    // ------------------------------------------------------------------------
    //! \brief The font of the Label and Caption components (text on the
    //! picture, Renderer/Components/Text.hpp): its letters are baked once
    //! into a picture of distance fields, sharp at any size. A failure (a
    //! missing file, not a font) is returned by prepare().
    //! \code
    //! scene.font(dataPath("fonts/DejaVuSans.ttf"));
    //! scene.sphere("Mars", look).set(compages::renderer::Label{
    //!     .text = "Mars", .color = { 1.0f, 0.95f, 0.85f }, .pixels = 16.0f,
    //!     .hide_closer_than = 0.0f });
    //! \endcode
    //! \param[in] p_file A TrueType or OpenType font file.
    //! \return This Scene, to chain.
    // ------------------------------------------------------------------------
    Scene& font(std::filesystem::path const& p_file);

    [[nodiscard]] RenderSettings& renderSettings();
    [[nodiscard]] RenderSettings const& renderSettings() const;
    [[nodiscard]] Environment& environment();
    [[nodiscard]] Environment const& environment() const;

    // --- compages::Frame
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

    // ------------------------------------------------------------------------
    //! \brief Draw what the active camera sees into a PNG file, without a
    //! window: an off-screen picture of p_width by p_height pixels.
    //!
    //! For a report, a thumbnail, a test that compares pictures, or a server
    //! with no screen (only an OpenGL context is needed, see the
    //! Compages-headless program). Like render(), it draws the world as the
    //! last update() left it, and it waits for the GPU: not for every frame.
    //!
    //! \code
    //! scene.update(frame);
    //! COMPAGES_TRY(scene.renderToFile("scene.png", 1280u, 720u));
    //! \endcode
    //!
    //! \param[in] p_path the PNG file to write.
    //! \param[in] p_width width of the picture, in pixels.
    //! \param[in] p_height height of the picture, in pixels.
    //! \return the first mistake met while drawing or writing.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status renderToFile(std::filesystem::path const& p_path,
                                      std::uint32_t p_width,
                                      std::uint32_t p_height);

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
    pick(compages::Vector2f p_pixel) const;

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
