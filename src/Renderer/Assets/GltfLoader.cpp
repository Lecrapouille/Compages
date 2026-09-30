// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Assets/GltfLoader.hpp"

#include "Compages/Renderer/Assets/AnimationClip.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"
#include "Compages/Renderer/Assets/Skin.hpp"
#include "Compages/Renderer/Components/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/World.hpp"

#include "Compages/GPU/Core/PixelFormat.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/Texture.hpp"

#include "CgltfInclude.hpp"

#include <stb_image.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace compages::renderer
{

// Transitional implementation detail used only by the old importer body while
// the public API is loadGltf() + Scene::instantiate().
struct GltfImport
{
    compages::world::EntityId root{};
    compages::world::EntityId animator{};
    std::vector<AnimationClipId> animations;
    std::size_t mesh_count = 0u;
    std::size_t texture_count = 0u;
    std::size_t node_count = 0u;
    std::size_t skin_count = 0u;
};

namespace
{

using TextureMap = std::unordered_map<cgltf_image*, TextureAssetId>;

// --- Embedded images ---------------------------------------------------------
// Decode buffer-view pixels into TextureAsset (sRGB base colour by default).

[[nodiscard]] Result<TextureAsset>
textureFromMemory(std::span<const std::byte> p_pixels, bool p_srgb)
{
    // glTF UV (0, 0) is the top-left of the image file. OpenGL treats the
    // first row in memory as V = 0. Leaving the file unflipped therefore
    // matches the accessor; flipping here (as Texture::fromFile does for
    // OpenGL-authored UVs) puts the beak and the eyes on the wrong side.
    stbi_set_flip_vertically_on_load(0);
    int width = 0;
    int height = 0;
    int channels = 0;
    // Four channels: RGB widths that are not a multiple of four would
    // otherwise trip GL_UNPACK_ALIGNMENT.
    unsigned char* decoded = stbi_load_from_memory(
        reinterpret_cast<unsigned char const*>(p_pixels.data()),
        static_cast<int>(p_pixels.size()),
        &width,
        &height,
        &channels,
        4);
    if (decoded == nullptr)
    {
        const char* why = stbi_failure_reason();
        return failure(std::string("embedded glTF image: ") +
                       ((why == nullptr) ? "unknown reason" : why));
    }

    const compages::gpu::PixelFormat format =
        p_srgb ? compages::gpu::PixelFormat::SRGB8A8
               : compages::gpu::PixelFormat::RGBA8;

    compages::gpu::TextureDesc desc;
    desc.kind = compages::gpu::TextureKind::Texture2D;
    desc.format = format;
    desc.width = static_cast<std::uint32_t>(width);
    desc.height = static_cast<std::uint32_t>(height);
    desc.levels = 0u;
    desc.wrap_x = compages::gpu::Wrap::Repeat;
    desc.wrap_y = compages::gpu::Wrap::Repeat;

    const std::size_t bytes = static_cast<std::size_t>(width) *
                              static_cast<std::size_t>(height) *
                              compages::gpu::bytesPerPixel(format);
    TextureAsset result;
    result.description = desc;
    result.pixels.assign(reinterpret_cast<const std::byte*>(decoded),
                         reinterpret_cast<const std::byte*>(decoded) + bytes);
    stbi_image_free(decoded);
    if (compages::gpu::initialized())
    {
        COMPAGES_TRY(result.upload());
    }
    return result;
}

[[nodiscard]] Result<TextureAssetId> importImage(cgltf_image* p_image,
                                                 AssetManager& p_assets,
                                                 TextureMap& p_cache,
                                                 std::size_t& p_texture_count)
{
    if (p_image == nullptr)
    {
        return failure("glTF image pointer is null");
    }
    const auto cached = p_cache.find(p_image);
    if (cached != p_cache.end())
    {
        return cached->second;
    }

    std::span<const std::byte> bytes;
    if ((p_image->buffer_view != nullptr) &&
        (p_image->buffer_view->buffer != nullptr))
    {
        const cgltf_buffer_view* view = p_image->buffer_view;
        bytes = std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(view->buffer->data) +
                view->offset,
            view->size);
    }
    else if (p_image->uri != nullptr)
    {
        return failure(
            "external glTF image URIs are not supported yet; embed images "
            "in the GLB");
    }
    else
    {
        return failure("glTF image has neither a buffer view nor a URI");
    }

    auto texture_result = textureFromMemory(bytes, true);
    if (!texture_result)
    {
        return failure(texture_result.error());
    }
    auto texture = texture_result.take();
    texture.name = (p_image->name != nullptr) ? p_image->name : "gltf-image";
    auto id_result = p_assets.addTexture(texture.name, std::move(texture));
    if (!id_result)
    {
        return failure(id_result.error());
    }
    auto id = id_result.take();
    p_cache.emplace(p_image, id);
    ++p_texture_count;
    return id;
}

// --- Mesh primitives ---------------------------------------------------------
// Unpack POSITION/NORMAL/TEXCOORD/JOINTS/WEIGHTS into MeshAsset (+ optional
// skin).

[[nodiscard]] cgltf_accessor const*
findAttribute(cgltf_primitive const* p_primitive,
              cgltf_attribute_type p_type,
              cgltf_int p_index = 0)
{
    for (std::size_t i = 0u; i < p_primitive->attributes_count; ++i)
    {
        if ((p_primitive->attributes[i].type == p_type) &&
            (p_primitive->attributes[i].index == p_index))
        {
            return p_primitive->attributes[i].data;
        }
    }
    return nullptr;
}

[[nodiscard]] Result<MeshAsset>
meshFromPrimitive(cgltf_primitive const* p_primitive)
{
    cgltf_accessor const* positions =
        findAttribute(p_primitive, cgltf_attribute_type_position);
    if (positions == nullptr)
    {
        return failure("glTF primitive has no POSITION attribute");
    }
    if (positions->count > 65535u)
    {
        return failure("glTF primitive exceeds the 65535 vertex limit of "
                       "the current MeshAsset");
    }

    cgltf_accessor const* normals =
        findAttribute(p_primitive, cgltf_attribute_type_normal);
    cgltf_accessor const* uvs =
        findAttribute(p_primitive, cgltf_attribute_type_texcoord);
    cgltf_accessor const* joints =
        findAttribute(p_primitive, cgltf_attribute_type_joints);
    cgltf_accessor const* weights =
        findAttribute(p_primitive, cgltf_attribute_type_weights);

    std::vector<float> position_data(positions->count * 3u);
    if (cgltf_accessor_unpack_floats(
            positions, position_data.data(), position_data.size()) == 0)
    {
        return failure("failed to read glTF POSITION data");
    }

    std::vector<float> normal_data;
    if (normals != nullptr)
    {
        normal_data.resize(normals->count * 3u);
        if (cgltf_accessor_unpack_floats(
                normals, normal_data.data(), normal_data.size()) == 0)
        {
            return failure("failed to read glTF NORMAL data");
        }
    }

    std::vector<float> uv_data;
    if (uvs != nullptr)
    {
        uv_data.resize(uvs->count * 2u);
        if (cgltf_accessor_unpack_floats(uvs, uv_data.data(), uv_data.size()) ==
            0)
        {
            return failure("failed to read glTF TEXCOORD data");
        }
    }

    std::vector<float> weight_data;
    if (weights != nullptr)
    {
        weight_data.resize(weights->count * 4u);
        if (cgltf_accessor_unpack_floats(
                weights, weight_data.data(), weight_data.size()) == 0)
        {
            return failure("failed to read glTF WEIGHTS data");
        }
    }

    const bool skinned = (joints != nullptr) && (weights != nullptr);

    std::vector<MeshVertex> vertices;
    vertices.reserve(positions->count);
    compages::core::AABB bounds;
    for (std::size_t i = 0u; i < positions->count; ++i)
    {
        MeshVertex vertex;
        vertex.position =
            compages::core::Vector3f(position_data[(i * 3u) + 0u],
                                     position_data[(i * 3u) + 1u],
                                     position_data[(i * 3u) + 2u]);
        if (!normal_data.empty())
        {
            vertex.normal =
                compages::core::Vector3f(normal_data[(i * 3u) + 0u],
                                         normal_data[(i * 3u) + 1u],
                                         normal_data[(i * 3u) + 2u]);
        }
        else
        {
            vertex.normal = compages::core::vector::normalize(vertex.position);
        }
        if (!uv_data.empty())
        {
            vertex.uv = compages::core::Vector2f(uv_data[(i * 2u) + 0u],
                                                 uv_data[(i * 2u) + 1u]);
        }
        bounds.expand(vertex.position);
        vertices.emplace_back(vertex);
    }

    std::vector<std::uint16_t> joint_indices;
    std::vector<compages::core::Vector4f> joint_weights;
    if (skinned)
    {
        joint_indices.resize(vertices.size() * 4u);
        joint_weights.resize(vertices.size());
        for (std::size_t i = 0u; i < vertices.size(); ++i)
        {
            cgltf_uint raw[4] = { 0u, 0u, 0u, 0u };
            if (cgltf_accessor_read_uint(joints, i, raw, 4u) == 0)
            {
                return failure("failed to read glTF JOINTS data");
            }
            joint_indices[(i * 4u) + 0u] = static_cast<std::uint16_t>(raw[0]);
            joint_indices[(i * 4u) + 1u] = static_cast<std::uint16_t>(raw[1]);
            joint_indices[(i * 4u) + 2u] = static_cast<std::uint16_t>(raw[2]);
            joint_indices[(i * 4u) + 3u] = static_cast<std::uint16_t>(raw[3]);
            joint_weights[i] =
                compages::core::Vector4f(weight_data[(i * 4u) + 0u],
                                         weight_data[(i * 4u) + 1u],
                                         weight_data[(i * 4u) + 2u],
                                         weight_data[(i * 4u) + 3u]);
            vertices[i].joints =
                compages::core::Vector4i(static_cast<int>(raw[0]),
                                         static_cast<int>(raw[1]),
                                         static_cast<int>(raw[2]),
                                         static_cast<int>(raw[3]));
            vertices[i].weights = joint_weights[i];
        }
    }

    std::vector<std::uint32_t> indices;
    if (p_primitive->indices != nullptr)
    {
        indices.resize(p_primitive->indices->count);
        for (std::size_t i = 0u; i < p_primitive->indices->count; ++i)
        {
            indices[i] = static_cast<std::uint32_t>(
                cgltf_accessor_read_index(p_primitive->indices, i));
        }
    }
    else
    {
        indices.reserve(vertices.size());
        for (std::size_t i = 0u; i < vertices.size(); ++i)
        {
            indices.emplace_back(static_cast<std::uint32_t>(i));
        }
    }

    MeshAsset mesh;
    mesh.source_vertices = vertices;
    mesh.source_indices = indices;
    mesh.index_count = indices.size();
    mesh.local_bounds = bounds;
    if (skinned)
    {
        mesh.rest_pose = std::move(vertices);
        mesh.joint_indices = std::move(joint_indices);
        mesh.joint_weights = std::move(joint_weights);
    }
    if (compages::gpu::initialized())
    {
        COMPAGES_TRY(mesh.upload());
    }
    return mesh;
}

// --- Node transform ----------------------------------------------------------
// Map glTF matrix or TRS into compages::world::LocalTransform / PrefabNode
// fields.

template <typename Transform>
void applyNodeTransform(cgltf_node const* p_node, Transform&& p_local)
{
    if (p_node->has_matrix)
    {
        // glTF matrices are column-major, column-vector: translation sits at
        // indices 12, 13, 14 and the first three columns are the scaled basis.
        // compages::core::Quatf::fromMatrix() reads a mathematical
        // (column-vector) rotation.
        p_local.position = compages::core::Vector3f(
            p_node->matrix[12], p_node->matrix[13], p_node->matrix[14]);

        const compages::core::Vector3f axis_x(
            p_node->matrix[0], p_node->matrix[1], p_node->matrix[2]);
        const compages::core::Vector3f axis_y(
            p_node->matrix[4], p_node->matrix[5], p_node->matrix[6]);
        const compages::core::Vector3f axis_z(
            p_node->matrix[8], p_node->matrix[9], p_node->matrix[10]);
        const float scale_x = compages::core::vector::norm(axis_x);
        const float scale_y = compages::core::vector::norm(axis_y);
        const float scale_z = compages::core::vector::norm(axis_z);
        p_local.scale = compages::core::Vector3f(scale_x, scale_y, scale_z);

        compages::core::Matrix44f rotation_matrix(
            compages::core::matrix::Identity);
        if (scale_x > 1.0e-8f)
        {
            const compages::core::Vector3f n = axis_x / scale_x;
            rotation_matrix(0, 0) = n.x;
            rotation_matrix(1, 0) = n.y;
            rotation_matrix(2, 0) = n.z;
        }
        if (scale_y > 1.0e-8f)
        {
            const compages::core::Vector3f n = axis_y / scale_y;
            rotation_matrix(0, 1) = n.x;
            rotation_matrix(1, 1) = n.y;
            rotation_matrix(2, 1) = n.z;
        }
        if (scale_z > 1.0e-8f)
        {
            const compages::core::Vector3f n = axis_z / scale_z;
            rotation_matrix(0, 2) = n.x;
            rotation_matrix(1, 2) = n.y;
            rotation_matrix(2, 2) = n.z;
        }
        p_local.rotation = compages::core::Quatf::fromMatrix(rotation_matrix);
        return;
    }

    if (p_node->has_translation)
    {
        p_local.position = compages::core::Vector3f(p_node->translation[0],
                                                    p_node->translation[1],
                                                    p_node->translation[2]);
    }
    if (p_node->has_rotation)
    {
        p_local.rotation = compages::core::Quatf(p_node->rotation[3],
                                                 p_node->rotation[0],
                                                 p_node->rotation[1],
                                                 p_node->rotation[2]);
    }
    if (p_node->has_scale)
    {
        p_local.scale = compages::core::Vector3f(
            p_node->scale[0], p_node->scale[1], p_node->scale[2]);
    }
}

// --- Materials ---------------------------------------------------------------
// PBR base colour factor + embedded base colour texture, shared MaterialId.

[[nodiscard]] Result<MaterialInstanceId>
materialInstanceFromGltf(cgltf_material const* p_material,
                         MaterialId p_shared_material,
                         AssetManager& p_assets,
                         TextureMap& p_textures,
                         std::size_t& p_texture_count)
{
    MaterialInstance instance;
    instance.material = p_shared_material;
    if ((p_material != nullptr) && p_material->has_pbr_metallic_roughness)
    {
        const cgltf_pbr_metallic_roughness& pbr =
            p_material->pbr_metallic_roughness;
        instance.base_color_factor =
            compages::core::Vector3f(pbr.base_color_factor[0],
                                     pbr.base_color_factor[1],
                                     pbr.base_color_factor[2]);
        if (pbr.base_color_texture.texture != nullptr)
        {
            auto texture_id_result =
                importImage(pbr.base_color_texture.texture->image,
                            p_assets,
                            p_textures,
                            p_texture_count);
            if (!texture_id_result)
            {
                return failure(texture_id_result.error());
            }
            auto texture_id = texture_id_result.take();
            instance.base_color_texture = texture_id;
        }
    }

    const std::string name =
        (p_material != nullptr && p_material->name != nullptr)
            ? p_material->name
            : "gltf-material";
    auto id_result = p_assets.addMaterialInstance(name, instance);
    if (!id_result)
    {
        return failure(id_result.error());
    }
    auto id = id_result.take();
    return id;
}

[[nodiscard]] std::size_t nodeIndex(cgltf_data const* p_data,
                                    cgltf_node const* p_node)
{
    return static_cast<std::size_t>(p_node - p_data->nodes);
}

// --- Live world import (legacy) ----------------------------------------------
// Recursively spawn entities, first primitive only per mesh node.

[[nodiscard]] Status importNode(cgltf_node* p_node,
                                cgltf_data* p_data,
                                AssetManager& p_assets,
                                compages::world::World& p_world,
                                compages::world::EntityId p_parent,
                                MaterialId p_shared_material,
                                TextureMap& p_textures,
                                std::vector<compages::world::EntityId>& p_nodes,
                                GltfImport& p_result)
{
    const std::string node_name =
        (p_node->name != nullptr) ? p_node->name : "gltf-node";
    compages::world::EntityId entity = p_world.create(node_name);
    p_nodes[nodeIndex(p_data, p_node)] = entity;
    ++p_result.node_count;
    if (p_parent.valid())
    {
        COMPAGES_TRY(p_world.setParent(entity, p_parent));
    }

    applyNodeTransform(p_node, p_world.transform(entity));

    if (p_node->mesh != nullptr)
    {
        for (std::size_t i = 0u; i < p_node->mesh->primitives_count; ++i)
        {
            auto mesh_result = meshFromPrimitive(&p_node->mesh->primitives[i]);
            if (!mesh_result)
            {
                return failure(mesh_result.error());
            }
            auto mesh = mesh_result.take();
            auto mesh_id_result = p_assets.addMesh(
                node_name + "-mesh-" + std::to_string(i), std::move(mesh));
            if (!mesh_id_result)
            {
                return failure(mesh_id_result.error());
            }
            auto mesh_id = mesh_id_result.take();
            ++p_result.mesh_count;

            auto instance_id_result =
                materialInstanceFromGltf(p_node->mesh->primitives[i].material,
                                         p_shared_material,
                                         p_assets,
                                         p_textures,
                                         p_result.texture_count);
            if (!instance_id_result)
            {
                return failure(instance_id_result.error());
            }
            auto instance_id = instance_id_result.take();

            compages::renderer::MeshRenderer renderer;
            renderer.mesh = mesh_id;
            renderer.material_instance = instance_id;
            p_world.add(entity, renderer);
            break;
        }
    }

    for (std::size_t i = 0u; i < p_node->children_count; ++i)
    {
        COMPAGES_TRY(importNode(p_node->children[i],
                                p_data,
                                p_assets,
                                p_world,
                                entity,
                                p_shared_material,
                                p_textures,
                                p_nodes,
                                p_result));
    }
    return success();
}

// --- Skins -------------------------------------------------------------------
// Inverse bind matrices and joint entity wiring after the node tree exists.

[[nodiscard]] Result<SkinAssetId>
importSkinAsset(cgltf_skin* p_skin, AssetManager& p_assets, std::size_t p_index)
{
    SkinAsset asset;
    if (p_skin->inverse_bind_matrices != nullptr)
    {
        asset.inverse_bind.resize(p_skin->joints_count);
        std::vector<float> raw(p_skin->joints_count * 16u);
        if (cgltf_accessor_unpack_floats(
                p_skin->inverse_bind_matrices, raw.data(), raw.size()) == 0)
        {
            return failure("failed to read glTF inverse bind matrices");
        }
        for (std::size_t i = 0u; i < p_skin->joints_count; ++i)
        {
            // glTF is column-major, Matrix is row-major.
            compages::core::Matrix44f column_major;
            std::memcpy(column_major.data(),
                        raw.data() + (i * 16u),
                        sizeof(float) * 16u);
            asset.inverse_bind[i] = compages::core::transpose(column_major);
        }
    }
    else
    {
        asset.inverse_bind.assign(
            p_skin->joints_count,
            compages::core::Matrix44f(compages::core::matrix::Identity));
    }
    const std::string name = (p_skin->name != nullptr)
                                 ? p_skin->name
                                 : ("gltf-skin-" + std::to_string(p_index));
    auto id_result = p_assets.addSkin(name, std::move(asset));
    if (!id_result)
    {
        return failure(id_result.error());
    }
    auto id = id_result.take();
    return id;
}

[[nodiscard]] Status
bindSkins(cgltf_data* p_data,
          AssetManager& p_assets,
          compages::world::World& p_world,
          std::vector<compages::world::EntityId> const& p_nodes,
          GltfImport& p_result)
{
    std::unordered_map<cgltf_skin*, SkinAssetId> skins;
    for (std::size_t i = 0u; i < p_data->nodes_count; ++i)
    {
        cgltf_node* node = &p_data->nodes[i];
        if ((node->skin == nullptr) || (node->mesh == nullptr))
        {
            continue;
        }
        const compages::world::EntityId entity = p_nodes[i];
        if (!entity.valid() ||
            !p_world.has<compages::renderer::MeshRenderer>(entity))
        {
            continue;
        }
        SkinAssetId skin_id;
        const auto cached = skins.find(node->skin);
        if (cached != skins.end())
        {
            skin_id = cached->second;
        }
        else
        {
            const std::size_t skin_index =
                static_cast<std::size_t>(node->skin - p_data->skins);
            auto created_result =
                importSkinAsset(node->skin, p_assets, skin_index);
            if (!created_result)
            {
                return failure(created_result.error());
            }
            auto created = created_result.take();
            skin_id = created;
            skins.emplace(node->skin, skin_id);
            ++p_result.skin_count;
        }

        compages::renderer::MeshRenderer const& renderer =
            p_world.get<compages::renderer::MeshRenderer>(entity);
        compages::renderer::MeshAsset* mesh = p_assets.mesh(renderer.mesh);
        if (mesh != nullptr)
        {
            mesh->skin = skin_id;
        }

        compages::world::SkinInstance instance;
        instance.joints.resize(node->skin->joints_count);
        for (std::size_t j = 0u; j < node->skin->joints_count; ++j)
        {
            if (node->skin->joints[j] != nullptr)
            {
                instance.joints[j] =
                    p_nodes[nodeIndex(p_data, node->skin->joints[j])];
            }
        }
        p_world.add(entity, std::move(instance));
    }
    return success();
}

// --- Animation clips ---------------------------------------------------------
// glTF samplers become compages::renderer::AnimationClip assets keyed by node
// index.

[[nodiscard]] Status importAnimations(cgltf_data* p_data,
                                      AssetManager& p_assets,
                                      std::string const& p_path,
                                      std::vector<AnimationClipId>& p_result)
{
    for (std::size_t i = 0u; i < p_data->animations_count; ++i)
    {
        cgltf_animation const& source = p_data->animations[i];
        AnimationClip clip;
        clip.name = (source.name != nullptr) ? source.name
                                             : ("clip-" + std::to_string(i));
        clip.channels.reserve(source.channels_count);
        for (std::size_t c = 0u; c < source.channels_count; ++c)
        {
            cgltf_animation_channel const& channel = source.channels[c];
            if ((channel.target_node == nullptr) ||
                (channel.sampler == nullptr) ||
                (channel.sampler->input == nullptr) ||
                (channel.sampler->output == nullptr))
            {
                continue;
            }
            if ((channel.target_path !=
                 cgltf_animation_path_type_translation) &&
                (channel.target_path != cgltf_animation_path_type_rotation) &&
                (channel.target_path != cgltf_animation_path_type_scale))
            {
                continue;
            }

            AnimationChannel curve;
            const std::size_t target = nodeIndex(p_data, channel.target_node);
            if (target >= p_data->nodes_count)
            {
                continue;
            }
            curve.target = static_cast<std::uint32_t>(target);
            if (channel.target_path == cgltf_animation_path_type_translation)
            {
                curve.path = AnimationChannel::Path::Translation;
            }
            else if (channel.target_path == cgltf_animation_path_type_rotation)
            {
                curve.path = AnimationChannel::Path::Rotation;
            }
            else
            {
                curve.path = AnimationChannel::Path::Scale;
            }
            curve.interpolation = (channel.sampler->interpolation ==
                                   cgltf_interpolation_type_step)
                                      ? AnimationChannel::Interpolation::Step
                                      : AnimationChannel::Interpolation::Linear;

            curve.times.resize(channel.sampler->input->count);
            if (cgltf_accessor_unpack_floats(channel.sampler->input,
                                             curve.times.data(),
                                             curve.times.size()) == 0)
            {
                return failure("failed to read glTF animation times");
            }
            const std::size_t components =
                (curve.path == AnimationChannel::Path::Rotation) ? 4u : 3u;
            curve.values.resize(channel.sampler->output->count * components);
            if (cgltf_accessor_unpack_floats(channel.sampler->output,
                                             curve.values.data(),
                                             curve.values.size()) == 0)
            {
                return failure("failed to read glTF animation values");
            }
            if (!curve.times.empty())
            {
                clip.duration = std::max(clip.duration, curve.times.back());
            }
            clip.channels.emplace_back(std::move(curve));
        }
        const std::string asset_name = p_path + "#" + clip.name;
        auto id_result = p_assets.addAnimation(asset_name, std::move(clip));
        if (!id_result)
        {
            return failure(id_result.error());
        }
        auto id = id_result.take();
        p_result.emplace_back(id);
    }
    return success();
}

// --- Prefab tree -------------------------------------------------------------
// Headless path: one PrefabNode per glTF node; extra primitives become
// children.

[[nodiscard]] Result<PrefabNode>
loadPrefabNode(cgltf_node* p_node,
               cgltf_data* p_data,
               AssetManager& p_assets,
               MaterialId p_shared_material,
               TextureMap& p_textures,
               std::size_t& p_texture_count,
               std::unordered_map<cgltf_skin*, SkinAssetId>& p_skins,
               std::string const& p_asset_name)
{
    PrefabNode result;
    result.source_index = static_cast<std::uint32_t>(nodeIndex(p_data, p_node));
    result.name = (p_node->name != nullptr) ? p_node->name : "gltf-node";
    applyNodeTransform(p_node, result.transform);

    // One renderer per entity: the first primitive of the mesh is drawn by
    // the node itself, each other one by a child of it, at the same place,
    // with the same skin. A mesh made of several materials is several
    // primitives, and loses parts of itself otherwise.
    std::vector<PrefabNode> extra_primitives;
    const std::size_t primitives =
        (p_node->mesh != nullptr) ? p_node->mesh->primitives_count : 0u;
    for (std::size_t p = 0u; p < primitives; ++p)
    {
        cgltf_primitive* primitive = &p_node->mesh->primitives[p];
        PrefabNode extra;
        PrefabNode& target = (p == 0u) ? result : extra;
        if (p != 0u)
        {
            // Past every index of the file, distinct for each node and
            // primitive: what skins and clips name are never these.
            extra.source_index = static_cast<std::uint32_t>(
                p_data->nodes_count + 1u + (result.source_index * 16u) + p);
            extra.name = result.name + "#" + std::to_string(p);
        }
        auto mesh_result = meshFromPrimitive(primitive);
        if (!mesh_result)
        {
            return failure(mesh_result.error());
        }
        auto mesh = mesh_result.take();
        const std::string mesh_name =
            p_asset_name + "#node-" + std::to_string(result.source_index) +
            "-mesh" + ((p == 0u) ? std::string() : ("-" + std::to_string(p)));
        auto mesh_id_result = p_assets.addMesh(mesh_name, std::move(mesh));
        if (!mesh_id_result)
        {
            return failure(mesh_id_result.error());
        }
        auto mesh_id = mesh_id_result.take();
        auto material_id_result = materialInstanceFromGltf(primitive->material,
                                                           p_shared_material,
                                                           p_assets,
                                                           p_textures,
                                                           p_texture_count);
        if (!material_id_result)
        {
            return failure(material_id_result.error());
        }
        auto material_id = material_id_result.take();

        target.mesh_renderer =
            PrefabMeshRenderer{ p_assets.meshName(mesh_id),
                                p_assets.materialInstanceName(material_id),
                                compages::renderer::RenderFlags::None };

        if (p_node->skin != nullptr)
        {
            SkinAssetId skin_id;
            auto const cached = p_skins.find(p_node->skin);
            if (cached != p_skins.end())
            {
                skin_id = cached->second;
            }
            else
            {
                const std::size_t skin_index =
                    static_cast<std::size_t>(p_node->skin - p_data->skins);
                auto created_result =
                    importSkinAsset(p_node->skin, p_assets, skin_index);
                if (!created_result)
                {
                    return failure(created_result.error());
                }
                auto created = created_result.take();
                skin_id = created;
                p_skins.emplace(p_node->skin, skin_id);
            }
            if (MeshAsset* stored = p_assets.mesh(mesh_id); stored != nullptr)
            {
                stored->skin = skin_id;
            }

            PrefabSkinInstance skin;
            skin.joints.reserve(p_node->skin->joints_count);
            for (std::size_t i = 0u; i < p_node->skin->joints_count; ++i)
            {
                skin.joints.emplace_back(static_cast<std::uint32_t>(
                    nodeIndex(p_data, p_node->skin->joints[i])));
            }
            target.skin_instance = std::move(skin);
        }
        if (p != 0u)
        {
            extra_primitives.emplace_back(std::move(extra));
        }
    }

    result.children.reserve(p_node->children_count);
    for (std::size_t i = 0u; i < p_node->children_count; ++i)
    {
        auto child_result = loadPrefabNode(p_node->children[i],
                                           p_data,
                                           p_assets,
                                           p_shared_material,
                                           p_textures,
                                           p_texture_count,
                                           p_skins,
                                           p_asset_name);
        if (!child_result)
        {
            return failure(child_result.error());
        }
        auto child = child_result.take();
        result.children.emplace_back(std::move(child));
    }
    for (PrefabNode& extra : extra_primitives)
    {
        result.children.emplace_back(std::move(extra));
    }
    return result;
}

} // namespace

