// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/World/Components/Camera.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief A picture a Scene draws into, instead of the window: the view of a
//! robot camera, a thumbnail, a picture for a test or a file.
//!
//! It holds a colour picture (RGBA8, the colours of the screen) and a depth
//! picture of the same size, and keeps them from one frame to the next.
//! Scene::render(camera, target) draws into it; color() shows it (in an
//! ImGui window, on a material), readColor() brings it to the CPU.
//!
//! \code
//! COMPAGES_TRY_ASSIGN(m_eye, compages::renderer::RenderTarget::create(640u, 480u));
//! // each frame
//! scene.render(robot_camera, m_eye);
//! COMPAGES_TRY_ASSIGN(pixels, m_eye.readColor());   // top row first
//! \endcode
// ****************************************************************************
class RenderTarget
{
public:

    //! \brief No picture: valid() is false until create() or resize().
    RenderTarget() = default;

    // ------------------------------------------------------------------------
    //! \brief A target of \c p_width x \c p_height pixels.
    //! \return the target, or why the GPU could not make its pictures.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<RenderTarget> create(std::uint32_t p_width, std::uint32_t p_height);

    // ------------------------------------------------------------------------
    //! \brief Make the pictures \c p_width x \c p_height. Does nothing when
    //! the size is already that one (call it each frame with the size of a
    //! panel). On failure the target is left empty.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status resize(std::uint32_t p_width, std::uint32_t p_height);

    //! \brief True once it holds pictures.
    [[nodiscard]] bool valid() const
    {
        return m_framebuffer.valid();
    }

    [[nodiscard]] std::uint32_t width() const
    {
        return m_width;
    }

    [[nodiscard]] std::uint32_t height() const
    {
        return m_height;
    }

    //! \brief The colour picture, to show or to read in a shader.
    [[nodiscard]] compages::gpu::Texture const& color() const
    {
        return m_color;
    }

    //! \brief The depth picture (Depth32F, reversed: 1 near, 0 far).
    [[nodiscard]] compages::gpu::Texture const& depth() const
    {
        return m_depth;
    }

    // ------------------------------------------------------------------------
    //! \brief The time of the World it last showed: the \c total of the last
    //! World step before Scene::render(camera, target), zero before the
    //! first.
    //!
    //! A camera of a robot stamps its picture with it, never with the time
    //! it was read: the two differ as soon as the picture is read later, or
    //! by a host whose simulation runs faster than the pictures.
    // ------------------------------------------------------------------------
    [[nodiscard]] si::Seconds time() const
    {
        return m_time;
    }

    //! \brief What a RenderPass opens to draw into it.
    [[nodiscard]] compages::gpu::Framebuffer const& framebuffer() const
    {
        return m_framebuffer;
    }

    // ------------------------------------------------------------------------
    //! \brief The colour pixels, RGBA8, the top row first (as an image file,
    //! OpenCV or a robot camera expect). Waits for the GPU: for a sensor, a
    //! test or a file, at its own rate.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<std::vector<std::byte>> readColor() const;

    // ------------------------------------------------------------------------
    //! \brief The depth of each pixel in metres, along the axis of the camera
    //! (what an RGB-D camera gives), the top row first. A pixel where nothing
    //! was drawn gives +infinity. Waits for the GPU, as readColor().
    //!
    //! \code
    //! scene.render(eye, target);
    //! auto metres = target.readDepth(eye.get<compages::world::Camera>());
    //! \endcode
    //!
    //! \param[in] p_lens the camera the picture was drawn with: its near and
    //! far planes, and its projection, undo the depth of the GPU.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<std::vector<float>> readDepth(compages::world::Camera const& p_lens) const;

    // ------------------------------------------------------------------------
    //! \brief Write the colour picture into a PNG file.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status saveColor(std::filesystem::path const& p_path) const;

private:

    // Scene::render(camera, target) writes the time it drew.
    friend class Scene;

    compages::gpu::Texture m_color;
    compages::gpu::Texture m_depth;
    compages::gpu::Framebuffer m_framebuffer;
    std::uint32_t m_width = 0u;
    std::uint32_t m_height = 0u;
    si::Seconds m_time{ 0.0 };
};

} // namespace compages::renderer
