// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/Renderer/Render/Renderer.hpp"
#include "../Render/Skybox.hpp"
#include "Compages/World/Entity.hpp"

#include "Compages/Core/AABB.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Import/StlLoader.hpp"
#include "Compages/Renderer/Import/UrdfLoader.hpp"
#include "Compages/Renderer/Animation/Animator.hpp"
#include "Compages/Renderer/Components/Helpers.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Import/PrefabInstantiate.hpp"
#include "Compages/Renderer/Render/Picker.hpp"
#include "Compages/Renderer/Render/SceneExtractor.hpp"
#include "Compages/Renderer/Animation/AnimationSystem.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Components/Light.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace compages::renderer
{

namespace
{

// ----------------------------------------------------------------------------
//! \brief Turn the helper components (Grid, Axes, ShowBounds) into lines,
//! where their entities are now.
// ----------------------------------------------------------------------------
void queueHelpers(compages::world::World& p_world,
                  AssetManager const& p_assets,
                  DebugDraw& p_debug)
{
    p_world.each<Grid>(
        [&](compages::world::EntityId p_entity, Grid const& p_grid)
        {
            if (!p_world.enabledInHierarchy(p_entity))
            {
                return;
            }
            const compages::Matrix44f& m = p_world.worldMatrix(p_entity);
            const float half = 0.5f * p_grid.size;
            const float step = p_grid.size / float(p_grid.divisions);
            for (std::uint32_t i = 0u; i <= p_grid.divisions; ++i)
            {
                const float t = -half + (float(i) * step);
                const bool centre = (2u * i) == p_grid.divisions;
                const compages::Vector3f color =
                    centre ? p_grid.color * 1.8f : p_grid.color;
                p_debug.line(compages::transformPoint(m, { t, 0.0f, -half }),
                             compages::transformPoint(m, { t, 0.0f, half }),
                             color);
                p_debug.line(compages::transformPoint(m, { -half, 0.0f, t }),
                             compages::transformPoint(m, { half, 0.0f, t }),
                             color);
            }
        });
    p_world.each<Axes>(
        [&](compages::world::EntityId p_entity, Axes const& p_axes)
        {
            if (!p_world.enabledInHierarchy(p_entity))
            {
                return;
            }
            const compages::Matrix44f& m = p_world.worldMatrix(p_entity);
            const compages::Vector3f origin =
                compages::transformPoint(m, { 0.0f, 0.0f, 0.0f });
            const float l = p_axes.length;
            p_debug.line(origin,
                         compages::transformPoint(m, { l, 0.0f, 0.0f }),
                         { 1.0f, 0.2f, 0.2f });
            p_debug.line(origin,
                         compages::transformPoint(m, { 0.0f, l, 0.0f }),
                         { 0.2f, 1.0f, 0.2f });
            p_debug.line(origin,
                         compages::transformPoint(m, { 0.0f, 0.0f, l }),
                         { 0.3f, 0.5f, 1.0f });
        });
    p_world.each<ShowBounds, MeshRenderer>(
        [&](compages::world::EntityId p_entity,
            ShowBounds const& p_show,
            MeshRenderer const& p_renderer)
        {
            MeshAsset const* mesh = p_assets.mesh(p_renderer.mesh);
            if ((mesh == nullptr) || !p_world.enabledInHierarchy(p_entity))
            {
                return;
            }
            // A little larger than the mesh: lines lying on its faces would
            // be half hidden by them.
            compages::AABB box = mesh->local_bounds;
            const compages::Vector3f margin = (box.max - box.min) * 0.02f;
            box.min = box.min - margin;
            box.max = box.max + margin;
            p_debug.box(box, p_world.worldMatrix(p_entity), p_show.color);
        });
}

} // namespace

// ****************************************************************************
//! \brief The private part of a Scene: what drawing needs.
// ****************************************************************************
struct Scene::Impl
{
    Impl(std::unique_ptr<AssetManager> p_owned, AssetManager& p_assets)
        : owned_assets(std::move(p_owned)), looks(p_assets)
    {
    }

    //! \brief The assets of the Scene, when it was not given some.
    std::unique_ptr<AssetManager> owned_assets;
    Looks looks;
    Renderer renderer;
    DebugDraw debug;
    compages::world::EntityId active_camera{};
    RenderSettings settings{};
    Environment environment{};
    CameraFrame last_camera{};
    Skybox sky;
};

Scene::Scene(compages::world::World& p_world)
    : m_world(p_world), m_assets(nullptr)
{
    auto owned = std::make_unique<AssetManager>();
    m_assets = owned.get();
    m_impl = std::make_unique<Impl>(std::move(owned), *m_assets);
}

Scene::Scene(compages::world::World& p_world, AssetManager& p_assets)
    : m_world(p_world),
      m_assets(&p_assets),
      m_impl(std::make_unique<Impl>(nullptr, p_assets))
{
}

Scene::~Scene() = default;

MeshAssetId Scene::shapeMesh(Shape p_shape)
{
    return m_impl->looks.shapeMesh(p_shape);
}

MaterialInstanceId Scene::material(std::string p_name, Look const& p_look)
{
    return m_impl->looks.material(std::move(p_name), p_look);
}

void Scene::activeCamera(compages::world::EntityId p_camera)
{
    m_impl->active_camera = p_camera;
}

compages::world::Entity Scene::activeCamera() const
{
    return compages::world::Entity(m_world, m_impl->active_camera);
}

RenderSettings& Scene::renderSettings()
{
    return m_impl->settings;
}

RenderSettings const& Scene::renderSettings() const
{
    return m_impl->settings;
}

Environment& Scene::environment()
{
    return m_impl->environment;
}

Environment const& Scene::environment() const
{
    return m_impl->environment;
}

CameraFrame const& Scene::lastCamera() const
{
    return m_impl->last_camera;
}

DebugDraw& Scene::debug()
{
    return m_impl->debug;
}

compages::world::Entity
Scene::drawn(MeshAssetId p_mesh, std::string p_name, Look const& p_look)
{
    compages::world::Entity entity = m_world.entity(std::move(p_name));
    auto instance = m_assets->addMaterialInstance({}, m_impl->looks.instanceOf(p_look));
    if (!instance)
    {
        compages::gpu::reportError(instance.error());
        return entity;
    }
    entity.set(compages::renderer::MeshRenderer{ p_mesh, instance.value() });
    return entity;
}

compages::world::Entity
Scene::shape(Shape p_shape, std::string p_name, Look const& p_look)
{
    return drawn(m_impl->looks.shapeMesh(p_shape), std::move(p_name), p_look);
}

compages::world::Entity Scene::box(std::string p_name, Look const& p_look)
{
    return shape(Shape::Box, std::move(p_name), p_look);
}

compages::world::Entity Scene::sphere(std::string p_name, Look const& p_look)
{
    return shape(Shape::Sphere, std::move(p_name), p_look);
}

compages::world::Entity Scene::plane(std::string p_name, Look const& p_look)
{
    return shape(Shape::Plane, std::move(p_name), p_look);
}

compages::world::Entity Scene::cylinder(std::string p_name, Look const& p_look)
{
    return shape(Shape::Cylinder, std::move(p_name), p_look);
}

compages::world::Entity Scene::cone(std::string p_name, Look const& p_look)
{
    return shape(Shape::Cone, std::move(p_name), p_look);
}

compages::world::Entity Scene::pyramid(std::string p_name, Look const& p_look)
{
    return shape(Shape::Pyramid, std::move(p_name), p_look);
}

compages::world::Entity
Scene::mesh(MeshAsset p_mesh, std::string p_name, Look const& p_look)
{
    auto id = m_assets->addMesh({}, std::move(p_mesh));
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return m_world.entity(std::move(p_name));
    }
    return drawn(id.value(), std::move(p_name), p_look);
}

