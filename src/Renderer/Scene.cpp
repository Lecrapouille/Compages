// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "Compages/Renderer/Scene.hpp"

#include "Compages/Core/AABB.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/Renderer/Systems/AnimationSystem.hpp"
#include "Compages/Renderer/Components/Animator.hpp"
#include "Compages/Renderer/Assets/AnimationClip.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"
#include "Compages/Renderer/Assets/StlLoader.hpp"
#include "Compages/Renderer/Assets/UrdfLoader.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Prefab/PrefabInstantiate.hpp"
#include "Compages/Renderer/Render/Picker.hpp"
#include "Compages/Renderer/Render/SceneExtractor.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace compages::renderer
{

using compages::renderer::AnimationSystem;
using compages::renderer::Animator;
using compages::world::Camera;
using compages::world::DirectionalLight;
using compages::renderer::MeshRenderer;
using compages::world::PointLight;
using Viewport = compages::world::Camera::Viewport;

namespace
{

//! \brief The name a built-in asset is kept under: what a second Scene sharing
//! the assets finds it by, and what a prefab refers to it by.
char const* shapeName(Shape p_shape)
{
    switch (p_shape)
    {
        case Shape::Box: return "box";
        case Shape::Sphere: return "sphere";
        case Shape::Plane: return "plane";
        case Shape::Cylinder: return "cylinder";
        case Shape::Cone: return "cone";
        case Shape::Pyramid: return "pyramid";
    }
    return "shape";
}

//! \brief Turn a mesh built along Z so that it stands along Y, the way the
//! shapes of the Scene all stand.
compages::Result<MeshAsset> standUp(compages::Result<MeshAsset> p_mesh)
{
    if (!p_mesh)
    {
        return p_mesh;
    }
    MeshAsset mesh = p_mesh.take();
    AABB bounds;
    for (MeshVertex& vertex : mesh.source_vertices)
    {
        vertex.position = Vector3f(vertex.position.x, -vertex.position.z,
                                   vertex.position.y);
        vertex.normal = Vector3f(vertex.normal.x, -vertex.normal.z,
                                 vertex.normal.y);
        bounds.expand(vertex.position);
    }
    mesh.local_bounds = bounds;
    // The primitive was uploaded lying down; prepare() sends it again upright.
    mesh.releaseGpu();
    return mesh;
}

compages::Result<MeshAsset> buildShape(Shape p_shape)
{
    switch (p_shape)
    {
        case Shape::Box: return makeCube();
        case Shape::Sphere: return makeSphere(0.5f, 24u, 32u);
        case Shape::Plane: return makePlane(1.0f, 1.0f);
        case Shape::Cylinder: return standUp(makeCylinder(0.5f, 1.0f, 32u));
        case Shape::Cone: return standUp(makeCone(0.5f, 0.0f, 1.0f, 32u));
        case Shape::Pyramid: return standUp(makePyramid(0.5f, 1.0f));
    }
    return compages::failure("unknown shape");
}

char const* familyName(ShaderFamily p_family)
{
    switch (p_family)
    {
        case ShaderFamily::Lit: return "lit";
        case ShaderFamily::PbrMinimal: return "textured";
        case ShaderFamily::Depth: return "depth";
        case ShaderFamily::Normals: return "normals";
    }
    return "material";
}

compages::Result<Material> buildFamily(ShaderFamily p_family)
{
    switch (p_family)
    {
        case ShaderFamily::Lit: return makeLitMaterial();
        case ShaderFamily::PbrMinimal: return makePbrMaterial();
        case ShaderFamily::Depth: return makeDepthMaterial();
        case ShaderFamily::Normals: return makeNormalsMaterial();
    }
    return compages::failure("unknown material family");
}

constexpr char const* SKY_VERTEX = R"(#version 450 core
in vec3 position;
uniform mat4 view;
uniform mat4 projection;
out vec3 vDirection;

void main()
{
    vDirection = position;
    // The camera turns but never moves away from the sky: the translation
    // of the view is dropped.
    gl_Position = projection * mat4(mat3(view)) * vec4(position, 1.0);
}
)";

constexpr char const* SKY_FRAGMENT = R"(#version 450 core
in vec3 vDirection;
uniform samplerCube sky;
out vec4 oColor;

void main()
{
    oColor = texture(sky, normalize(vDirection));
}
)";

} // namespace

