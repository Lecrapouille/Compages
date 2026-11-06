// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
//! \file
//! \brief Draw a scene into a PNG file on a machine without a screen.
//!
//! Compages never makes the OpenGL context: the program does, then calls
//! gpu::init(). The gallery makes it with a GLFW window. This program makes it
//! with EGL and no surface at all, which works on a server, in a container or
//! in a CI job, as long as the driver gives OpenGL 4.5 (Mesa llvmpipe does,
//! with no GPU).
//!
//! Usage: Compages-headless [out.png] [width] [height]
// ****************************************************************************

#include "Compages/Compages.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

// ****************************************************************************
//! \brief An OpenGL 4.5 core context with no window and no surface.
// ****************************************************************************
class HeadlessContext
{
public:

    ~HeadlessContext()
    {
        if (m_display != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(
                m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (m_context != EGL_NO_CONTEXT)
            {
                eglDestroyContext(m_display, m_context);
            }
            eglTerminate(m_display);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Open the first GPU EGL lists, or the default display, and make
    //! a context current on it without any surface.
    // ------------------------------------------------------------------------
    compages::Status open()
    {
        m_display = firstDevice();
        if (m_display == EGL_NO_DISPLAY)
        {
            m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        }
        if ((m_display == EGL_NO_DISPLAY) ||
            (eglInitialize(m_display, nullptr, nullptr) == EGL_FALSE))
        {
            return compages::failure("EGL found no display to draw with");
        }
        if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
        {
            return compages::failure("this EGL does not offer desktop OpenGL");
        }

        // A display without a window system has no window surfaces: ask for
        // the pbuffer kind, which every EGL has, even if none is made.
        const EGLint wanted[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                                  EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                                  EGL_NONE };
        EGLConfig config = nullptr;
        EGLint found = 0;
        if ((eglChooseConfig(m_display, wanted, &config, 1, &found) ==
             EGL_FALSE) ||
            (found == 0))
        {
            return compages::failure("EGL has no configuration for OpenGL");
        }

        const EGLint version[] = { EGL_CONTEXT_MAJOR_VERSION,
                                   4,
                                   EGL_CONTEXT_MINOR_VERSION,
                                   5,
                                   EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                   EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                   EGL_NONE };
        m_context =
            eglCreateContext(m_display, config, EGL_NO_CONTEXT, version);
        if (m_context == EGL_NO_CONTEXT)
        {
            return compages::failure(
                "EGL could not make an OpenGL 4.5 core context");
        }
        // No surface: everything is drawn into framebuffers.
        if (eglMakeCurrent(
                m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_context) ==
            EGL_FALSE)
        {
            return compages::failure(
                "EGL could not use the context without a surface "
                "(EGL_KHR_surfaceless_context is missing)");
        }
        return compages::success();
    }

    //! \brief What compages::gpu::init() needs to find the OpenGL functions.
    static void* load(const char* p_name)
    {
        return reinterpret_cast<void*>(eglGetProcAddress(p_name));
    }

private:

    //! \brief The first GPU, through EGL_EXT_platform_device, which needs no
    //! X server. EGL_NO_DISPLAY when the extension is missing.
    static EGLDisplay firstDevice()
    {
        auto query = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(
            eglGetProcAddress("eglQueryDevicesEXT"));
        auto platform = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if ((query == nullptr) || (platform == nullptr))
        {
            return EGL_NO_DISPLAY;
        }
        EGLDeviceEXT device = nullptr;
        EGLint count = 0;
        if ((query(1, &device, &count) == EGL_FALSE) || (count == 0))
        {
            return EGL_NO_DISPLAY;
        }
        return platform(EGL_PLATFORM_DEVICE_EXT, device, nullptr);
    }

    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLContext m_context = EGL_NO_CONTEXT;
};

// ----------------------------------------------------------------------------
//! \brief Three shapes, a camera and a sun, built, updated once, written.
// ----------------------------------------------------------------------------
compages::Status renderScene(std::string const& p_path,
                             std::uint32_t p_width,
                             std::uint32_t p_height)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.04f, 0.05f, 0.08f);
    scene.camera().position(0.0f, 2.0f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
    scene.box("Cube", compages::renderer::color(0.9f, 0.2f, 0.1f))
        .position(-1.2f, 0.0f, 0.0f)
        .rotation(0.6f, { 0.0f, 1.0f, 0.0f });
    scene.sphere("Ball", compages::renderer::color(0.2f, 0.7f, 0.3f));
    scene.cone("Cone", compages::renderer::color(0.2f, 0.4f, 0.9f))
        .position(1.2f, 0.0f, 0.0f);
    COMPAGES_TRY(scene.prepare());

    // One update computes the world matrices; then one picture.
    scene.update(compages::world::ViewFrame{});
    return scene.renderToFile(p_path, p_width, p_height);
}

} // namespace

int main(int argc, char* argv[])
{
    const std::string path = (argc > 1) ? argv[1] : "headless.png";
    const auto width = std::uint32_t((argc > 2) ? std::atoi(argv[2]) : 800);
    const auto height = std::uint32_t((argc > 3) ? std::atoi(argv[3]) : 600);

    HeadlessContext context;
    compages::Status status = context.open();
    if (status)
    {
        status = compages::gpu::init(&HeadlessContext::load);
    }
    if (status)
    {
        status = renderScene(path, width, height);
        compages::gpu::shutdown();
    }
    if (!status)
    {
        std::cerr << status.error() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "wrote " << path << std::endl;
    return EXIT_SUCCESS;
}
