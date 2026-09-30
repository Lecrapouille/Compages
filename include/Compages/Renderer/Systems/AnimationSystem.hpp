// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"

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
//! \code
//! COMPAGES_TRY(compages::renderer::AnimationSystem::tick(world, assets,
//! frame.elapsed));
//! \endcode
// ****************************************************************************
class AnimationSystem
{
public:

    //! \brief Sample clips, update transforms, then pose skinned meshes.
    [[nodiscard]] static Status
    tick(compages::world::World& p_world, AssetManager& p_assets, float p_dt);

    //! \brief Advance \c compages::renderer::Animator time and write local TRS
    //! from clips.
    [[nodiscard]] static Status sample(compages::world::World& p_world,
                                       AssetManager const& p_assets,
                                       float p_dt);

    //! \brief Fill \c compages::world::SkinInstance::pose after world matrices
    //! are current.
    [[nodiscard]] static Status pose(compages::world::World& p_world,
                                     AssetManager const& p_assets);

    //! \brief CPU vertex deformation. Prototype only.
    [[nodiscard]] static Status skin(compages::world::World& p_world,
                                     AssetManager& p_assets);
};

} // namespace compages::renderer