compages::world::Entity Scene::copy(compages::world::EntityId p_entity,
                                    std::string p_name)
{
    compages::world::Entity copy = m_world.entity(std::move(p_name));
    if (!m_world.alive(p_entity))
    {
        compages::gpu::reportError("copy() of an entity that does not exist");
        return copy;
    }
    compages::world::Entity original(m_world, p_entity);
    copy.position(original.position())
        .rotation(original.rotation())
        .scale(original.scale());
    if (compages::renderer::MeshRenderer const* renderer =
            original.find<compages::renderer::MeshRenderer>())
    {
        copy.set(*renderer);
    }
    return copy;
}

void Scene::look(compages::world::EntityId p_entity, Look const& p_look)
{
    m_impl->looks.wear(m_world, p_entity, m_impl->looks.instanceOf(p_look));
}

void Scene::look(compages::world::EntityId p_entity, TextureAssetId p_albedo)
{
    MaterialInstance instance = m_impl->looks.instanceOf(texture({}));
    instance.base_color_texture = p_albedo;
    m_impl->looks.wear(m_world, p_entity, instance);
}

compages::world::Entity Scene::camera(std::string p_name)
{
    compages::world::Entity camera =
        m_world.entity(std::move(p_name)).set(compages::world::Camera{});
    if (!m_world.alive(m_impl->active_camera))
    {
        m_impl->active_camera = camera;
    }
    return camera;
}

