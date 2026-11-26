// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Animation/AnimationSystem.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Animation/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/PoseDriver.hpp"
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

[[nodiscard]] compages::Quatf
slerp(compages::Quatf p_a, compages::Quatf p_b, float p_t)
{
    float dot =
        (p_a.a * p_b.a) + (p_a.b * p_b.b) + (p_a.c * p_b.c) + (p_a.d * p_b.d);
    if (dot < 0.0f)
    {
        p_b = compages::Quatf(-p_b.a, -p_b.b, -p_b.c, -p_b.d);
        dot = -dot;
    }
    if (dot > 0.9995f)
    {
        compages::Quatf result(p_a.a + (p_t * (p_b.a - p_a.a)),
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
    return compages::Quatf((w0 * p_a.a) + (w1 * p_b.a),
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

//! \brief Write the value of one curve at \c p_time onto a local transform.
//! With \c p_weight below 1 the value is mixed with what the transform holds
//! (the pose of the clip that fades out, written just before).
void applyChannel(compages::renderer::AnimationChannel const& p_channel,
                  float p_time,
                  float p_weight,
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
        // glTF stores xyzw; compages::Quatf is wxyz.
        const compages::Quatf a(p_channel.values[i0 + 3u],
                                      p_channel.values[i0 + 0u],
                                      p_channel.values[i0 + 1u],
                                      p_channel.values[i0 + 2u]);
        const compages::Quatf b(p_channel.values[i1 + 3u],
                                      p_channel.values[i1 + 0u],
                                      p_channel.values[i1 + 1u],
                                      p_channel.values[i1 + 2u]);
        const compages::Quatf value = slerp(a, b, blend);
        p_local.rotation =
            (p_weight >= 1.0f) ? value : slerp(p_local.rotation, value, p_weight);
        return;
    }

    const std::size_t i0 = left * 3u;
    const std::size_t i1 = right * 3u;
    if ((i0 + 2u >= p_channel.values.size()) ||
        (i1 + 2u >= p_channel.values.size()))
    {
        return;
    }
    const compages::Vector3f a(p_channel.values[i0 + 0u],
                                     p_channel.values[i0 + 1u],
                                     p_channel.values[i0 + 2u]);
    const compages::Vector3f b(p_channel.values[i1 + 0u],
                                     p_channel.values[i1 + 1u],
                                     p_channel.values[i1 + 2u]);
    const compages::Vector3f value = a + ((b - a) * blend);
    compages::Vector3f& out =
        (p_channel.path == compages::renderer::AnimationChannel::Path::Translation)
            ? p_local.position
            : p_local.scale;
    out = (p_weight >= 1.0f) ? value : (out + ((value - out) * p_weight));
}

[[nodiscard]] compages::Vector3f
transformAffinePoint(compages::Matrix44f const& p_matrix,
                     compages::Vector3f const& p_point)
{
    const compages::Vector4f out =
        p_matrix *
        compages::Vector4f(p_point.x, p_point.y, p_point.z, 1.0f);
    return compages::Vector3f(out.x, out.y, out.z);
}

[[nodiscard]] compages::Vector3f
transformVector(compages::Matrix44f const& p_matrix,
                compages::Vector3f const& p_vector)
{
    const compages::Vector4f out =
        p_matrix *
        compages::Vector4f(p_vector.x, p_vector.y, p_vector.z, 0.0f);
    return compages::Vector3f(out.x, out.y, out.z);
}

//! \brief Move \c p_time by \c p_step in a clip of \c p_duration: wrap
//! when it loops, else stop at the end.
//! \return false when a clip that does not loop reached its end.
bool advance(si::Seconds& p_time, si::Seconds p_step, si::Seconds p_duration, bool p_loop)
{
    p_time += p_step;
    if (p_duration <= si::Seconds(0.0))
    {
        return true;
    }
    if (p_loop)
    {
        p_time = units::math::fmod(p_time, p_duration);
        if (p_time < si::Seconds(0.0))
        {
            p_time += p_duration;
        }
        return true;
    }
    if (p_time > p_duration)
    {
        p_time = p_duration;
        return false;
    }
    return true;
}

//! \brief Write every curve of \c p_clip at \c p_time on the entities of
//! the animator, mixed by \c p_weight with what they hold.
void applyClip(compages::world::World& p_world,
               compages::renderer::Animator const& p_animator,
               compages::renderer::AnimationClip const& p_clip,
               si::Seconds p_time,
               float p_weight)
{
    for (compages::renderer::AnimationChannel const& channel : p_clip.channels)
    {
        if (channel.target >= p_animator.targets.size())
        {
            continue;
        }
        compages::world::EntityId const target = p_animator.targets[channel.target];
        if (!p_world.alive(target) ||
            !compages::world::mayWrite(
                p_world.tryGet<compages::world::PoseDriver>(target),
                compages::world::PoseDriver::Animation))
        {
            continue;
        }
        applyChannel(channel, compages::toSeconds(p_time), p_weight,
                     p_world.transform(target));
    }
}

} // namespace

Status compages::renderer::AnimationSystem::sample(
    compages::world::World& p_world,
    compages::renderer::AssetManager const& p_assets,
    si::Seconds p_dt)
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
            const si::Seconds step = p_dt * animator.speed;
            // The clip that fades out goes first, at full weight; the
            // current one is mixed over it.
            float weight = 1.0f;
            compages::renderer::AnimationClip const* previous =
                animator.previous.valid() ? p_assets.animation(animator.previous) : nullptr;
            if (previous != nullptr)
            {
                animator.faded += units::math::abs(step);
                if (animator.faded >= animator.fade)
                {
                    animator.previous = AnimationClipId{};
                    previous = nullptr;
                }
                else
                {
                    advance(animator.previous_time, step, previous->duration, animator.loop);
                    applyClip(p_world, animator, *previous, animator.previous_time, 1.0f);
                    weight = static_cast<float>(animator.faded / animator.fade);
                }
            }
            if (!advance(animator.time, step, clip->duration, animator.loop))
            {
                animator.playing = false;
            }
            applyClip(p_world, animator, *clip, animator.time, weight);
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

            const compages::Matrix44f mesh_world =
                p_world.worldMatrix(p_entity);
            const compages::Matrix44f inverse_mesh =
                compages::inverse(mesh_world);
            const std::size_t joint_count = skin_asset->inverse_bind.size();
            p_instance.pose.assign(
                joint_count,
                compages::Matrix44f(compages::matrix::Identity));
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

            std::vector<compages::Matrix44f> const& joint_matrices =
                p_instance.pose;
            posed = mesh->rest_pose;
            compages::AABB bounds;
            for (std::size_t v = 0u; v < posed.size(); ++v)
            {
                const compages::Vector4f weights =
                    (v < mesh->joint_weights.size())
                        ? mesh->joint_weights[v]
                        : compages::Vector4f(1.0f, 0.0f, 0.0f, 0.0f);
                compages::Vector3f position(0.0f, 0.0f, 0.0f);
                compages::Vector3f normal(0.0f, 0.0f, 0.0f);
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
                    compages::Matrix44f const& skin_matrix =
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
                const float nlen = compages::vector::norm(normal);
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
    si::Seconds p_dt)
{
    COMPAGES_TRY(sample(p_world, p_assets, p_dt));
    p_world.update();
    return pose(p_world, p_assets);
}

} // namespace compages::renderer

bool compages::renderer::AnimationSystem::play(
    compages::world::World& p_world,
    compages::renderer::AssetManager const& p_assets,
    compages::world::EntityId p_model,
    std::string_view p_clip,
    si::Seconds p_fade)
{
    compages::renderer::Animator* animator =
        p_world.tryGet<compages::renderer::Animator>(p_model);
    if (animator == nullptr)
    {
        return false;
    }
    for (compages::renderer::AnimationClipId const id : animator->clips)
    {
        compages::renderer::AnimationClip const* clip = p_assets.animation(id);
        if ((clip != nullptr) && (clip->name == p_clip))
        {
            if (!(animator->clip == id))
            {
                // Fade only from a clip that is moving: a stopped one has no
                // pose to keep.
                if ((p_fade > si::Seconds(0.0)) && animator->clip.valid() && animator->playing)
                {
                    animator->previous = animator->clip;
                    animator->previous_time = animator->time;
                    animator->fade = p_fade;
                    animator->faded = si::Seconds(0.0);
                }
                else
                {
                    animator->previous = AnimationClipId{};
                }
                animator->clip = id;
                animator->time = si::Seconds(0.0);
            }
            animator->playing = true;
            return true;
        }
    }
    return false;
}

compages::si::LinearVelocity compages::renderer::AnimationSystem::groundSpeed(
    compages::world::World& p_world,
    compages::renderer::AssetManager& p_assets,
    compages::world::EntityId p_model,
    std::string_view p_clip)
{
    using compages::si::LinearVelocity;
    compages::renderer::Animator* animator =
        p_world.tryGet<compages::renderer::Animator>(p_model);
    if (animator == nullptr)
    {
        return LinearVelocity(0.0);
    }
    const compages::renderer::Animator saved = *animator;
    if (!play(p_world, p_assets, p_model, p_clip))
    {
        return LinearVelocity(0.0);
    }
    compages::renderer::AnimationClip const* clip = p_assets.animation(animator->clip);
    if ((clip == nullptr) || (clip->duration <= si::Seconds(0.0)))
    {
        *animator = saved;
        return LinearVelocity(0.0);
    }
    // The joints the clip moves: the feet are among them, the root of the
    // file is not.
    std::vector<compages::world::EntityId> moved;
    for (compages::renderer::AnimationChannel const& channel : clip->channels)
    {
        if ((channel.target < animator->targets.size()) &&
            p_world.alive(animator->targets[channel.target]))
        {
            moved.push_back(animator->targets[channel.target]);
        }
    }
    std::sort(moved.begin(), moved.end());
    moved.erase(std::unique(moved.begin(), moved.end()), moved.end());

    constexpr int SAMPLES = 60;
    const si::Seconds step = clip->duration / static_cast<double>(SAMPLES);
    animator->speed = 1.0f;
    animator->loop = true;
    animator->previous = AnimationClipId{};
    // Positions in the frame of the model, at time k * step.
    auto positions = [&](int p_k)
    {
        animator->time = step * static_cast<double>(p_k);
        animator->playing = true;
        static_cast<void>(sample(p_world, p_assets, si::Seconds(0.0)));
        p_world.update();
        const compages::Matrix44f to_model = compages::inverse(p_world.worldMatrix(p_model));
        std::vector<compages::Vector3f> out;
        out.reserve(moved.size());
        for (compages::world::EntityId const id : moved)
        {
            out.push_back(transformAffinePoint(to_model * p_world.worldMatrix(id),
                                               compages::Vector3f(0.0f, 0.0f, 0.0f)));
        }
        return out;
    };
    float distance = 0.0f;
    std::vector<compages::Vector3f> before = positions(0);
    for (int k = 1; (k <= SAMPLES) && !moved.empty(); ++k)
    {
        std::vector<compages::Vector3f> after = positions(k % SAMPLES);
        std::size_t lowest = 0u;
        for (std::size_t i = 1u; i < before.size(); ++i)
        {
            if (before[i].y < before[lowest].y)
            {
                lowest = i;
            }
        }
        // glTF is y up: the step on the ground is in x and z.
        const float dx = after[lowest].x - before[lowest].x;
        const float dz = after[lowest].z - before[lowest].z;
        distance += std::sqrt((dx * dx) + (dz * dz));
        before = std::move(after);
    }
    const compages::Vector3f scale = p_world.transform(p_model).scale;

    *animator = saved;
    static_cast<void>(sample(p_world, p_assets, si::Seconds(0.0)));
    p_world.update();
    return si::Length(static_cast<double>(distance * scale.x)) /
           clip->duration;
}

std::vector<std::string> compages::renderer::AnimationSystem::clips(
    compages::world::World const& p_world,
    compages::renderer::AssetManager const& p_assets,
    compages::world::EntityId p_model)
{
    std::vector<std::string> names;
    compages::renderer::Animator const* animator =
        p_world.tryGet<compages::renderer::Animator>(p_model);
    if (animator == nullptr)
    {
        return names;
    }
    for (compages::renderer::AnimationClipId const id : animator->clips)
    {
        compages::renderer::AnimationClip const* clip = p_assets.animation(id);
        if (clip != nullptr)
        {
            names.emplace_back(clip->name);
        }
    }
    return names;
}

std::string compages::renderer::AnimationSystem::playing(
    compages::world::World const& p_world,
    compages::renderer::AssetManager const& p_assets,
    compages::world::EntityId p_model)
{
    compages::renderer::Animator const* animator =
        p_world.tryGet<compages::renderer::Animator>(p_model);
    compages::renderer::AnimationClip const* clip =
        (animator == nullptr) ? nullptr : p_assets.animation(animator->clip);
    return (clip == nullptr) ? std::string() : clip->name;
}
