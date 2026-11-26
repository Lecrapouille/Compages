// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Units.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/World/EntityId.hpp"

#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Plays one AnimationClip onto the entities the clip's channels name.
//!
//! \c compages::renderer::AnimationSystem advances \c time and writes local TRS on the compages::world::World. It
//! does not own the clip: the AssetManager does. The component only stores
//! ids.
//!
//! A change of clip can fade: for \c fade seconds the old clip keeps playing
//! under the new one, and the weight of the new one grows from 0 to 1. A
//! walker that stops then goes from Walk to Idle without a jump of its pose.
//! Scene::play(model, "Idle", 0.3_s) starts the fade; this component only
//! keeps where it is.
//!
//! \code
//! entity.set(compages::renderer::Animator{ .clip = walkClip, .playing = true });
//! \endcode
// ****************************************************************************
struct Animator
{
    //! \brief The clip playing.
    AnimationClipId clip{};
    //! \brief Every clip the model came with, what Scene::play() chooses from.
    std::vector<AnimationClipId> clips;
    //! \brief Prefab-node index to compages::world::EntityId mapping for this instance.
    std::vector<compages::world::EntityId> targets;
    //! \brief Time in \c clip.
    si::Seconds time{ 0.0 };
    //! \brief Factor on the time of both clips: 2 plays twice as fast.
    float speed = 1.0f;
    bool loop = true;
    bool playing = true;
    //! \brief The clip fading out, invalid when no fade is running.
    AnimationClipId previous{};
    //! \brief Time in \c previous: it keeps moving during the fade.
    si::Seconds previous_time{ 0.0 };
    //! \brief Length of the running fade.
    si::Seconds fade{ 0.0 };
    //! \brief Time since the start of the fade; the weight of \c clip is
    //! faded / fade.
    si::Seconds faded{ 0.0 };
};

} // namespace compages::renderer
