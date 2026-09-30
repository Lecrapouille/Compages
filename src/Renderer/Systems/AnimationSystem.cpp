// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Systems/AnimationSystem.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Components/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/Entity.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace compages::renderer
{

namespace
{

[[nodiscard]] compages::core::Quatf
slerp(compages::core::Quatf p_a, compages::core::Quatf p_b, float p_t)
{
    float dot =
        (p_a.a * p_b.a) + (p_a.b * p_b.b) + (p_a.c * p_b.c) + (p_a.d * p_b.d);
    if (dot < 0.0f)
    {
        p_b = compages::core::Quatf(-p_b.a, -p_b.b, -p_b.c, -p_b.d);
        dot = -dot;
    }
    if (dot > 0.9995f)
    {
        compages::core::Quatf result(p_a.a + (p_t * (p_b.a - p_a.a)),
                                     p_a.b + (p_t * (p_b.b - p_a.b)),
                                     p_a.c + (p_t * (p_b.c - p_a.c)),
                                     p_a.d + (p_t * (p_b.d - p_a.d)));
        result.normalize();
        return result;
    }
    const float theta = std::acos(std::min(dot, 1.0f));
    const float s = std::sin(theta);
    const float w0 = std::sin((1.0f - p_t) * theta) / s;
    const float w1 = std::sin(p_t * theta) / s;
    return compages::core::Quatf((w0 * p_a.a) + (w1 * p_b.a),
                                 (w0 * p_a.b) + (w1 * p_b.b),
                                 (w0 * p_a.c) + (w1 * p_b.c),
                                 (w0 * p_a.d) + (w1 * p_b.d));
}

void locateKey(std::vector<float> const& p_times,
               float p_time,
               std::size_t& p_left,
               std::size_t& p_right,
               float& p_blend)
{
    p_left = 0u;
    p_right = 0u;
    p_blend = 0.0f;
    if (p_times.empty())
    {
        return;
    }
    if ((p_times.size() == 1u) || (p_time <= p_times.front()))
    {
        return;
    }
    if (p_time >= p_times.back())
    {
        p_left = p_times.size() - 1u;
        p_right = p_left;
        return;
    }
    const auto it = std::upper_bound(p_times.begin(), p_times.end(), p_time);
    p_right = static_cast<std::size_t>(it - p_times.begin());
    p_left = p_right - 1u;
    const float span = p_times[p_right] - p_times[p_left];
    p_blend = (span > 1.0e-8f) ? ((p_time - p_times[p_left]) / span) : 0.0f;
}

void applyChannel(compages::renderer::AnimationChannel const& p_channel,
                  float p_time,
                  compages::world::LocalTransformView p_local)
{
    if (p_channel.times.empty() || p_channel.values.empty())
    {
        return;
    }

    std::size_t left = 0u;
    std::size_t right = 0u;
    float blend = 0.0f;
    locateKey(p_channel.times, p_time, left, right, blend);
    if (p_channel.interpolation ==
        compages::renderer::AnimationChannel::Interpolation::Step)
    {
        blend = 0.0f;
        right = left;
    }

    if (p_channel.path == compages::renderer::AnimationChannel::Path::Rotation)
    {
        const std::size_t i0 = left * 4u;
        const std::size_t i1 = right * 4u;
        if ((i0 + 3u >= p_channel.values.size()) ||
            (i1 + 3u >= p_channel.values.size()))
        {
            return;
        }
        // glTF stores xyzw; compages::core::Quatf is wxyz.
        const compages::core::Quatf a(p_channel.values[i0 + 3u],
                                      p_channel.values[i0 + 0u],
                                      p_channel.values[i0 + 1u],
                                      p_channel.values[i0 + 2u]);
        const compages::core::Quatf b(p_channel.values[i1 + 3u],
                                      p_channel.values[i1 + 0u],
                                      p_channel.values[i1 + 1u],
                                      p_channel.values[i1 + 2u]);
        p_local.rotation = slerp(a, b, blend);
        return;
    }

    const std::size_t i0 = left * 3u;
    const std::size_t i1 = right * 3u;
    if ((i0 + 2u >= p_channel.values.size()) ||
        (i1 + 2u >= p_channel.values.size()))
    {
        return;
    }
    const compages::core::Vector3f a(p_channel.values[i0 + 0u],
                                     p_channel.values[i0 + 1u],
                                     p_channel.values[i0 + 2u]);
    const compages::core::Vector3f b(p_channel.values[i1 + 0u],
                                     p_channel.values[i1 + 1u],
                                     p_channel.values[i1 + 2u]);
    const compages::core::Vector3f mixed = a + ((b - a) * blend);
    if (p_channel.path ==
        compages::renderer::AnimationChannel::Path::Translation)
    {
        p_local.position = mixed;
    }
    else
    {
        p_local.scale = mixed;
    }
}

[[nodiscard]] compages::core::Vector3f
transformAffinePoint(compages::core::Matrix44f const& p_matrix,
                     compages::core::Vector3f const& p_point)
{
    const compages::core::Vector4f out =
        p_matrix *
        compages::core::Vector4f(p_point.x, p_point.y, p_point.z, 1.0f);
    return compages::core::Vector3f(out.x, out.y, out.z);
}

[[nodiscard]] compages::core::Vector3f
transformVector(compages::core::Matrix44f const& p_matrix,
                compages::core::Vector3f const& p_vector)
{
    const compages::core::Vector4f out =
        p_matrix *
        compages::core::Vector4f(p_vector.x, p_vector.y, p_vector.z, 0.0f);
    return compages::core::Vector3f(out.x, out.y, out.z);
}

} // namespace

Status compages::renderer::AnimationSystem::sample(
    compages::world::World& p_world,
    compages::renderer::AssetManager const& p_assets,
    float p_dt)
{
    p_world.each<compages::renderer::Animator>(
        [&](compages::world::EntityId, compages::renderer::Animator& animator)
        {
            if (!animator.playing || !animator.clip.valid())
            {
                return;
            }
            compages::renderer::AnimationClip const* clip =
                p_assets.animation(animator.clip);
            if (clip == nullptr)
            {
                return;
            }
            animator.time += p_dt * animator.speed;
            if (clip->duration > 0.0f)
            {
                if (animator.loop)
                {
                    animator.time = std::fmod(animator.time, clip->duration);
                    if (animator.time < 0.0f)
                    {
                        animator.time += clip->duration;
                    }
                }
                else if (animator.time > clip->duration)
                {
                    animator.time = clip->duration;
                    animator.playing = false;
                }
            }
            for (compages::renderer::AnimationChannel const& channel :
                 clip->channels)
            {
                if (channel.target >= animator.targets.size())
                {
                    continue;
                }
                compages::world::EntityId const target =
                    animator.targets[channel.target];
                if (!p_world.alive(target))
                {
                    continue;
                }
                applyChannel(channel, animator.time, p_world.transform(target));
            }
        });
    return success();
}

Status compages::renderer::AnimationSystem::pose(
    compages::world::World& p_world,
    compages::renderer::AssetManager const& p_assets)
{
    p_world.each<compages::world::SkinInstance>(
        [&](compages::world::EntityId p_entity,
            compages::world::SkinInstance& p_instance)
        {
            compages::renderer::MeshRenderer const* renderer =
                p_world.tryGet<compages::renderer::MeshRenderer>(p_entity);
            if (renderer == nullptr)
            {
                return;
            }
            compages::renderer::MeshAsset const* mesh =
                p_assets.mesh(renderer->mesh);
            if ((mesh == nullptr) || !mesh->skin.valid())
            {
                return;
            }
            compages::renderer::SkinAsset const* skin_asset =
                p_assets.skin(mesh->skin);
            if (skin_asset == nullptr)
            {
                return;
            }

            const compages::core::Matrix44f mesh_world =
                p_world.worldMatrix(p_entity);
            const compages::core::Matrix44f inverse_mesh =
                compages::core::inverse(mesh_world);
            const std::size_t joint_count = skin_asset->inverse_bind.size();
            p_instance.pose.assign(
                joint_count,
                compages::core::Matrix44f(compages::core::matrix::Identity));
            for (std::size_t j = 0u; j < joint_count; ++j)
            {
                if (j >= p_instance.joints.size() ||
                    !p_world.alive(p_instance.joints[j]))
                {
                    continue;
                }
                p_instance.pose[j] = inverse_mesh *
                                     p_world.worldMatrix(p_instance.joints[j]) *
                                     skin_asset->inverse_bind[j];
            }
        });
    return success();
}

Status compages::renderer::AnimationSystem::skin(
    compages::world::World& p_world,
    compages::renderer::AssetManager& p_assets)
{
    COMPAGES_TRY(pose(p_world, p_assets));

    std::vector<compages::renderer::MeshVertex> posed;
    std::string write_error;

    p_world.each<compages::world::SkinInstance>(
        [&](compages::world::EntityId p_entity,
            compages::world::SkinInstance const& p_instance)
        {
            if (!write_error.empty())
            {
                return;
            }
            compages::renderer::MeshRenderer const* renderer =
                p_world.tryGet<compages::renderer::MeshRenderer>(p_entity);
            if (renderer == nullptr)
            {
                return;
            }
            compages::renderer::MeshAsset* mesh = p_assets.mesh(renderer->mesh);
            if ((mesh == nullptr) || mesh->rest_pose.empty() ||
                !mesh->skin.valid())
            {
                return;
            }
            compages::renderer::SkinAsset const* skin_asset =
                p_assets.skin(mesh->skin);
            if (skin_asset == nullptr)
            {
                return;
            }

            std::vector<compages::core::Matrix44f> const& joint_matrices =
                p_instance.pose;
            posed = mesh->rest_pose;
            compages::core::AABB bounds;
            for (std::size_t v = 0u; v < posed.size(); ++v)
            {
                const compages::core::Vector4f weights =
                    (v < mesh->joint_weights.size())
                        ? mesh->joint_weights[v]
                        : compages::core::Vector4f(1.0f, 0.0f, 0.0f, 0.0f);
                compages::core::Vector3f position(0.0f, 0.0f, 0.0f);
                compages::core::Vector3f normal(0.0f, 0.0f, 0.0f);
                float weight_sum = 0.0f;
                for (std::size_t k = 0u; k < 4u; ++k)
                {
                    const float weight = weights[k];
                    if (weight <= 0.0f)
                    {
                        continue;
                    }
                    const std::size_t joint =
                        (v * 4u + k) < mesh->joint_indices.size()
                            ? static_cast<std::size_t>(
                                  mesh->joint_indices[(v * 4u) + k])
                            : 0u;
                    if (joint >= joint_matrices.size())
                    {
                        continue;
                    }
                    compages::core::Matrix44f const& skin_matrix =
                        joint_matrices[joint];
                    position += transformAffinePoint(
                                    skin_matrix, mesh->rest_pose[v].position) *
                                weight;
                    normal += transformVector(skin_matrix,
                                              mesh->rest_pose[v].normal) *
                              weight;
                    weight_sum += weight;
                }
                if (weight_sum <= 0.0f)
                {
                    position = mesh->rest_pose[v].position;
                    normal = mesh->rest_pose[v].normal;
                }
                posed[v].position = position;
                const float nlen = compages::core::vector::norm(normal);
                posed[v].normal = (nlen > 1.0e-8f) ? (normal / nlen)
                                                   : mesh->rest_pose[v].normal;
                bounds.expand(posed[v].position);
            }
            auto written = compages::gpu::attempt(
                [&]
                {
                    mesh->vertices.write(
                        std::span<const compages::renderer::MeshVertex>(posed));
                });
            if (!written)
            {
                write_error = written.error();
                return;
            }
            if (!bounds.empty())
            {
                mesh->local_bounds = bounds;
            }
        });
    if (!write_error.empty())
    {
        return failure(std::move(write_error));
    }
    return success();
}

Status compages::renderer::AnimationSystem::tick(
    compages::world::World& p_world,
    compages::renderer::AssetManager& p_assets,
    float p_dt)
{
    COMPAGES_TRY(sample(p_world, p_assets, p_dt));
    p_world.update();
    return pose(p_world, p_assets);
}

} // namespace compages::renderer
