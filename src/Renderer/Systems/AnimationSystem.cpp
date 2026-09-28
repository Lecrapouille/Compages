//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

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

using compages::world::EntityId;
using compages::world::LocalTransformView;
using compages::world::SkinInstance;
using compages::world::World;

namespace
{

[[nodiscard]] Quatf slerp(Quatf p_a, Quatf p_b, float p_t)
{
    float dot = (p_a.a * p_b.a) + (p_a.b * p_b.b) + (p_a.c * p_b.c) +
                (p_a.d * p_b.d);
    if (dot < 0.0f)
    {
        p_b = Quatf(-p_b.a, -p_b.b, -p_b.c, -p_b.d);
        dot = -dot;
    }
    if (dot > 0.9995f)
    {
        Quatf result(p_a.a + (p_t * (p_b.a - p_a.a)),
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
    return Quatf((w0 * p_a.a) + (w1 * p_b.a), (w0 * p_a.b) + (w1 * p_b.b),
                 (w0 * p_a.c) + (w1 * p_b.c), (w0 * p_a.d) + (w1 * p_b.d));
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
    const auto it =
        std::upper_bound(p_times.begin(), p_times.end(), p_time);
    p_right = static_cast<std::size_t>(it - p_times.begin());
    p_left = p_right - 1u;
    const float span = p_times[p_right] - p_times[p_left];
    p_blend = (span > 1.0e-8f) ? ((p_time - p_times[p_left]) / span) : 0.0f;
}

void applyChannel(compages::renderer::AnimationChannel const& p_channel,
                  float p_time,
                  LocalTransformView p_local)
{
    if (p_channel.times.empty() || p_channel.values.empty())
    {
        return;
    }

    std::size_t left = 0u;
    std::size_t right = 0u;
    float blend = 0.0f;
    locateKey(p_channel.times, p_time, left, right, blend);
    if (p_channel.interpolation == compages::renderer::AnimationChannel::Interpolation::Step)
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
        // glTF stores xyzw; Quatf is wxyz.
        const Quatf a(p_channel.values[i0 + 3u], p_channel.values[i0 + 0u],
                      p_channel.values[i0 + 1u], p_channel.values[i0 + 2u]);
        const Quatf b(p_channel.values[i1 + 3u], p_channel.values[i1 + 0u],
                      p_channel.values[i1 + 1u], p_channel.values[i1 + 2u]);
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
    const Vector3f a(p_channel.values[i0 + 0u], p_channel.values[i0 + 1u],
                     p_channel.values[i0 + 2u]);
    const Vector3f b(p_channel.values[i1 + 0u], p_channel.values[i1 + 1u],
                     p_channel.values[i1 + 2u]);
    const Vector3f mixed = a + ((b - a) * blend);
    if (p_channel.path == compages::renderer::AnimationChannel::Path::Translation)
    {
        p_local.position = mixed;
    }
    else
    {
        p_local.scale = mixed;
    }
}

[[nodiscard]] Vector3f transformPoint(Matrix44f const& p_matrix,
                                      Vector3f const& p_point)
{
    const Vector4f homogeneous(p_point.x, p_point.y, p_point.z, 1.0f);
    const Vector4f out = homogeneous * p_matrix;
    return Vector3f(out.x, out.y, out.z);
}

[[nodiscard]] Vector3f transformVector(Matrix44f const& p_matrix,
                                       Vector3f const& p_vector)
{
    return Vector3f((p_vector.x * p_matrix(0, 0)) +
                        (p_vector.y * p_matrix(1, 0)) +
                        (p_vector.z * p_matrix(2, 0)),
                    (p_vector.x * p_matrix(0, 1)) +
                        (p_vector.y * p_matrix(1, 1)) +
                        (p_vector.z * p_matrix(2, 1)),
                    (p_vector.x * p_matrix(0, 2)) +
                        (p_vector.y * p_matrix(1, 2)) +
                        (p_vector.z * p_matrix(2, 2)));
}

} // namespace

//------------------------------------------------------------------------------
compages::Status AnimationSystem::sample(World& p_world,
                                    compages::renderer::AssetManager const& p_assets,
                                    float p_dt)
{
    p_world.each<Animator>([&](EntityId, Animator& animator) {
        if (!animator.playing || !animator.clip.valid())
        {
            return;
        }
        compages::renderer::AnimationClip const* clip = p_assets.animation(animator.clip);
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
        for (compages::renderer::AnimationChannel const& channel : clip->channels)
        {
            if (channel.target >= animator.targets.size())
            {
                continue;
            }
            EntityId const target = animator.targets[channel.target];
            if (!p_world.alive(target))
            {
                continue;
            }
            applyChannel(channel, animator.time, p_world.transform(target));
        }
    });
    return compages::success();
}

//------------------------------------------------------------------------------
compages::Status AnimationSystem::pose(World& p_world,
                                   compages::renderer::AssetManager const& p_assets)
{
    p_world.each<SkinInstance>(
        [&](EntityId p_entity, SkinInstance& p_instance) {
        MeshRenderer const* renderer =
            p_world.tryGet<MeshRenderer>(p_entity);
        if (renderer == nullptr)
        {
            return;
        }
        compages::renderer::MeshAsset const* mesh = p_assets.mesh(renderer->mesh);
        if ((mesh == nullptr) || !mesh->skin.valid())
        {
            return;
        }
        compages::renderer::SkinAsset const* skin_asset = p_assets.skin(mesh->skin);
        if (skin_asset == nullptr)
        {
            return;
        }

        const Matrix44f mesh_world = p_world.worldMatrix(p_entity);
        const Matrix44f inverse_mesh = compages::matrix::inverse(mesh_world);
        const std::size_t joint_count = skin_asset->inverse_bind.size();
        p_instance.pose.assign(joint_count, Matrix44f(compages::matrix::Identity));
        for (std::size_t j = 0u; j < joint_count; ++j)
        {
            if (j >= p_instance.joints.size() ||
                !p_world.alive(p_instance.joints[j]))
            {
                continue;
            }
            p_instance.pose[j] =
                skin_asset->inverse_bind[j] *
                p_world.worldMatrix(p_instance.joints[j]) *
                inverse_mesh;
        }
    });
    return compages::success();
}

//------------------------------------------------------------------------------
compages::Status AnimationSystem::skin(World& p_world,
                                  compages::renderer::AssetManager& p_assets)
{
    COMPAGES_TRY(pose(p_world, p_assets));

    std::vector<compages::renderer::MeshVertex> posed;
    std::string write_error;

    p_world.each<SkinInstance>(
        [&](EntityId p_entity, SkinInstance const& p_instance) {
        if (!write_error.empty())
        {
            return;
        }
        MeshRenderer const* renderer =
            p_world.tryGet<MeshRenderer>(p_entity);
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
        compages::renderer::SkinAsset const* skin_asset = p_assets.skin(mesh->skin);
        if (skin_asset == nullptr)
        {
            return;
        }

        std::vector<Matrix44f> const& joint_matrices = p_instance.pose;
        posed = mesh->rest_pose;
        AABB bounds;
        for (std::size_t v = 0u; v < posed.size(); ++v)
        {
            const Vector4f weights = (v < mesh->joint_weights.size())
                                         ? mesh->joint_weights[v]
                                         : Vector4f(1.0f, 0.0f, 0.0f, 0.0f);
            Vector3f position(0.0f, 0.0f, 0.0f);
            Vector3f normal(0.0f, 0.0f, 0.0f);
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
                Matrix44f const& skin_matrix = joint_matrices[joint];
                position +=
                    transformPoint(skin_matrix, mesh->rest_pose[v].position) *
                    weight;
                normal +=
                    transformVector(skin_matrix, mesh->rest_pose[v].normal) *
                    weight;
                weight_sum += weight;
            }
            if (weight_sum <= 0.0f)
            {
                position = mesh->rest_pose[v].position;
                normal = mesh->rest_pose[v].normal;
            }
            posed[v].position = position;
            const float nlen = compages::vector::norm(normal);
            posed[v].normal =
                (nlen > 1.0e-8f) ? (normal / nlen) : mesh->rest_pose[v].normal;
            bounds.expand(posed[v].position);
        }
        auto written = compages::gpu::attempt([&] {
            mesh->vertices.write(std::span<const compages::renderer::MeshVertex>(posed));
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
        return compages::failure(std::move(write_error));
    }
    return compages::success();
}

//------------------------------------------------------------------------------
compages::Status AnimationSystem::tick(World& p_world,
                                  compages::renderer::AssetManager& p_assets,
                                  float p_dt)
{
    COMPAGES_TRY(sample(p_world, p_assets, p_dt));
    p_world.update();
    return pose(p_world, p_assets);
}

} // namespace compages::renderer