Scene::Scene(World& p_world)
    : m_world(p_world),
      m_owned_assets(std::make_unique<AssetManager>()),
      m_assets(m_owned_assets.get())
{
}

Scene::Scene(World& p_world, AssetManager& p_assets)
    : m_world(p_world), m_assets(&p_assets)
{
}

Scene::~Scene() = default;

MeshAssetId Scene::shapeMesh(Shape p_shape)
{
    const MeshAssetId known = m_assets->findMesh(shapeName(p_shape));
    if (known.valid())
    {
        return known;
    }
    auto mesh = buildShape(p_shape);
    if (!mesh)
    {
        compages::gpu::reportError(mesh.error());
        return {};
    }
    auto id = m_assets->addMesh(shapeName(p_shape), mesh.take());
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return {};
    }
    return id.value();
}

MaterialId Scene::familyMaterial(ShaderFamily p_family)
{
    const MaterialId known = m_assets->findMaterial(familyName(p_family));
    if (known.valid())
    {
        return known;
    }
    auto material = buildFamily(p_family);
    if (!material)
    {
        compages::gpu::reportError(material.error());
        return {};
    }
    auto id = m_assets->addMaterial(familyName(p_family), material.take());
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return {};
    }
    return id.value();
}

TextureAssetId Scene::pictureTexture(std::string const& p_path)
{
    const TextureAssetId known = m_assets->findTexture(p_path);
    if (known.valid())
    {
        return known;
    }
    TextureAsset picture;
    picture.name = p_path;
    // A picture is a colour for the eye: stored in sRGB, the GPU turns it
    // back to linear light when the shader samples it.
    compages::gpu::LoadOptions options;
    options.srgb = true;
    if (!compages::gpu::check(picture.texture.load(p_path, options)))
    {
        return {};
    }
    auto id = m_assets->addTexture(p_path, std::move(picture));
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return {};
    }
    return id.value();
}

MaterialInstance Scene::instanceOf(Look const& p_look)
{
    MaterialInstance instance;
    instance.material = familyMaterial(p_look.family);
    instance.color = p_look.color;
    instance.base_color_factor = p_look.color;
    instance.depth_near = p_look.depth_near;
    instance.depth_far = p_look.depth_far;
    instance.opacity = p_look.opacity;
    if (!p_look.texture.empty())
    {
        instance.base_color_texture = pictureTexture(p_look.texture);
    }
    return instance;
}

MaterialInstanceId Scene::material(std::string p_name, Look const& p_look)
{
    auto id = m_assets->addMaterialInstance(std::move(p_name), instanceOf(p_look));
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return {};
    }
    return id.value();
}

Entity Scene::drawn(MeshAssetId p_mesh, std::string p_name, Look const& p_look)
{
    Entity entity = m_world.entity(std::move(p_name));
    auto instance = m_assets->addMaterialInstance({}, instanceOf(p_look));
    if (!instance)
    {
        compages::gpu::reportError(instance.error());
        return entity;
    }
    entity.set(MeshRenderer{ p_mesh, instance.value() });
    return entity;
}

Entity Scene::shape(Shape p_shape, std::string p_name, Look const& p_look)
{
    return drawn(shapeMesh(p_shape), std::move(p_name), p_look);
}

Entity Scene::box(std::string p_name, Look const& p_look)
{
    return shape(Shape::Box, std::move(p_name), p_look);
}

Entity Scene::sphere(std::string p_name, Look const& p_look)
{
    return shape(Shape::Sphere, std::move(p_name), p_look);
}

Entity Scene::plane(std::string p_name, Look const& p_look)
{
    return shape(Shape::Plane, std::move(p_name), p_look);
}