compages::world::Entity Scene::sun(std::string p_name,
                                   compages::Vector3f p_color,
                                   float p_intensity)
{
    return m_world.entity(std::move(p_name))
        .set(compages::world::DirectionalLight{ p_color, p_intensity })
        .position(4.0f, 8.0f, 6.0f)
        .lookAt(0.0f, 0.0f, 0.0f);
}

compages::world::Entity Scene::lamp(std::string p_name,
                                    compages::Vector3f p_color,
                                    float p_intensity,
                                    float p_range)
{
    return m_world.entity(std::move(p_name))
        .set(compages::world::PointLight{ p_color, p_intensity, p_range });
}

compages::world::Entity Scene::grid(float p_size, std::uint32_t p_divisions)
{
    return m_world.entity("Grid").set(
        Grid{ .size = p_size, .divisions = std::max(1u, p_divisions) });
}

compages::world::Entity Scene::axes(float p_length)
{
    return m_world.entity("Axes").set(Axes{ .length = p_length });
}

void Scene::showBounds(compages::world::EntityId p_entity, bool p_show)
{
    compages::world::Entity entity(m_world, p_entity);
    if (p_show)
    {
        entity.set(ShowBounds{});
    }
    else
    {
        entity.remove<ShowBounds>();
    }
}

compages::Vector3f Scene::frameAll()
{
    m_world.update();
    compages::AABB bounds;
    m_world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId p_entity,
            compages::renderer::MeshRenderer const& p_renderer)
        {
            MeshAsset const* mesh = m_assets->mesh(p_renderer.mesh);
            if (mesh != nullptr)
            {
                bounds = bounds.merged(mesh->local_bounds.transformed(
                    m_world.worldMatrix(p_entity)));
            }
        });
    if (bounds.empty())
    {
        compages::gpu::reportError(
            "frameAll() with nothing drawn to frame: load or "
            "make the shapes first");
        return compages::Vector3f(0.0f, 0.0f, 0.0f);
    }

    const compages::Vector3f center = bounds.center();
    const compages::Vector3f extent = bounds.extent();
    const float radius = std::max({ extent.x, extent.y, extent.z, 0.05f });

    compages::world::Entity eye =
        m_world.alive(m_impl->active_camera) ? activeCamera() : camera();
    eye.position(center + compages::Vector3f(0.0f, radius, radius * 4.0f))
        .lookAt(center);
    compages::world::Camera& lens = eye.get<compages::world::Camera>();
    lens.near_plane = std::max(radius * 0.01f, 0.01f);
    lens.far_plane = std::max(radius * 40.0f, 20.0f);

    sun("Sun", compages::Vector3f(1.0f, 0.98f, 0.92f), 1.2f)
        .position(center +
                  compages::Vector3f(radius, radius * 2.0f, radius))
        .lookAt(center);
    m_impl->environment.ambient = compages::Vector3f(0.22f, 0.22f, 0.24f);
    return center;
}

Result<compages::world::Entity>
Scene::instantiate(PrefabId p_prefab,
                   compages::world::EntityId p_parent,
                   compages::world::LocalTransform p_offset)
{
    auto root = compages::renderer::instantiate(
        m_world, *m_assets, p_prefab, p_parent, p_offset);
    if (!root)
    {
        return failure(root.error());
    }
    return compages::world::Entity(m_world, root.value());
}

