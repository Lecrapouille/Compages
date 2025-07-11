//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "UI/Window.hpp"
#include "OpenGL/Buffers/GPUMemory.hpp"
#include "UI/Layer.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

GLWindow::Mouse GLWindow::m_mouse;
std::vector<char> GLWindow::m_lastKeys;
std::vector<char> GLWindow::m_currentKeys;

//--------------------------------------------------------------------------
//! \brief static method initializing Glew context
//--------------------------------------------------------------------------
static void initGlew()
{
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        throw GL::Exception("Failed to initialize GLFW");

    // Print out some info about the graphics drivers
    std::cout << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
              << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << std::endl;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

    // Make sure OpenGL version 3.3 API is available
    if (!GLEW_VERSION_3_3)
        throw GL::Exception("OpenGL 3.3 API is not available!");

#pragma GCC diagnostic pop
}

class Glfw3: private NonCopyable
{
public:

    static Glfw3& instance()
    {
        static Glfw3 instance;
        return instance;
    }

protected:

    Glfw3()
    {
        std::cout << "Glfw3()" << std::endl;

        // Callback triggered when GLFW failed.
        glfwSetErrorCallback([](int /*errorCode*/, const char* msg)
                             { throw GL::Exception(msg); });

        if (!glfwInit())
        {
            throw GL::Exception("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Mac OS X
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    }

    ~Glfw3()
    {
        std::cout << "~Glfw3()" << std::endl;
        glfwTerminate();
    }
};

//------------------------------------------------------------------------------
GLWindow::GLWindow(uint32_t const width,
                   uint32_t const height,
                   const char* title)
    : m_title(title)
{
    Glfw3::instance();

    m_lastKeys.resize(GLFW_KEY_LAST + 1);
    m_currentKeys.resize(GLFW_KEY_LAST + 1);

    staticWidth() = width;
    staticHeight() = height;

    GL::Context::Window* current = GL::Context::getCurrentContext();
    m_context = glfwCreateWindow(static_cast<int>(width),
                                 static_cast<int>(height),
                                 m_title,
                                 nullptr,
                                 nullptr);
    if (m_context == nullptr)
        throw GL::Exception("Failed to open GLFW window");

    GL::Context::makeCurrentContext(m_context);
    glfwSwapInterval(1); // Enable vsync
    initGlew();
    GL::Context::makeCurrentContext(current);
}

//------------------------------------------------------------------------------
GLWindow::~GLWindow()
{
    GL::Context::Window* current = GL::Context::getCurrentContext();
    GL::Context::makeCurrentContext(m_context);

    // Layers
    // for (auto& it: m_layers)
    //{
    //    it->onRelease();
    //}

    glfwDestroyWindow(m_context);
    GL::Context::makeCurrentContext(current);
}

//------------------------------------------------------------------------------
void GLWindow::hideCursor()
{
    m_mouse.visible = false;
    glfwSetInputMode(m_context, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

//------------------------------------------------------------------------------
void GLWindow::showCursor()
{
    m_mouse.visible = true;
    glfwSetInputMode(m_context, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

//------------------------------------------------------------------------------
void GLWindow::resize(uint32_t const width, uint32_t const height)
{
    staticWidth() = std::max(1u, width);
    staticHeight() = std::max(1u, height);

    // Callback to be implemented by the derived class
    onWindowResized();
}

//------------------------------------------------------------------------------
void GLWindow::computeFPS()
{
    double currentTime = glfwGetTime();
    m_deltaTime = static_cast<float>(currentTime - m_lastFrameTime);
    m_lastFrameTime = currentTime;
    ++nbFrames;

    if (currentTime - m_lastTime >= 1.0)
    {
        // If last prinf() was more than 1sec ago printf and reset
        std::ostringstream oss;
        m_fps = nbFrames;
        int ms_by_frame = static_cast<int>(1000.0f / static_cast<float>(m_fps));
        oss << '[' << m_fps << " FPS, " << ms_by_frame << " ms] " << m_title;
        glfwSetWindowTitle(m_context, oss.str().c_str());
        nbFrames = 0u;
        m_lastTime += 1.0;
    }
}

//------------------------------------------------------------------------------
//! \brief Display the GPU memory consumption in human readable format
//------------------------------------------------------------------------------
void GLWindow::monitorGPUMemory()
{
    size_t current_gpu_mem = GPUMemory();

    if (previous_gpu_mem != current_gpu_mem)
    {
        previous_gpu_mem = current_gpu_mem;
        onGPUMemoryChanged(current_gpu_mem);
    }
}

//------------------------------------------------------------------------------
// \note we use lambda functions in the aim to make private GLWindow callbacks.
// Else, if using static function, we could not reach private methods.
// \fixme callbacksOn(Event::All) + callbacksOn(Event::None) => callbacks are
// not removed.
void GLWindow::reactTo(GLWindow::Event const events)
{
    // Note I use lambda functions instead of static functions to access to
    // private methods.

    if ((events & GLWindow::Event::MouseMove) != GLWindow::Event::None)
    {
        // Mouse moved event
        glfwSetCursorPosCallback(
            m_context,
            [](GLFWwindow* obj, double xpos, double ypos)
            {
                static double lastX = xpos;
                static double lastY = ypos;

                // Get the window.
                assert(nullptr != obj);
                GLWindow* window =
                    static_cast<GLWindow*>(glfwGetWindowUserPointer(obj));

                // Save context.
                GL::Context::Window* current = GL::Context::getCurrentContext();
                GL::Context::makeCurrentContext(window->m_context);

                // Update mouse states
                window->m_mouse.position.x = xpos;
                window->m_mouse.position.y = ypos;
                window->m_mouse.displacement.x = xpos - lastX;
                window->m_mouse.displacement.y =
                    lastY - ypos; // Reversed since
                                  // y-coordinates go
                                  // from bottom to top
                lastX = xpos;
                lastY = ypos;

                // Callback to be implemented by the derived class
                window->onMouseMoved();

                // Clear states
                window->m_mouse.displacement.x = 0.0;
                window->m_mouse.displacement.y = 0.0;

                // Restore context.
                GL::Context::makeCurrentContext(current);
            });
    }

    // Mouse scroll event
    if ((events & GLWindow::Event::MouseScroll) != GLWindow::Event::None)
    {
        glfwSetScrollCallback(
            m_context,
            [](GLFWwindow* obj, double xoffset, double yoffset)
            {
                // Get the window.
                assert(nullptr != obj);
                GLWindow* window =
                    static_cast<GLWindow*>(glfwGetWindowUserPointer(obj));

                // Save context.
                GL::Context::Window* current = GL::Context::getCurrentContext();
                GL::Context::makeCurrentContext(window->m_context);

                // Update mouse states
                window->m_mouse.scroll.x = xoffset;
                window->m_mouse.scroll.y = yoffset;

                // Callback to be implemented by the derived class
                window->onMouseScrolled();

                // Clear states
                window->m_mouse.scroll.x = 0.0;
                window->m_mouse.scroll.y = 0.0;

                // Restore context.
                GL::Context::makeCurrentContext(current);
            });
    }

    // Mouse click event
    if ((events & GLWindow::Event::MouseButton) != GLWindow::Event::None)
    {
        glfwSetMouseButtonCallback(
            m_context,
            [](GLFWwindow* obj, int button, int action, int /*mods*/)
            {
                // Get the window.
                assert(nullptr != obj);
                GLWindow* window =
                    static_cast<GLWindow*>(glfwGetWindowUserPointer(obj));

                // Save context.
                GL::Context::Window* current = GL::Context::getCurrentContext();
                GL::Context::makeCurrentContext(window->m_context);

                // Update mouse states
                window->m_mouse.button =
                    static_cast<GLWindow::Mouse::Button>(button);
                window->m_mouse.pressed =
                    (action == GLFW_PRESS); // else GLFW_RELEASE

                // Callback to be implemented by the derived class
                window->onMouseButtonPressed();

                // Clear states
                window->m_mouse.button = GLWindow::Mouse::Button::NONE;
                window->m_mouse.pressed = false;

                // Restore context.
                GL::Context::makeCurrentContext(current);
            });
    }

    // Key pressed or released events
    if ((events & GLWindow::Event::Keyboard) != GLWindow::Event::None)
    {
        glfwSetKeyCallback(
            m_context,
            [](GLFWwindow* obj,
               int key,
               int /*scancode*/,
               int action,
               int /*mods*/)
            {
                // Get the window.
                assert(nullptr != obj);
                GLWindow* window =
                    static_cast<GLWindow*>(glfwGetWindowUserPointer(obj));

                // Save context.
                GL::Context::Window* current = GL::Context::getCurrentContext();
                GL::Context::makeCurrentContext(window->m_context);

                // Update keyboard states
                {
                    const std::lock_guard<std::mutex> lock(
                        window->m_mutex_keyboard);
                    window->m_currentKeys[static_cast<size_t>(key)] =
                        (action == GLFW_PRESS) ? GLWindow::KEY_PRESS
                                               : GLWindow::KEY_RELEASE;
                }

                // Restore context.
                GL::Context::makeCurrentContext(current);
            });
    }
}

//------------------------------------------------------------------------------
bool GLWindow::setup()
{
    m_should_halt = false;

    // Set this window as active context
    GL::Context::makeCurrentContext(m_context);

    // Save the context address: it will be passed as parameter in glfw
    // callbacks
    glfwSetWindowUserPointer(m_context, this);

    // Set the windows resized callback. I use lambda function instead of static
    // function to access to private methods.
    glfwSetFramebufferSizeCallback(
        m_context,
        [](GLFWwindow* obj, int width, int height)
        {
            // Get the window.
            assert(nullptr != obj);
            GLWindow* window =
                static_cast<GLWindow*>(glfwGetWindowUserPointer(obj));

            // Save context.
            GL::Context::Window* current = GL::Context::getCurrentContext();
            GL::Context::makeCurrentContext(window->m_context);

            // Call the GLWindow callback that has to be implemented by the
            // derived class.
            window->staticWidth() = std::max(1u, static_cast<uint32_t>(width));
            window->staticHeight() =
                std::max(1u, static_cast<uint32_t>(height));
            window->onWindowResized();

            // Restore context.
            GL::Context::makeCurrentContext(current);
        });

    // Ensure we can capture keyboard being pressed below.
    glfwSetInputMode(m_context, GLFW_STICKY_KEYS, GL_TRUE);
    {
        const std::lock_guard<std::mutex> lock(m_mutex_keyboard);
        for (size_t i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; ++i)
        {
            m_lastKeys[i] = m_currentKeys[i] = GLWindow::KEY_RELEASE;
        }
    }

    // Flush OpenGL errors before using this function on real OpenGL
    // routines else a fake error is returned on the first OpenGL
    // routines while valid.
    glCheck();

    try
    {
        // Init OpenGL stats as well as user application states.
        if (unlikely(!onSetup()))
        {
            // Aborted explicitely by the developper
            onSetupFailed("Has returned false");
            return false;
        }

        // Layers
        for (auto& it : m_layers)
        {
            if (unlikely(!it->onSetup()))
            {
                // Aborted explicitely by the developper
                onSetupFailed("Layer " + it->name() + " has returned false");
                return false;
            }
        }

        // Force refreshing computation made when window changed
        onWindowResized();
        glfwSwapBuffers(m_context);
    }
    catch (const GL::Exception& e)
    {
        onSetupFailed(e.message());
        return false;
    }

    // init FPS
    m_lastTime = glfwGetTime();
    m_lastFrameTime = m_lastTime;
    m_fps = 0;

    // Draw OpenGL scene
    return true;
}

//------------------------------------------------------------------------------
bool GLWindow::update()
{
    // Set this window as active context
    GL::Context::makeCurrentContext(m_context);

    computeFPS();
    monitorGPUMemory();

    try
    {
        // Render the background scene
        if (likely(false == onPaint()))
        {
            onPaintFailed("Has returned false");
            return false;
        }

        // Render Layers
        for (auto& it : m_layers)
        {
            if (unlikely(!it->onPaint()))
            {
                // Aborted explicitely by the developper
                onPaintFailed("Layer " + it->name() + " has returned false");
                return false;
            }
        }
    }
    catch (const GL::Exception& e)
    {
        onPaintFailed(e.message());
        return false;
    }

    // Swap buffers
    glfwSwapBuffers(m_context);

    // Update window events (input etc)
    glfwPollEvents();
    {
        const std::lock_guard<std::mutex> lock(m_mutex_keyboard);
        onKeyboardEvent();
        for (size_t i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; ++i)
            m_lastKeys[i] = m_currentKeys[i];
    }

    return true;
}

//------------------------------------------------------------------------------
bool GLWindow::haltCondition()
{
    // Check if the ESC key was pressed or the window was closed
    // Note: use keyPressed() not isKeyDown() because if Event::Keyboard is not
    // we no longer can halt the window.
    return (GLFW_PRESS == glfwGetKey(m_context, GLFW_KEY_ESCAPE)) ||
           (glfwWindowShouldClose(m_context));
}

//------------------------------------------------------------------------------
bool GLWindow::run()
{
    if (!setup())
    {
        return false;
    }

    do
    {
        if (!update())
        {
            // This will call onPaintFailed()
            return false;
        }

        m_should_halt |= haltCondition();
    } while (!m_should_halt);

    return true;
}