// --- Public: reusable prefab -------------------------------------------------
// Parse file, build Prefab + animations; no compages::world::World entities.

Result<PrefabId> loadGltf(std::string const& p_path,
                          AssetManager& p_assets,
                          MaterialId p_shared_material)
{
    cgltf_options options{};
    cgltf_data* raw = nullptr;
    if (cgltf_parse_file(&options, p_path.c_str(), &raw) !=
        cgltf_result_success)
    {
        return failure("cgltf_parse_file failed for '" + p_path + "'");
    }
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(raw, &cgltf_free);
    if (cgltf_load_buffers(&options, data.get(), p_path.c_str()) !=
        cgltf_result_success)
    {
        return failure("cgltf_load_buffers failed for '" + p_path + "'");
    }
    if ((data->scenes_count == 0u) || (data->scene == nullptr))
    {
        return failure("glTF file has no default scene");
    }

    MaterialId shared_material = p_shared_material;
    if (!shared_material.valid())
    {
        Material material;
        if (compages::gpu::initialized())
        {
            COMPAGES_TRY_ASSIGN(material, makePbrMaterial());
        }
        else
        {
            material.name = "gltf-pbr";
            material.family = ShaderFamily::PbrMinimal;
        }
        auto created_result =
            p_assets.addMaterial("gltf-pbr", std::move(material));
        if (!created_result)
        {
            return failure(created_result.error());
        }
        auto created = created_result.take();
        shared_material = created;
    }

    Prefab prefab;
    prefab.name = p_path;
    prefab.root.name = "gltf-root";
    prefab.root.source_index = static_cast<std::uint32_t>(data->nodes_count);

    TextureMap textures;
    std::size_t texture_count = 0u;
    std::unordered_map<cgltf_skin*, SkinAssetId> skins;
    prefab.root.children.reserve(data->scene->nodes_count);
    for (std::size_t i = 0u; i < data->scene->nodes_count; ++i)
    {
        auto child_result = loadPrefabNode(data->scene->nodes[i],
                                           data.get(),
                                           p_assets,
                                           shared_material,
                                           textures,
                                           texture_count,
                                           skins,
                                           p_path);
        if (!child_result)
        {
            return failure(child_result.error());
        }
        auto child = child_result.take();
        prefab.root.children.emplace_back(std::move(child));
    }
    COMPAGES_TRY(
        importAnimations(data.get(), p_assets, p_path, prefab.animations));

    return p_assets.addPrefab(p_path, std::move(prefab));
}

