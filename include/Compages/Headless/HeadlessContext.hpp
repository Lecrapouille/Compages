// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"

namespace compages
{

// ****************************************************************************
//! \brief An OpenGL context with no window and no screen, made with EGL.
//!
//! Compages never makes its context: \c gpu::init() receives one that is
//! ready. A program with a window makes it with GLFW. This class makes one for
//! a program with no window: a test of robot cameras in CI, pictures made on a
//! server or in a container, a simulation that renders what a sensor sees. It
//! works on a GPU, or with Mesa llvmpipe and no GPU at all.
//!
//! It asks for the context the backend needs: OpenGL 4.5 core (GL45), 4.1 core
//! (GL41) or OpenGL ES 3.0 (GLES3). It uses no surface: everything is drawn
//! into framebuffers (\c RenderTarget, \c Scene::renderToFile).
//!
//! It lives in its own library, \c Compages-Headless (Linux, the \c egl
//! package), outside the three layers: they never open a context.
//!
//! \code
//! auto context = compages::HeadlessContext::open();
//! if (!context)
//! {
//!     return compages::failure(context.error());
//! }
//! COMPAGES_TRY(compages::gpu::init(&compages::HeadlessContext::load));
//! // ... draw ...
//! compages::gpu::shutdown();   // before the context goes
//! \endcode
//!
//! \note Call \c gpu::shutdown() before the context is destroyed: the GPU
//! objects are freed through it.
// ****************************************************************************
class HeadlessContext
{
public:

    // ------------------------------------------------------------------------
    //! \brief Open the first GPU EGL lists (no X server needed), or the
    //! default display, and make a context current on it with no surface.
    //!
    //! \return the context, current on this thread, or why EGL could not make
    //! it (no display, no desktop OpenGL, no surfaceless context).
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<HeadlessContext> open();

    //! \brief What \c compages::gpu::init() needs to find the OpenGL
    //! functions.
    static void* load(const char* p_name);

    HeadlessContext(HeadlessContext&& p_other) noexcept;
    HeadlessContext& operator=(HeadlessContext&& p_other) noexcept;
    HeadlessContext(HeadlessContext const&) = delete;
    HeadlessContext& operator=(HeadlessContext const&) = delete;

    //! \brief Release the context and the display.
    ~HeadlessContext();

private:

    HeadlessContext() = default;
    void release();

    //! \brief The EGLDisplay and the EGLContext, kept opaque so that this
    //! header does not include EGL.
    void* m_display = nullptr;
    void* m_context = nullptr;
};

} // namespace compages