Entity Scene::cylinder(std::string p_name, Look const& p_look)
{
    return shape(Shape::Cylinder, std::move(p_name), p_look);
}

Entity Scene::cone(std::string p_name, Look const& p_look)
{
    return shape(Shape::Cone, std::move(p_name), p_look);
}

Entity Scene::pyramid(std::string p_name, Look const& p_look)
{
    return shape(Shape::Pyramid, std::move(p_name), p_look);
}

Entity Scene::mesh(MeshAsset p_mesh, std::string p_name, Look const& p_look)
{
    auto id = m_assets->addMesh({}, std::move(p_mesh));
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return m_world.entity(std::move(p_name));
    }
    return drawn(id.value(), std::move(p_name), p_look);
}

Entity Scene::copy(EntityId p_entity, std::string p_name)
{
    Entity copy = m_world.entity(std::move(p_name));
    if (!m_world.alive(p_entity))
    {
        compages::gpu::reportError("copy() of an entity that does not exist");
        return copy;
    }
    Entity original(m_world, p_entity);
    copy.position(original.position())
        .rotation(original.rotation())
        .scale(original.scale());
    if (MeshRenderer const* renderer = original.find<MeshRenderer>())
    {
        copy.set(*renderer);
    }
    return copy;
}

void Scene::look(EntityId p_entity, Look const& p_look)
{
    MeshRenderer* renderer = m_world.tryGet<MeshRenderer>(p_entity);
    if (renderer == nullptr)
    {
        return compages::gpu::reportError(
            "look() on an entity that is not drawn: make it with box(), "
            "sphere(), mesh()...");
    }
    // A new instance rather than a change of the old one, which copies may
    // share. The old one goes if nothing else wears it.
    auto id = m_assets->addMaterialInstance({}, instanceOf(p_look));
    if (!id)
    {
        return compages::gpu::reportError(id.error());
    }
    const MaterialInstanceId old = renderer->material_instance;
    renderer->material_instance = id.value();

    bool worn = false;
    m_world.each<MeshRenderer>([&](EntityId, MeshRenderer const& p_other) {
        worn = worn || (p_other.material_instance == old);
    });
    if (!worn && m_assets->materialInstanceName(old).empty())
    {
        m_assets->removeMaterialInstance(old);
    }
}

Entity Scene::camera(std::string p_name)
{
    Entity camera = m_world.entity(std::move(p_name)).set(Camera{});
    if (!m_world.alive(m_active_camera))
    {
        m_active_camera = camera;
    }
    return camera;
}

Entity Scene::sun(std::string p_name, Vector3f p_color, float p_intensity)
{
    return m_world.entity(std::move(p_name))
        .set(DirectionalLight{ p_color, p_intensity })
        .position(4.0f, 8.0f, 6.0f)
        .lookAt(0.0f, 0.0f, 0.0f);
}

Entity Scene::lamp(std::string p_name,
                   Vector3f p_color,
                   float p_intensity,
                   float p_range)
{
    return m_world.entity(std::move(p_name))
        .set(PointLight{ p_color, p_intensity, p_range });
}