// --- Public: immediate world instance ----------------------------------------
// Same assets as loadGltf but attaches
// compages::renderer::MeshRenderer/Animator under p_parent.

Result<GltfImport> importGltf(std::string const& p_path,
                              AssetManager& p_assets,
                              compages::world::World& p_world,
                              compages::world::EntityId p_parent,
                              MaterialId p_shared_material)
{
    cgltf_options options{};
    cgltf_data* data = nullptr;
    cgltf_result parse_result =
        cgltf_parse_file(&options, p_path.c_str(), &data);
    if (parse_result != cgltf_result_success)
    {
        return failure("cgltf_parse_file failed for '" + p_path + "'");
    }

    if (cgltf_load_buffers(&options, data, p_path.c_str()) !=
        cgltf_result_success)
    {
        cgltf_free(data);
        return failure("cgltf_load_buffers failed for '" + p_path + "'");
    }

    MaterialId shared_material = p_shared_material;
    if (!shared_material.valid())
    {
        auto material_result = makePbrMaterial();
        if (!material_result)
        {
            return failure(material_result.error());
        }
        auto material = material_result.take();
        // COMPAGES_TRY_VALUE always declares a new name; assigning back into
        // the outer id is required or every instance points at an empty
        // Material.
        auto created_result =
            p_assets.addMaterial("gltf-pbr", std::move(material));
        if (!created_result)
        {
            return failure(created_result.error());
        }
        auto created = created_result.take();
        shared_material = created;
    }

    GltfImport result;
    TextureMap textures;

    if ((data->scenes_count == 0u) || (data->scene == nullptr))
    {
        cgltf_free(data);
        return failure("glTF file has no default scene");
    }

    // Identity wrapper so the caller can frame the model without overwriting
    // the file's node scale (Duck.glb stores centimetres and a 0.01 root).
    result.root = p_world.create("gltf-root");
    if (p_parent.valid())
    {
        COMPAGES_TRY(p_world.setParent(result.root, p_parent));
    }

    std::vector<compages::world::EntityId> nodes(data->nodes_count);
    cgltf_scene* scene = data->scene;
    for (std::size_t i = 0u; i < scene->nodes_count; ++i)
    {
        COMPAGES_TRY(importNode(scene->nodes[i],
                                data,
                                p_assets,
                                p_world,
                                result.root,
                                shared_material,
                                textures,
                                nodes,
                                result));
    }

    COMPAGES_TRY(bindSkins(data, p_assets, p_world, nodes, result));
    COMPAGES_TRY(importAnimations(data, p_assets, p_path, result.animations));
    if (!result.animations.empty())
    {
        compages::renderer::Animator animator;
        animator.clip = result.animations.front();
        animator.targets = nodes;
        for (AnimationClipId const id : result.animations)
        {
            AnimationClip const* clip = p_assets.animation(id);
            if ((clip != nullptr) && (clip->name == "Walk"))
            {
                animator.clip = id;
                break;
            }
        }
        p_world.add(result.root, animator);
        result.animator = result.root;
    }

    cgltf_free(data);
    p_world.update();
    return result;
}

} // namespace compages::renderer
