// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Headless/HeadlessContext.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <string>
#include <utility>

namespace compages
{

namespace
{

// ----------------------------------------------------------------------------
//! \brief The first GPU, through EGL_EXT_platform_device, which needs no X
//! server. EGL_NO_DISPLAY when the extension is missing.
// ----------------------------------------------------------------------------
EGLDisplay firstDevice()
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

// The context the backend compiled into Compages-GPU needs.
#if defined(COMPAGES_GPU_BACKEND_GLES3)
constexpr EGLenum kApi = EGL_OPENGL_ES_API;
constexpr EGLint kRenderable = EGL_OPENGL_ES3_BIT;
constexpr EGLint kVersion[] = { EGL_CONTEXT_MAJOR_VERSION, 3,
                                EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };
constexpr const char* kName = "an OpenGL ES 3.0";
#else
constexpr EGLenum kApi = EGL_OPENGL_API;
constexpr EGLint kRenderable = EGL_OPENGL_BIT;
#    if defined(COMPAGES_GPU_BACKEND_GL41)
constexpr EGLint kMinor = 1;
constexpr const char* kName = "an OpenGL 4.1 core";
#    else
constexpr EGLint kMinor = 5;
constexpr const char* kName = "an OpenGL 4.5 core";
#    endif
constexpr EGLint kVersion[] = { EGL_CONTEXT_MAJOR_VERSION,
                                4,
                                EGL_CONTEXT_MINOR_VERSION,
                                kMinor,
                                EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                EGL_NONE };
#endif

} // namespace

Result<HeadlessContext> HeadlessContext::open()
{
    HeadlessContext context;
    EGLDisplay display = firstDevice();
    if (display == EGL_NO_DISPLAY)
    {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if ((display == EGL_NO_DISPLAY) ||
        (eglInitialize(display, nullptr, nullptr) == EGL_FALSE))
    {
        return failure("EGL found no display to draw with");
    }
    // From here the destructor terminates the display, whatever fails next.
    context.m_display = display;
    if (eglBindAPI(kApi) == EGL_FALSE)
    {
        return failure(std::string("this EGL cannot make ") + kName +
                       " context");
    }

    // A display without a window system has no window surfaces: ask for the
    // pbuffer kind, which every EGL has, even if none is made.
    const EGLint wanted[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE, kRenderable, EGL_NONE };
    EGLConfig config = nullptr;
    EGLint found = 0;
    if ((eglChooseConfig(display, wanted, &config, 1, &found) == EGL_FALSE) ||
        (found == 0))
    {
        return failure(std::string("EGL has no configuration for ") + kName +
                       " context");
    }
    EGLContext egl = eglCreateContext(display, config, EGL_NO_CONTEXT, kVersion);
    if (egl == EGL_NO_CONTEXT)
    {
        return failure(std::string("EGL could not make ") + kName + " context");
    }
    context.m_context = egl;
    // No surface: everything is drawn into framebuffers.
    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl) ==
        EGL_FALSE)
    {
        return failure("EGL could not use the context without a surface "
                       "(EGL_KHR_surfaceless_context is missing)");
    }
    return context;
}

void* HeadlessContext::load(const char* p_name)
{
    return reinterpret_cast<void*>(eglGetProcAddress(p_name));
}

HeadlessContext::HeadlessContext(HeadlessContext&& p_other) noexcept
    : m_display(std::exchange(p_other.m_display, nullptr)),
      m_context(std::exchange(p_other.m_context, nullptr))
{
}

HeadlessContext& HeadlessContext::operator=(HeadlessContext&& p_other) noexcept
{
    if (this != &p_other)
    {
        release();
        m_display = std::exchange(p_other.m_display, nullptr);
        m_context = std::exchange(p_other.m_context, nullptr);
    }
    return *this;
}

HeadlessContext::~HeadlessContext()
{
    release();
}

void HeadlessContext::release()
{
    if (m_display == nullptr)
    {
        return;
    }
    EGLDisplay display = static_cast<EGLDisplay>(m_display);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_context != nullptr)
    {
        eglDestroyContext(display, static_cast<EGLContext>(m_context));
    }
    eglTerminate(display);
    m_display = nullptr;
    m_context = nullptr;
}

} // namespace compages
