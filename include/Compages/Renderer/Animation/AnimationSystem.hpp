// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/EntityId.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace compages::world
{
class World;
}

namespace compages::renderer
{

class AssetManager;

// ****************************************************************************
//! \brief Samples AnimationClips onto local transforms, then poses skins.
//!
//! Call order inside \c tick:
//! 1. write local TRS from every playing compages::renderer::Animator;
//! 2. \c compages::world::World::update so joint world matrices are current;
//! 3. \c pose writes ibm * jointWorld * inv(meshWorld) onto each
//!    \c compages::world::SkinInstance. The rest-pose vertex buffer stays on
//!    the GPU; the vertex shader is what deforms it.
//!
//! \c skin is the CPU prototype: it still rebuilds the VBO. Do not call
//! it from the frame loop.
//!
//! \c tick runs no routine: a host that steps the World with its own clock
//! calls it after World::update(), instead of Scene::update(). To show the
//! pose at a given time (a history slider), set \c Animator::time and tick
//! with no time: the same time always gives the same pose.
//!
//! \code
//! COMPAGES_TRY(compages::renderer::AnimationSystem::tick(world, assets,
//! frame.elapsed));
//! // at the time of the slider:
//! animator.time = slider_time;
//! COMPAGES_TRY(compages::renderer::AnimationSystem::tick(world, assets, 0.0_s));
//! \endcode
// ****************************************************************************
class AnimationSystem
{
public:

    //! \brief Sample clips, update transforms, then pose skinned meshes.
    [[nodiscard]] static Status
    tick(compages::world::World& p_world, AssetManager& p_assets, si::Seconds p_dt);

    //! \brief Advance \c compages::renderer::Animator time and write local TRS
    //! from clips.
    [[nodiscard]] static Status sample(compages::world::World& p_world,
                                       AssetManager const& p_assets,
                                       si::Seconds p_dt);

    //! \brief Fill \c compages::world::SkinInstance::pose after world matrices
    //! are current.
    [[nodiscard]] static Status pose(compages::world::World& p_world,
                                     AssetManager const& p_assets);

    //! \brief Play the clip of that name on a model with an Animator, from
    //! its start unless it is already the current clip.
    //!
    //! With \c p_fade above zero, the clip that was playing fades out under
    //! the new one during that time (see compages::renderer::Animator). A new
    //! play() during a fade starts a new fade from the clip that was current.
    //! \return false when the model has no Animator or no such clip.
    static bool play(compages::world::World& p_world,
                     AssetManager const& p_assets,
                     compages::world::EntityId p_model,
                     std::string_view p_clip,
                     si::Seconds p_fade = si::Seconds(0.0));

    //! \brief The speed at which a model must move for its feet not to
    //! slide while it plays a clip made in place (Walk, Run).
    //!
    //! A clip made in place moves the legs, not the body: the foot on the
    //! ground goes back under the hips. This measures that speed on the
    //! lowest joint the clip moves, at each of 60 times of the clip, in the
    //! frame of the model (its scale counted). A walker then sets
    //! Animator::speed = wanted speed / groundSpeed. It plays the clip on the
    //! model, then puts the Animator and the pose back: call it at load, not
    //! in the frame loop.
    //! \return zero for a clip with root motion, a still clip, or no such clip.
    //!
    //! \code
    //! const si::LinearVelocity stride = scene.groundSpeed(soldier, "Walk");
    //! \endcode
    [[nodiscard]] static si::LinearVelocity groundSpeed(compages::world::World& p_world,
                                                        AssetManager& p_assets,
                                                        compages::world::EntityId p_model,
                                                        std::string_view p_clip);

    //! \brief The names of the clips of a model, empty without an Animator.
    [[nodiscard]] static std::vector<std::string>
    clips(compages::world::World const& p_world,
          AssetManager const& p_assets,
          compages::world::EntityId p_model);

    //! \brief The name of the current clip of a model, or empty.
    [[nodiscard]] static std::string
    playing(compages::world::World const& p_world,
            AssetManager const& p_assets,
            compages::world::EntityId p_model);

    //! \brief CPU vertex deformation. Prototype only.
    [[nodiscard]] static Status skin(compages::world::World& p_world,
                                     AssetManager& p_assets);
};

} // namespace compages::renderer
