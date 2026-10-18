// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Render/RenderQueue.hpp"
#include "Compages/Renderer/Render/RenderSnapshot.hpp"

namespace compages::renderer
{

class AssetManager;

// ****************************************************************************
//! \brief Consumes a RenderSnapshot and produces \c compages::gpu:: calls.
//!
//! The Renderer is stateless in the sense that it does not own snapshots,
//! queues or camera frames: each call takes them as arguments. It owns a
//! scratch \c RenderQueue that is reused frame after frame, so a per-frame
//! allocation is avoided.
//!
//! It does not open the pass. Whoever prepares the frame opens the pass; the
//! Renderer draws into it. That way an application can compose several draws
//! into one pass (main + overlay, main + gizmos) without the Renderer needing
//! to know.
// ****************************************************************************
class Renderer
{
public:

    Renderer() = default;
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Draw the snapshot into the open pass.
    //! \param[in] p_snapshot what to draw.
    //! \param[in,out] p_assets the AssetManager to resolve ids against. Must
    //! be the one the snapshot was extracted from. Not const: a mesh or a
    //! material seen for the first time is sent to the GPU here.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status render(RenderSnapshot const& p_snapshot,
                                compages::renderer::AssetManager& p_assets);

    //! \brief The queue that was built during the last render, for inspection.
    [[nodiscard]] RenderQueue const& queue() const
    {
        return m_queue;
    }

    //! \brief The shadow of the sun, as the lit materials read it.
    struct Shadow
    {
        bool enabled = false;
        compages::core::Matrix44f light_view_projection{ compages::core::matrix::Identity };
        //! World size of one pixel of the shadow picture.
        float texel = 0.0f;
        //! Depth tolerance, in the units of the shadow picture.
        float bias = 0.0f;
    };

private:

    //! \brief Draws the depth seen from the sun into the shadow picture.
    [[nodiscard]] Status renderShadow(RenderSnapshot const& p_snapshot,
                                      compages::renderer::AssetManager& p_assets);

    RenderQueue m_queue;
    Shadow m_shadow;
    compages::gpu::Texture m_shadow_depth;
    compages::gpu::Framebuffer m_shadow_target;
    compages::gpu::Program m_shadow_program;
    compages::gpu::Pipeline m_shadow_pipeline;
    //! \brief One white pixel, sampled by a textured material drawn without
    //! its picture, so that its sampler never reads an empty unit.
    compages::gpu::Texture m_white;
};

} // namespace compages::renderer
