// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Scene/Looks.hpp"

#include "Compages/Core/AABB.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Procedural/Primitives.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Entity.hpp"

namespace compages::renderer
{

namespace
{

//! \brief The name a built-in asset is kept under: what a second Scene sharing
//! the assets finds it by, and what a prefab refers to it by.
char const* shapeName(Shape p_shape)
{
    switch (p_shape)
    {
        case Shape::Box:
            return "box";
        case Shape::Sphere:
            return "sphere";
        case Shape::Plane:
            return "plane";
        case Shape::Cylinder:
            return "cylinder";
        case Shape::Cone:
            return "cone";
        case Shape::Pyramid:
            return "pyramid";
    }
    return "shape";
}

//! \brief Turn a mesh built along Z so that it stands along Y, the way the
//! shapes of the Scene all stand.
Result<MeshAsset> standUp(Result<MeshAsset> p_mesh)
{
    if (!p_mesh)
    {
        return p_mesh;
    }
    MeshAsset mesh = p_mesh.take();
    compages::AABB bounds;
    for (MeshVertex& vertex : mesh.source_vertices)
    {
        vertex.position = compages::Vector3f(
            vertex.position.x, -vertex.position.z, vertex.position.y);
        vertex.normal = compages::Vector3f(
            vertex.normal.x, -vertex.normal.z, vertex.normal.y);
        bounds.expand(vertex.position);
    }
    mesh.local_bounds = bounds;
    // The primitive was uploaded lying down; prepare() sends it again upright.
    mesh.releaseGpu();
    return mesh;
}

Result<MeshAsset> buildShape(Shape p_shape)
{
    switch (p_shape)
    {
        case Shape::Box:
            return makeCube();
        case Shape::Sphere:
            return makeSphere(0.5f, 24u, 32u);
        case Shape::Plane:
            return makePlane(1.0f, 1.0f);
        case Shape::Cylinder:
            return standUp(makeCylinder(0.5f, 1.0f, 32u));
        case Shape::Cone:
            return standUp(makeCone(0.5f, 0.0f, 1.0f, 32u));
        case Shape::Pyramid:
            return standUp(makePyramid(0.5f, 1.0f));
    }
    return failure("unknown shape");
}

char const* familyName(ShaderFamily p_family)
{
    switch (p_family)
    {
        case ShaderFamily::Lit:
            return "lit";
        case ShaderFamily::PbrMinimal:
            return "textured";
        case ShaderFamily::Depth:
            return "depth";
        case ShaderFamily::Normals:
            return "normals";
    }
    return "material";
}

Result<Material> buildFamily(ShaderFamily p_family)
{
    switch (p_family)
    {
        case ShaderFamily::Lit:
            return makeLitMaterial();
        case ShaderFamily::PbrMinimal:
            return makePbrMaterial();
        case ShaderFamily::Depth:
            return makeDepthMaterial();
        case ShaderFamily::Normals:
            return makeNormalsMaterial();
    }
    return failure("unknown material family");
}

} // namespace

MeshAssetId Looks::shapeMesh(Shape p_shape)
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

MaterialId Looks::familyMaterial(ShaderFamily p_family)
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

TextureAssetId Looks::pictureTexture(std::filesystem::path const& p_path)
{
    // A picture is registered under its path, as written.
    const std::string name = p_path.string();
    const TextureAssetId known = m_assets->findTexture(name);
    if (known.valid())
    {
        return known;
    }
    TextureAsset picture;
    picture.name = name;
    // A picture is a colour for the eye: stored in sRGB, the GPU turns it
    // back to linear light when the shader samples it.
    compages::gpu::LoadOptions options;
    options.srgb = true;
    if (!compages::gpu::check(picture.texture.loadFile(p_path, options)))
    {
        return {};
    }
    auto id = m_assets->addTexture(name, std::move(picture));
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return {};
    }
    return id.value();
}

MaterialInstance Looks::instanceOf(Look const& p_look)
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

MaterialInstanceId Looks::material(std::string p_name, Look const& p_look)
{
    auto id =
        m_assets->addMaterialInstance(std::move(p_name), instanceOf(p_look));
    if (!id)
    {
        compages::gpu::reportError(id.error());
        return {};
    }
    return id.value();
}

void Looks::wear(compages::world::World& p_world,
                 compages::world::EntityId p_entity,
                 MaterialInstance const& p_instance)
{
    compages::renderer::MeshRenderer* renderer =
        p_world.tryGet<compages::renderer::MeshRenderer>(p_entity);
    if (renderer == nullptr)
    {
        return compages::gpu::reportError(
            "look() on an entity that is not drawn: make it with box(), "
            "sphere(), mesh()...");
    }
    // A new instance rather than a change of the old one, which copies may
    // share. The old one goes if nothing else wears it.
    auto id = m_assets->addMaterialInstance({}, p_instance);
    if (!id)
    {
        return compages::gpu::reportError(id.error());
    }
    const MaterialInstanceId old = renderer->material_instance;
    renderer->material_instance = id.value();

    bool worn = false;
    p_world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId,
            compages::renderer::MeshRenderer const& p_other)
        { worn = worn || (p_other.material_instance == old); });
    if (!worn && m_assets->materialInstanceName(old).empty())
    {
        m_assets->removeMaterialInstance(old);
    }
}

} // namespace compages::renderer
