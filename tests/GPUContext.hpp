// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "main.hpp"

#include <GLFW/glfw3.h>
#include <string>

namespace tests
{

// ****************************************************************************
//! \brief An invisible window holding an OpenGL 4.5 core profile context, so
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
        if (glfwInit() == GLFW_FALSE)
        {
            m_error = "GLFW could not be initialized. No display?";
            return;
        }
        m_glfw_ready = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        m_window =
            glfwCreateWindow(p_width, p_height, "unit tests", nullptr, nullptr);
        if (m_window == nullptr)
        {
            m_error = "No OpenGL 4.5 core profile context available";
            return;
        }
        glfwMakeContextCurrent(m_window);
    }

    // ------------------------------------------------------------------------
    //! \brief Destroy the window and shut GLFW down.
    // ------------------------------------------------------------------------
    ~GPUContext()
    {
        if (m_window != nullptr)
        {
            glfwDestroyWindow(m_window);
        }
        if (m_glfw_ready)
        {
            glfwTerminate();
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

    GLFWwindow* m_window = nullptr;
    bool m_glfw_ready = false;
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

} // namespace tests
