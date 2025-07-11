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
//
// This file is a derived work of https://github.com/glumpy/glumpy
//
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_GLSHADER_HPP
#define OPENGLCPPWRAPPER_GLSHADER_HPP

// ****************************************************************************
//! \file Shaders.hpp holds the GLSL shader code, offers some macros
//! code for processing the code (like code inclusion) and allow to
//! compile code source.
//!
//! \file Shaders.hpp file implements:
//!   - GLShader:
//!   - GLFragmentShader:
//!   - GLVertexShader:
//!   - GLGeometryShader:
// ****************************************************************************

#include "Common/Path.hpp"
#include "OpenGL/GLObject.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h> // strerror
#include <vector>

// ****************************************************************************
//! \brief Class holding the GLSL shader code, offering some routines for
//! processing the code (like including code files) and allowing to compile the
//! code source.
// ****************************************************************************
class GLShader: public GLObject<GLenum>
{
    friend class GLProgram;
    friend std::ostream& operator<<(std::ostream& os, GLShader const& shader);
    template <typename T>
    friend GLShader& operator>>(T const& s, GLShader& shader);

public:

    //-------------------------------------------------------------------------
    //! \brief Constructor. Initialize states but no actions are made.
    //! \param[in] name: the name of the instance (can be non-unique). It is
    //! mainly used for debug purpose but can be used as key for lookup tables.
    //! \param[in] target: type of desired shader: GL_VERTEX_SHADER or
    //! GL_FRAGMENT_SHADER or GL_GEOMETRY_SHADER.
    //-------------------------------------------------------------------------
    GLShader(std::string const& name, const GLenum target);

    //-------------------------------------------------------------------------
    //! \brief Release GPU memory.
    //! \param[in] name is the name of the instance. It is mainly used as key in
    //! lookup tables (such as GLProgram). It is also used in logs for debug
    //! purpose.
    //-------------------------------------------------------------------------
    virtual ~GLShader() override;

    //-------------------------------------------------------------------------
    //! \brief Replace the current shader code source by the new one.
    //! \param[in] code: the new code source.
    //! \return this instance of class.
    //-------------------------------------------------------------------------
    template <typename T>
    GLShader& operator=(T const& code)
    {
        throw_if_already_compiled();
        m_code = code;
        return *this;
    }

    //-------------------------------------------------------------------------
    //! \brief Append shader code to the existing one.
    //! \param[in] code: the new code source to append to m_code.
    //! \return this instance of class.
    //-------------------------------------------------------------------------
    template <typename T>
    GLShader& operator+=(T const& code)
    {
        throw_if_already_compiled();
        m_code += code;
        return *this;
    }

    //-------------------------------------------------------------------------
    //! \brief Append shader code to the existing one.
    //! \param[in] code: the new code source to append to the member variable
    //! m_code.
    //! \return this instance of class.
    //-------------------------------------------------------------------------
    template <typename T>
    GLShader& operator<<(T const& code)
    {
        throw_if_already_compiled();
        m_code += code;
        return *this;
    }

    //-------------------------------------------------------------------------
    //! \brief Needed for accepting std::endl when appending shader codes.
    //-------------------------------------------------------------------------
    GLShader& operator<<(std::ostream& (* /*os*/)(std::ostream&))
    {
        return operator<<('\n');
    }

    //-------------------------------------------------------------------------
    //! \brief Read the whole shader code from the given ascii file.
    //! Note: the shader code compilation is delayed. The code is only
    //! stored in the member variable m_code.
    //!
    //! \param[in] file the file of the shader code.
    //! \note: if you wish, the file and its included files can be found thanks
    //! to GLShader::path. See Path for more information. For example:
    //! \code
    //!   GLShader s;
    //!   s.path.add("first/path:/full/second/path");
    //! \endcode
    //!
    //! \return true if the whole file has been read, else return false.
    //! An error message is set and can be read through getError().
    //-------------------------------------------------------------------------
    bool read(std::string const& file);

    //-------------------------------------------------------------------------
    //! \brief Return the shader error message. Calling this method makes clear
    //! the error.
    //-------------------------------------------------------------------------
    std::string strerror();

    //-------------------------------------------------------------------------
    //! \brief Return in read only mode the code source of the shader.
    //-------------------------------------------------------------------------
    inline std::string const& code() const
    {
        return m_code;
    }

    //-------------------------------------------------------------------------
    //! \brief Return true if the instance has no dummy code.
    //-------------------------------------------------------------------------
    inline bool loaded() const
    {
        return !m_code.empty();
    }

