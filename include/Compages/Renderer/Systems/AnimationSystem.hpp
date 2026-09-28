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
//! 1. write local TRS from every playing Animator;
//! 2. \c World::update so joint world matrices are current;
//! 3. \c pose writes ibm * jointWorld * inv(meshWorld) onto each
//!    \c SkinInstance. The rest-pose vertex buffer stays on the GPU;
//!    the vertex shader is what deforms it.
//!
//! \c skin is the CPU prototype: it still rebuilds the VBO. Do not call
//! it from the frame loop.
//!
//! \code
//! COMPAGES_TRY(compages::renderer::AnimationSystem::tick(world, assets, frame.elapsed));
//! \endcode
// ****************************************************************************
class AnimationSystem
{
public:

    //! \brief Sample clips, update transforms, then pose skinned meshes.
    [[nodiscard]] static compages::Status tick(compages::world::World& p_world,
                                          AssetManager& p_assets,
                                          float p_dt);

    //! \brief Advance \c Animator time and write local TRS from clips.
    [[nodiscard]] static compages::Status sample(compages::world::World& p_world,
                                            AssetManager const& p_assets,
                                            float p_dt);

    //! \brief Fill \c SkinInstance::pose after world matrices are current.
    [[nodiscard]] static compages::Status pose(compages::world::World& p_world,
                                          AssetManager const& p_assets);

    //! \brief CPU vertex deformation. Prototype only.
    [[nodiscard]] static compages::Status skin(compages::world::World& p_world,
                                          AssetManager& p_assets);
};

} // namespace compages::renderer