Vector3f Scene::frameAll()
{
    m_world.update();
    AABB bounds;
    m_world.each<MeshRenderer>(
        [&](EntityId p_entity, MeshRenderer const& p_renderer) {
            MeshAsset const* mesh = m_assets->mesh(p_renderer.mesh);
            if (mesh != nullptr)
            {
                bounds = bounds.merged(
                    mesh->local_bounds.transformed(m_world.worldMatrix(p_entity)));
            }
        });
    if (bounds.empty())
    {
        compages::gpu::reportError("frameAll() with nothing drawn to frame: load or "
                         "make the shapes first");
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    const Vector3f center = bounds.center();
    const Vector3f extent = bounds.extent();
    const float radius = std::max({ extent.x, extent.y, extent.z, 0.05f });

    Entity eye = m_world.alive(m_active_camera) ? activeCamera() : camera();
    eye.position(center + Vector3f(0.0f, radius, radius * 4.0f)).lookAt(center);
    Camera& lens = eye.get<Camera>();
    lens.near_plane = std::max(radius * 0.01f, 0.01f);
    lens.far_plane = std::max(radius * 40.0f, 20.0f);

    sun("Sun", Vector3f(1.0f, 0.98f, 0.92f), 1.2f)
        .position(center + Vector3f(radius, radius * 2.0f, radius))
        .lookAt(center);
    m_environment.ambient = Vector3f(0.22f, 0.22f, 0.24f);
    return center;
}

compages::Result<Entity> Scene::instantiate(PrefabId p_prefab,
                                            EntityId p_parent,
                                            LocalTransform p_offset)
{
    auto root = compages::renderer::instantiate(m_world, *m_assets, p_prefab, p_parent,
                                   p_offset);
    if (!root)
    {
        return compages::failure(root.error());
    }
    return Entity(m_world, root.value());
}

compages::Result<Entity> Scene::load(std::string const& p_path,
                                     EntityId p_parent)
{
    std::string extension = std::filesystem::path(p_path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
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
            return compages::failure(mesh.error());
        }
        Entity drawn = this->mesh(mesh.take(),
                                  std::filesystem::path(p_path).stem().string());
        if (m_world.alive(p_parent))
        {
            drawn.parent(Entity(m_world, p_parent));
        }
        return drawn;
    }

    auto prefab = m_assets->load(p_path);
    if (!prefab)
    {
        return compages::failure(prefab.error());
    }
    return instantiate(prefab.value(), p_parent);
}

bool Scene::play(EntityId p_model, std::string_view p_clip)
{
    Animator* animator = m_world.tryGet<Animator>(p_model);
    if (animator == nullptr)
    {
        return false;
    }
    for (AnimationClipId const id : animator->clips)
    {
        AnimationClip const* clip = m_assets->animation(id);
        if ((clip != nullptr) && (clip->name == p_clip))
        {
            if (!(animator->clip == id))
            {
                animator->clip = id;
                animator->time = 0.0f;
            }
            animator->playing = true;
            return true;
        }
    }
    return false;
}

std::vector<std::string> Scene::clips(EntityId p_model) const
{
    std::vector<std::string> names;
    Animator const* animator = m_world.tryGet<Animator>(p_model);
    if (animator == nullptr)
    {
        return names;
    }
    for (AnimationClipId const id : animator->clips)
    {
        AnimationClip const* clip = m_assets->animation(id);
        if (clip != nullptr)
        {
            names.emplace_back(clip->name);
        }
    }
    return names;
}

std::string Scene::playing(EntityId p_model) const
{
    Animator const* animator = m_world.tryGet<Animator>(p_model);
    AnimationClip const* clip =
        (animator == nullptr) ? nullptr : m_assets->animation(animator->clip);
    return (clip == nullptr) ? std::string() : clip->name;
}

Scene& Scene::background(float p_red, float p_green, float p_blue)
{
    m_settings.clear_color = Vector4f(p_red, p_green, p_blue, 1.0f);
    return *this;
}

Scene& Scene::ambient(float p_red, float p_green, float p_blue)
{
    m_environment.ambient = Vector3f(p_red, p_green, p_blue);
    return *this;
}

Scene& Scene::skybox(std::array<std::string, 6u> const& p_faces)
{
    compages::gpu::LoadOptions options;
    options.flip_vertically = false;
    options.srgb = true;
    options.wrap = compages::gpu::Wrap::ClampToEdge;
    if (!compages::gpu::check(m_sky_texture.loadCube(p_faces, options)) ||
        !compages::gpu::check(m_sky.load(SKY_VERTEX, SKY_FRAGMENT)))
    {
        return *this;
    }
    // A cube around the camera, seen from the inside. Drawn first and
    // without the depth test, it is behind whatever comes after.
    m_sky["position"] = { { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 },
                          { -1, -1, 1 },  { 1, -1, 1 },  { 1, 1, 1 },  { -1, 1, 1 } };
    m_sky.indices({ 0, 1, 2, 2, 3, 0,  4, 6, 5, 6, 4, 7,  0, 3, 7, 7, 4, 0,
                    1, 5, 6, 6, 2, 1,  3, 2, 6, 6, 7, 3,  0, 4, 5, 5, 1, 0 });
    m_sky["sky"] = m_sky_texture;
    m_sky.depthTest(false);
    return *this;
}

