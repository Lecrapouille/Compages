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
//! \brief What RenderTarget::readLater() reads: the colour, the depth, or both.
//!
//! \code
//! target.readLater({ .color = true, .depth = true });
//! \endcode
// ****************************************************************************
struct ReadChannels
{
    //! \brief The colour pixels, RGBA8.
    bool color = false;
    //! \brief The depth of each pixel, in metres.
    bool depth = false;
};

// ****************************************************************************
//! \brief The pictures of a RenderTarget brought to the CPU, as the camera of
//! a robot gives them: rows top first.
// ****************************************************************************
struct Capture
{
    //! \brief The time of the World the pictures show (RenderTarget::time()
    //! when the read was asked).
    si::Seconds time{ 0.0 };
    //! \brief RGBA8, the top row first; empty if not asked.
    std::vector<std::byte> color;
    //! \brief Metres along the axis of the camera, the top row first,
    //! +infinity where nothing was drawn; empty if not asked.
    std::vector<float> depth;
};

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
    //! \brief Ask for the pictures to come to the CPU, without waiting: take
    //! them later with takeRead().
    //!
    //! readColor() and readDepth() stop the CPU until the GPU is done. A
    //! camera at 30 Hz asks after its render, and takes the pictures one frame
    //! later, when readReady() says they are there: the GPU copies them while
    //! the CPU goes on. The depth is turned into metres with the lens the last
    //! render used, and the capture keeps the time() of that render.
    //!
    //! \code
    //! scene.render(eye, target);
    //! COMPAGES_TRY(target.readLater({ .color = true, .depth = true }));
    //! // next frame
    //! if (target.readReady())
    //! {
    //!     COMPAGES_TRY_ASSIGN(capture, target.takeRead());
    //! }
    //! \endcode
    //!
    //! \return an error on an empty target, when nothing is asked, or when a
    //! read is already on its way (take it first).
    // ------------------------------------------------------------------------
    [[nodiscard]] Status readLater(ReadChannels p_channels);

    //! \brief True when a read was asked and the GPU has finished it. Never
    //! waits.
    [[nodiscard]] bool readReady() const;

    //! \brief True between readLater() and takeRead().
    [[nodiscard]] bool reading() const
    {
        return m_color_read.valid() || m_depth_read.valid();
    }

    // ------------------------------------------------------------------------
    //! \brief The pictures asked by readLater(). Waits for the GPU if they are
    //! not readReady(); then the read is over.
    //!
    //! \return the capture, or an error when no read was asked.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<Capture> takeRead();

    // ------------------------------------------------------------------------
    //! \brief Write the colour picture into a PNG file.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status saveColor(std::filesystem::path const& p_path) const;

private:

    // Scene::render(camera, target) writes the time and the lens it drew.
    friend class Scene;

    [[nodiscard]] Result<std::vector<float>> toMetres(std::vector<std::byte> const& p_bytes,
                                                      compages::world::Camera const& p_lens) const;
    void flipRows(std::vector<std::byte>& p_pixels) const;

    compages::gpu::Texture m_color;
    compages::gpu::Texture m_depth;
    compages::gpu::Framebuffer m_framebuffer;
    std::uint32_t m_width = 0u;
    std::uint32_t m_height = 0u;
    si::Seconds m_time{ 0.0 };
    //! \brief The lens of the last render, to turn its depth into metres.
    compages::world::Camera m_lens{};
    compages::gpu::PendingRead m_color_read;
    compages::gpu::PendingRead m_depth_read;
    //! \brief What a read in flight shows: the time and the lens of its
    //! render.
    si::Seconds m_read_time{ 0.0 };
    compages::world::Camera m_read_lens{};
};

} // namespace compages::renderer
