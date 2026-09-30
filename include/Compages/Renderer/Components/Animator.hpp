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

#pragma once

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
    float time = 0.0f;
    float speed = 1.0f;
    bool loop = true;
    bool playing = true;
};

} // namespace compages::renderer