compages::Status Scene::prepare()
{
    m_world.each<MeshRenderer>([&](EntityId, MeshRenderer const& p_renderer) {
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
            compages::gpu::check(m_assets->prepare(instance->base_color_texture));
        }
    });
    if (compages::gpu::hasFrameError())
    {
        return compages::failure(compages::gpu::takeFrameError());
    }
    return compages::success();
}

void Scene::update(ViewFrame const& p_frame)
{
    m_world.update(p_frame);
    compages::gpu::check(AnimationSystem::sample(m_world, *m_assets, p_frame.elapsed));
    m_world.update();
    compages::gpu::check(AnimationSystem::pose(m_world, *m_assets));
}

void Scene::render(EntityId p_camera)
{
    if (!compages::gpu::inRenderPass())
    {
        return compages::gpu::reportError(
            "Scene::render() with no render pass open: the pass says where "
            "the picture goes");
    }
    compages::gpu::PassDesc const& where = compages::gpu::currentPass();
    Camera const* lens = m_world.tryGet<Camera>(p_camera);

    // A camera drawing into a part of the picture gets a pass of its own over
    // that part, so that clearing it leaves the rest alone.
    compages::gpu::PassDesc part = where;
    part.color = m_settings.clear_color;
    part.clear_color = true;
    part.clear_depth = true;
    part.target = {};
    if (lens != nullptr)
    {
        Viewport const& v = lens->viewport;
        const float x = std::clamp(v.x, 0.0f, 1.0f);
        const float y = std::clamp(v.y, 0.0f, 1.0f);
        const float w = std::clamp(v.width, 0.0f, 1.0f - x);
        const float h = std::clamp(v.height, 0.0f, 1.0f - y);
        part.x = where.x + std::uint32_t(std::lround(x * float(where.width)));
        part.y = where.y + std::uint32_t(std::lround(y * float(where.height)));
        part.width = std::max(1u, std::uint32_t(std::lround(w * float(where.width))));
        part.height = std::max(1u, std::uint32_t(std::lround(h * float(where.height))));
    }
    compages::gpu::RenderPass pass(part);
    if (!pass.open())
    {
        return;
    }

    auto snapshot = SceneExtractor::extract(*this, p_camera, part.width,
                                            part.height);
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
    m_last_camera = snapshot.value().camera;
    m_last_camera.viewport_x = float(part.x - where.x);
    m_last_camera.viewport_y = float(part.y - where.y);

    if (m_sky_texture.valid())
    {
        m_sky["view"] = snapshot.value().camera.view;
        m_sky["projection"] = snapshot.value().camera.projection;
        m_sky.draw();
    }

    compages::gpu::check(m_renderer.render(snapshot.value(), *m_assets));
    compages::gpu::check(m_debug.flush(snapshot.value().camera));
    m_debug.clear();
}

void Scene::render()
{
    render(m_active_camera);
}

void Scene::draw(ViewFrame const& p_frame)
{
    update(p_frame);
    render();
}

std::optional<RayHit> Scene::pick(Vector2f p_pixel) const
{
    if ((m_last_camera.viewport_width == 0u) ||
        (m_last_camera.viewport_height == 0u))
    {
        return std::nullopt;
    }
    return pickAt(*this, m_last_camera,
                  p_pixel.x - m_last_camera.viewport_x,
                  p_pixel.y - m_last_camera.viewport_y,
                  m_last_camera.viewport_width,
                  m_last_camera.viewport_height);
}

} // namespace compages::renderer