Result<compages::world::Entity>
Scene::loadFile(std::filesystem::path const& p_path,
                compages::world::EntityId p_parent)
{
    std::string extension = p_path.extension().string();
    std::transform(extension.begin(),
                   extension.end(),
                   extension.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    if (extension == ".urdf")
    {
        return loadUrdf(*this, p_path, p_parent);
    }
    if (extension == ".stl")
    {
        auto mesh = loadStl(p_path);
        if (!mesh)
        {
            return failure(mesh.error());
        }
        compages::world::Entity drawn =
            this->mesh(mesh.take(), p_path.stem().string());
        if (m_world.alive(p_parent))
        {
            drawn.parent(compages::world::Entity(m_world, p_parent));
        }
        return drawn;
    }

    auto prefab = m_assets->loadFile(p_path);
    if (!prefab)
    {
        return failure(prefab.error());
    }
    return instantiate(prefab.value(), p_parent);
}

bool Scene::play(compages::world::EntityId p_model, std::string_view p_clip)
{
    return AnimationSystem::play(m_world, *m_assets, p_model, p_clip);
}

std::vector<std::string> Scene::clips(compages::world::EntityId p_model) const
{
    return AnimationSystem::clips(m_world, *m_assets, p_model);
}

std::string Scene::playing(compages::world::EntityId p_model) const
{
    return AnimationSystem::playing(m_world, *m_assets, p_model);
}

Scene& Scene::background(float p_red, float p_green, float p_blue)
{
    m_impl->settings.clear_color =
        compages::Vector4f(p_red, p_green, p_blue, 1.0f);
    return *this;
}

Scene& Scene::ambient(float p_red, float p_green, float p_blue)
{
    m_impl->environment.ambient = compages::Vector3f(p_red, p_green, p_blue);
    return *this;
}

Scene& Scene::skybox(std::array<std::filesystem::path, 6u> const& p_faces)
{
    compages::gpu::check(m_impl->sky.loadFiles(p_faces));
    return *this;
}

Status Scene::prepare()
{
    m_world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId,
            compages::renderer::MeshRenderer const& p_renderer)
        {
            if (m_assets->mesh(p_renderer.mesh) != nullptr)
            {
                compages::gpu::check(m_assets->prepare(p_renderer.mesh));
            }
            MaterialInstance const* instance =
                m_assets->materialInstance(p_renderer.material_instance);
            if (instance == nullptr)
            {
                return;
            }
            if (m_assets->material(instance->material) != nullptr)
            {
                compages::gpu::check(m_assets->prepare(instance->material));
            }
            if (instance->base_color_texture.valid())
            {
                compages::gpu::check(
                    m_assets->prepare(instance->base_color_texture));
            }
        });
    if (compages::gpu::hasFrameError())
    {
        return failure(compages::gpu::takeFrameError());
    }
    return success();
}

void Scene::update(compages::world::ViewFrame const& p_frame)
{
    // Routines and clips write local poses first; one update then turns the
    // joints into poses (a joint wins over a clip, see PoseDriver) and
    // computes every world matrix once.
    m_world.step(p_frame);
    compages::gpu::check(compages::renderer::AnimationSystem::sample(
        m_world, *m_assets, p_frame.elapsed));
    m_world.update();
    m_world.endStep();
    compages::gpu::check(
        compages::renderer::AnimationSystem::pose(m_world, *m_assets));
}

