// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "main.hpp"

#include "Compages/GPU/ComputeProgram.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/Shader.hpp"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <string>

namespace tests
{

// ****************************************************************************
//! \brief An invisible window holding the context of the backend (4.5 core,
//! 4.1 core or ES 3.0), so
//! that tests touching the GPU can run without anything showing up on screen.
//!
//! The context is created but the OpenGL functions are deliberately not loaded
//! here: that is the job of compages::gpu::init(), which receives the loader returned by
//! procAddress(). Keeping the two apart is what lets the compages::gpu:: layer stay free
//! of any windowing dependency.
//!
//! Creation is allowed to fail. A machine without a GPU or without a display
//! cannot give us a context, and tests must be skipped rather than failed in
//! that case, hence the error() message instead of an exception.
// ****************************************************************************
class GPUContext
{
public:

    //! \brief Type of the function loading OpenGL symbols, as expected by
    //! compages::gpu::init().
    using LoadProc = void* (*)(const char*);

    // ------------------------------------------------------------------------
    //! \brief Create the invisible window and make its context current.
    //!
    //! \param[in] p_width,p_height how large the target is. One by one is enough
    //! for anything that never draws, which is most tests. A test that draws and
    //! then reads the picture back needs room for the picture.
    // ------------------------------------------------------------------------
    explicit GPUContext(int p_width = 1, int p_height = 1)
    {
        if (!glfwReady())
        {
            m_error = "GLFW could not be initialized. No display?";
            return;
        }

        // The context the backend of the library needs (Compages.mk).
#if defined(COMPAGES_GPU_BACKEND_GLES3)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(COMPAGES_GPU_BACKEND_GL41)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        m_window =
            glfwCreateWindow(p_width, p_height, "unit tests", nullptr, nullptr);
        if (m_window == nullptr)
        {
#if defined(COMPAGES_GPU_BACKEND_GLES3)
            m_error = "No OpenGL ES 3.0 context available";
#elif defined(COMPAGES_GPU_BACKEND_GL41)
            m_error = "No OpenGL 4.1 core profile context available";
#else
            m_error = "No OpenGL 4.5 core profile context available";
#endif
            return;
        }
        glfwMakeContextCurrent(m_window);
    }

    // ------------------------------------------------------------------------
    //! \brief Destroy the window. GLFW stays initialized until the end of the
    //! tests.
    // ------------------------------------------------------------------------
    ~GPUContext()
    {
        if (m_window != nullptr)
        {
            glfwDestroyWindow(m_window);
        }
    }

    GPUContext(GPUContext const&) = delete;
    GPUContext& operator=(GPUContext const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Is a context available?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool ready() const
    {
        return m_window != nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Why the context could not be created. Empty on success.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& error() const
    {
        return m_error;
    }

    // ------------------------------------------------------------------------
    //! \brief The function compages::gpu::init() needs to resolve OpenGL symbols.
    // ------------------------------------------------------------------------
    [[nodiscard]] static LoadProc procAddress()
    {
        return reinterpret_cast<LoadProc>(glfwGetProcAddress);
    }

    // ------------------------------------------------------------------------
    //! \brief Version of the context actually granted by the driver, which may
    //! be higher than the one requested.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::pair<int, int> version() const
    {
        if (m_window == nullptr)
        {
            return { 0, 0 };
        }
        return { glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MAJOR),
                 glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MINOR) };
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Initialize GLFW once for all the tests. An X server (Xvfb in a
    //! CI) accepts a limited number of clients, and each glfwInit() of a test
    //! cost one of them.
    // ------------------------------------------------------------------------
    [[nodiscard]] static bool glfwReady()
    {
        static bool const ready = []()
        {
            if (glfwInit() == GLFW_FALSE)
            {
                return false;
            }
            std::atexit(glfwTerminate);
            return true;
        }();
        return ready;
    }

    GLFWwindow* m_window = nullptr;
    std::string m_error;
};

// ****************************************************************************
//! \brief Base fixture for every test needing a GPU. Tests derived from it are
//! skipped, not failed, when no context can be obtained.
// ****************************************************************************
class GPUTest: public ::testing::Test
{
protected:

    void SetUp() override
    {
        m_context = std::make_unique<GPUContext>(targetWidth(), targetHeight());
        if (!m_context->ready())
        {
            GTEST_SKIP() << m_context->error();
        }
    }

    //! \brief How wide the target should be. Overridden by a fixture whose tests
    //! read the picture back.
    [[nodiscard]] virtual int targetWidth() const
    {
        return 1;
    }

    //! \brief How tall the target should be.
    [[nodiscard]] virtual int targetHeight() const
    {
        return 1;
    }

    void TearDown() override
    {
        m_context.reset();
    }

    //! \brief The live context, valid for the whole duration of the test.
    std::unique_ptr<GPUContext> m_context;
};

// ----------------------------------------------------------------------------
//! \brief The public API builds programs in place (loadSources()); tests often
//! want the Result instead, to check the error or to make several in a row.
// ----------------------------------------------------------------------------
inline compages::Result<compages::gpu::Program>
programFromSources(std::string_view p_vertex, std::string_view p_fragment)
{
    compages::gpu::Program program;
    COMPAGES_TRY(program.loadSources(p_vertex, p_fragment));
    return program;
}

//! \brief Same as programFromSources(), from files.
inline compages::Result<compages::gpu::Program>
programFromFiles(std::filesystem::path const& p_vertex,
                 std::filesystem::path const& p_fragment)
{
    compages::gpu::Program program;
    COMPAGES_TRY(program.loadFiles(p_vertex, p_fragment));
    return program;
}

//! \brief A ComputeProgram as a Result, for the same reason.
inline compages::Result<compages::gpu::ComputeProgram>
computeFromSource(std::string_view p_source)
{
    compages::gpu::ComputeProgram compute;
    COMPAGES_TRY(compute.loadSource(p_source));
    return compute;
}

//! \brief The Program inside a compute program built from p_source.
inline compages::Result<compages::gpu::Program>
programFromCompute(std::string_view p_source)
{
    compages::gpu::ComputeProgram compute;
    COMPAGES_TRY(compute.loadSource(p_source));
    return std::move(compute.program());
}

// ----------------------------------------------------------------------------
//! \brief Skip a test that needs what the backend compiled in (GL41 for
//! macOS, GLES3 for WebGL 2) does not have, saying what. The flag is a field
//! of gpu::DeviceInfo: call it after gpu::init().
// ----------------------------------------------------------------------------
#define COMPAGES_SKIP_WITHOUT(p_flag)                                                                            \
    if (!compages::gpu::device().p_flag)                                                                         \
    {                                                                                                            \
        GTEST_SKIP() << "this backend has no " #p_flag;                                                          \
    }

// ----------------------------------------------------------------------------
//! \brief Skip a test of what OpenGL ES 3.0 (the GLES3 backend) has not.
// ----------------------------------------------------------------------------
#if defined(COMPAGES_GPU_BACKEND_GLES3)
#    define COMPAGES_SKIP_ON_GLES3(p_why) GTEST_SKIP() << "OpenGL ES 3.0: " << (p_why)
#else
#    define COMPAGES_SKIP_ON_GLES3(p_why) (void)0
#endif

} // namespace tests
