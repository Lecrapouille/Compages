// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/World/EntityId.hpp"

#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Plays one AnimationClip onto the entities the clip's channels name.
//!
//! \c AnimationSystem advances \c time and writes local TRS on the World. It
//! does not own the clip: the AssetManager does. The component only stores
//! ids.
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
    //! \brief Prefab-node index to EntityId mapping for this instance.
    std::vector<compages::world::EntityId> targets;
    float time = 0.0f;
    float speed = 1.0f;
    bool loop = true;
    bool playing = true;
};

} // namespace compages::renderer