void Scene::render(compages::world::EntityId p_camera)
{
    if (!compages::gpu::inRenderPass())
    {
        return compages::gpu::reportError(
            "Scene::render() with no render pass open: the pass says where "
            "the picture goes");
    }
    // A copy: opening the pass of the camera below adds to the stack of
    // passes, and a reference into it would then point at freed memory.
    const compages::gpu::PassDesc where = compages::gpu::currentPass();
    compages::world::Camera const* lens =
        m_world.tryGet<compages::world::Camera>(p_camera);

    // A camera drawing into a part of the picture gets a pass of its own over
    // that part, so that clearing it leaves the rest alone.
    compages::gpu::PassDesc part = where;
    part.color = m_impl->settings.clear_color;
    part.clear_color = true;
    part.clear_depth = true;
    part.target = {};
    // Reversed depth: precise from a metre to the Moon (with a Depth32F
    // target, as the gallery and renderToFile have).
    part.reversed_depth = true;
    if (lens != nullptr)
    {
        compages::world::Camera::Viewport const& v = lens->viewport;
        const float x = std::clamp(v.x, 0.0f, 1.0f);
        const float y = std::clamp(v.y, 0.0f, 1.0f);
        const float w = std::clamp(v.width, 0.0f, 1.0f - x);
        const float h = std::clamp(v.height, 0.0f, 1.0f - y);
        // Round the two edges, not the origin and the size: on 667 pixels,
        // a half from 0.5 would start at 334 and be 334 wide, one pixel
        // outside the target. Two halves sharing an edge then share it
        // exactly, without a gap or an overlap.
        const auto edge = [](float p_fraction, std::uint32_t p_size)
        {
            return std::min(p_size,
                            std::uint32_t(std::lround(p_fraction *
                                                      float(p_size))));
        };
        const std::uint32_t left = edge(x, where.width);
        const std::uint32_t right = edge(x + w, where.width);
        const std::uint32_t bottom = edge(y, where.height);
        const std::uint32_t top = edge(y + h, where.height);
        part.x = where.x + std::min(left, where.width - 1u);
        part.y = where.y + std::min(bottom, where.height - 1u);
        part.width = std::max(1u, right - std::min(left, right));
        part.height = std::max(1u, top - std::min(bottom, top));
        part.width = std::min(part.width, where.x + where.width - part.x);
        part.height = std::min(part.height, where.y + where.height - part.y);
    }
    compages::gpu::RenderPass pass(part);
    if (!pass.open())
    {
        return;
    }

    // The extractor takes the size of the whole target and applies the
    // viewport of the camera itself: given the part, it would apply it twice
    // and squash a split view to half its width.
    auto snapshot =
        SceneExtractor::extract(*this, p_camera, where.width, where.height);
    if (!snapshot)
    {
        return compages::gpu::reportError(snapshot.error());
    }
    // The extractor places the viewport inside the size it was given: the
    // pass already is that part, so the camera frame starts at its corner.
    snapshot.value().camera.viewport_x = 0.0f;
    snapshot.value().camera.viewport_y = 0.0f;
    snapshot.value().camera.viewport_width = part.width;
    snapshot.value().camera.viewport_height = part.height;
    m_impl->last_camera = snapshot.value().camera;
    m_impl->last_camera.viewport_x = float(part.x - where.x);
    m_impl->last_camera.viewport_y = float(part.y - where.y);

    m_impl->sky.draw(snapshot.value().camera);

    compages::gpu::check(m_impl->renderer.render(snapshot.value(), *m_assets));
    queueHelpers(m_world, *m_assets, m_impl->debug);
    compages::gpu::check(m_impl->debug.flush(snapshot.value().camera));
    m_impl->debug.clear();
}

void Scene::render()
{
    render(m_impl->active_camera);
}

Status Scene::renderToFile(std::filesystem::path const& p_path,
                           std::uint32_t p_width,
                           std::uint32_t p_height)
{
    // A colour picture and the distances that let near faces hide far ones.
    compages::gpu::Texture color;
    COMPAGES_TRY_ASSIGN(color,
                        compages::gpu::Texture::create(
                            { .format = compages::gpu::PixelFormat::RGBA8,
                              .width = p_width,
                              .height = p_height }));
    compages::gpu::Texture depth;
    COMPAGES_TRY_ASSIGN(depth,
                        compages::gpu::Texture::create(
                            { .format = compages::gpu::PixelFormat::Depth32F,
                              .width = p_width,
                              .height = p_height }));
    compages::gpu::Framebuffer target;
    COMPAGES_TRY_ASSIGN(target, compages::gpu::Framebuffer::create(color, depth));

    COMPAGES_TRY(compages::gpu::attempt(
        [&]
        {
            compages::gpu::RenderPass pass(target);
            render();
        }));
    return color.saveFile(p_path);
}

void Scene::draw(compages::world::ViewFrame const& p_frame)
{
    update(p_frame);
    render();
}

std::optional<compages::renderer::RayHit>
Scene::pick(compages::Vector2f p_pixel) const
{
    if ((m_impl->last_camera.viewport_width == 0u) ||
        (m_impl->last_camera.viewport_height == 0u))
    {
        return std::nullopt;
    }
    return pickAt(*this,
                  m_impl->last_camera,
                  p_pixel.x - m_impl->last_camera.viewport_x,
                  p_pixel.y - m_impl->last_camera.viewport_y,
                  m_impl->last_camera.viewport_width,
                  m_impl->last_camera.viewport_height);
}

} // namespace compages::renderer
