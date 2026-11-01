// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GltfConvert.hpp"

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


#include <stb_image.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace compages::renderer::gltf
{

namespace
{

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

} // namespace

// --- Mesh primitives ---------------------------------------------------------
// Unpack POSITION/NORMAL/TEXCOORD/JOINTS/WEIGHTS into MeshAsset (+ optional
// skin).

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

} // namespace compages::renderer::gltf
