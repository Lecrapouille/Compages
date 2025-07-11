#ifndef GL_OPENGL_CONTEXT_HPP
#define GL_OPENGL_CONTEXT_HPP

#include "Common/Exception.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace GL
{

// ***********************************************************************************************
//! \brief This macro will declare a class OpenGLException derived from
//! Exception.
// ***********************************************************************************************
DECLARE_EXCEPTION(Exception, BaseException)

class Context
{
public:

    using Window = GLFWwindow;

    static void makeCurrentContext(GL::Context::Window* context);

    static GL::Context::Window* getCurrentContext();

#ifdef CHECK_OPENGL
    //--------------------------------------------------------------------------
    //! \brief Allow to detect if the last OpenGL command succeeded or failed.
    //! In the case of failure an error is displayed on console and/or logged.
    //!
    //! \note do not use this function directly but use the macro glCheck.
    //!
    //! \param filename the file path where the OpenGL routine was called.
    //! \param line the line where the OpenGL routine was called.
    //! \param expression the line content where the OpenGL routine was called.
    //--------------------------------------------------------------------------
    static void checkError(const char* filename,
                           const uint32_t line,
                           const char* expression);
#endif // CHECK_OPENGL
};

#ifdef CHECK_OPENGL
#    define glCheck(expr) \
        expr;             \
        GL::Context::checkError(__FILE__, __LINE__, #expr);
#else
#    define glCheck(expr) expr;
#endif

} // namespace GL

#endif // GL_OPENGL_CONTEXT_HPP