    //-------------------------------------------------------------------------
    //! \brief Erase the current shader code
    //-------------------------------------------------------------------------
    inline void clear()
    {
        m_code.clear();
    }

    //-------------------------------------------------------------------------
    //! \brief Compile the shader.
    //!
    //! \return true if shader has been compiled, else return false. You also
    //! call compiled() after to get this information back. You call strerror()
    //! in case of failure to get the reason in human readable format.
    //-------------------------------------------------------------------------
    bool compile();

    //-------------------------------------------------------------------------
    //! \brief Return if the shader has been compiled ?
    //!
    //! \return true if shader has been compiled, else return false.
    //-------------------------------------------------------------------------
    inline bool compiled() const
    {
        return !m_need_setup;
    }

private:

    // TODO solveIncludes()

    //-------------------------------------------------------------------------
    //! \brief The shader is created inside the GPU.
    //-------------------------------------------------------------------------
    virtual bool onCreate() override;

    //-------------------------------------------------------------------------
    //! \brief Dummy method.
    //-------------------------------------------------------------------------
    virtual void onActivate() override;

    //-------------------------------------------------------------------------
    //! \brief Compile the shader code in the GPU.
    //!
    //! \return true if the compilation succeeded. Else return false.
    //! An error message is set and can be read through getError().
    //-------------------------------------------------------------------------
    virtual bool onSetup() override;

    //-------------------------------------------------------------------------
    //! \brief Dead method: never called !
    //-------------------------------------------------------------------------
    virtual bool onUpdate() override;

    //-------------------------------------------------------------------------
    //! \brief Dummy method.
    //-------------------------------------------------------------------------
    virtual void onDeactivate() override;

    //-------------------------------------------------------------------------
    //! \brief The shader is released from the GPU.
    //-------------------------------------------------------------------------
    virtual void onRelease() override;

    //-------------------------------------------------------------------------
    //! \brief Check if a shader script has been correctly compiled.
    //!
    //! An error message is set in m_error and which can be read through
    //! getError().
    //!
    //! \param[in] obj the identifer of the loaded shader.
    //! \return true if the shader has been compiled with success else return
    //! false.
    //-------------------------------------------------------------------------
    bool checkCompilationStatus(GLuint obj);

    //-------------------------------------------------------------------------
    //! \brief Throw GL::Exception if no shader have been loaded. Please if
    //! this happened, call methods read(), operator=(), operator+=(),
    //! operator<<().
    //-------------------------------------------------------------------------
    void throw_if_not_loaded();

    //-------------------------------------------------------------------------
    //! \brief Throw GL::Exception if try to modify the shader code while
    //! already loaded to the GPU.
    //-------------------------------------------------------------------------
    void throw_if_already_compiled();

    //-------------------------------------------------------------------------
    //! \brief Concat the last error to the list of errors
    //-------------------------------------------------------------------------
    void concatError(std::string const& msg);

private:

    //-------------------------------------------------------------------------
    //! \brief Since includes are not an allowed tokens, this method allows
    //! their usage.
    //! \return false if no errors occured (ie missing files).
    //-------------------------------------------------------------------------
    bool solveIncludes();

    //-------------------------------------------------------------------------
    //! \brief Concrete implementation for read(std::string const& file).
    //!
    //! \param[in] file the file of the shader code.
    //!
    //! \param[inout] code the code read form the file. Note: the string is not
    //! cleared before reading the file and therefore old code is still present.
    //! Call std::string::clear() before if needed.
    //!
    //! \param[in] path
    //!
    //! \return true if the whole file has been read, else return false.
    //! An error message is set and can be read through getError().
    //-------------------------------------------------------------------------
    bool read(std::string const& file, std::string& code);

public:

    //! \brief Helper for searching included GLSL shader files.
    Path path;

private:

    //! \brief Hold the code source of the shader
    std::string m_code;
    //! \brief Current file name
    std::string m_file_name;
    //! \brief Hold error messages
    std::string m_error;
};

//--------------------------------------------------------------------------
//! \brief Display on stream the shader code.
//--------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, GLShader const& shader)
{
    return os << shader.m_code;
}

//--------------------------------------------------------------------------
//! \brief Prepend the shader code to the shader.
//--------------------------------------------------------------------------
template <typename T>
inline GLShader& operator>>(T const& code, GLShader& shader)
{
    shader.m_code = code + shader.m_code;
    return shader;
}

#endif // OPENGLCPPWRAPPER_GLSHADER_HPP
