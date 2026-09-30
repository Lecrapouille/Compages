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
#include "Compages/Renderer/Render/RenderSnapshot.hpp"
#include "Compages/World/EntityId.hpp"

namespace compages::renderer
{

class Scene;

// ****************************************************************************
//! \brief Turns a Scene + compages::world::World into a RenderSnapshot.
//!
//! This is the explicit extraction brick of the frame contract
//! (\c Scene/FramePipeline.hpp). The compages::world::World stops here; the
//! Renderer never walks it. Extraction reads:
//! - the Scene's active camera (projection, transform, viewport, near/far);
//! - every compages::renderer::MeshRenderer, compages::world::SkinInstance
//! pose, compages::world::DirectionalLight and compages::world::PointLight;
//! - every world matrix produced by the last \c
//! compages::world::World::update();
//! - the AssetManager, to skip items that refer to stale ids.
//!
//! It writes a RenderSnapshot: a value that stands on its own.
//! compages::core::Frustum culling happens here, before the snapshot exists,
//! not after: an item whose world bounds fall entirely outside the camera
//! frustum is left out. LOD, visibility, batching and light selection belong
//! here later.
// ****************************************************************************
class SceneExtractor
{
public:

    // ------------------------------------------------------------------------
    //! \brief Build a snapshot out of a Scene, using the given aspect ratio
    //! for the perspective projection.
    //!
    //! Requires the Scene to have a valid active camera
    //! compages::world::EntityId with a compages::world::Camera component.
    //! Refuses otherwise, with a sentence: rendering with no camera should be
    //! an error, not a black screen.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<RenderSnapshot> extract(Scene const& p_scene,
                                                        float p_aspect);

    //! \brief Same, but taking width and height instead of aspect. Convenience.
    [[nodiscard]] static Result<RenderSnapshot> extract(Scene const& p_scene,
                                                        std::uint32_t p_width,
                                                        std::uint32_t p_height);

    //! \brief Same, through another camera than the active one.
    [[nodiscard]] static Result<RenderSnapshot>
    extract(Scene const& p_scene,
            compages::world::EntityId p_camera,
            std::uint32_t p_width,
            std::uint32_t p_height);
};

} // namespace compages::renderer
