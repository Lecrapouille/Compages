// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
