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

#ifndef OPENGLCPPWRAPPER_GLUNIFORM_HPP
#  define OPENGLCPPWRAPPER_GLUNIFORM_HPP

#  include "OpenGL/Variables/Location.hpp"
#  include "Math/Matrix.hpp"

// *****************************************************************************
//! \brief Represent an uniform variable used in a GLSL shader program (refered
//! by \c uniform keyword). Example:
//! \code
//!   uniform mat4 projection;
//!   uniform vec4 color;
//! \endcode
//!
//! Uniforms are used as links between shader variables and your C++ variable
//! and therefore are entry points to the shader pipeline by allowing you to
//! upload CPU data to the GPU. Uniforms are used for modifying the value
//! dynamically but shall not be used directly by the user but internaly private
//! instances by GLProgram.  and therefore shall be acceeded through the
//! GLProgram API (GLProgram::uniform<T>(name))
//!
//! \tparam T float or int for scalars or VectorXf or VectorXi for vector or
//! MatrixXXf with X = [2 .. 4] for matrices or GLSamplerXD with X = [1 .. 3]
//! (and GLSamplerCube) for textures.
// *****************************************************************************
template<class T>
class GLUniform: public GLLocation
{
public:

    //--------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //! \param[in] name Give a name to the instance. The name shall be in accordance to
    //! the uniform variable in the GLSL shader. GLProgram uses these names as internal hash key.
    //! \param[in] size set the dimension of variable (1 for scalar, 2 .. 4 depending on the
    //! dimension of the vector).
    //! \param[in] gltype set the OpenGL type of data (GL_FLOAT, GL_INT ...)
    //! \param[in] prog the handle of the GLProgram (which is the owner of this
    //! instance).
    //--------------------------------------------------------------------------
    GLUniform(const char *name, const GLint dim, const GLint gltype, const GLuint prog)
        : GLLocation(name, dim, static_cast<GLenum>(gltype), prog)
    {}

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements from CPU and GPU.
    //--------------------------------------------------------------------------
    virtual ~GLUniform() override
    {
        release();
    }

    //--------------------------------------------------------------------------
    //! \brief Setter in the C# propety style. Modify the CPU data. The new value
    //! will be transfered to GPU memory on the next GLObject::begin() call.
    //--------------------------------------------------------------------------
    template<class U>
    GLUniform<T>& operator=(const U& val)
    {
        GLUniform<T>::m_data = T(val);
        m_need_update = true;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Getter in the C# propety style. Return the reference of the CPU
    //! data in read only mode.
    //--------------------------------------------------------------------------
    inline operator const T&() const
    {
        return m_data;
    }

    //--------------------------------------------------------------------------
    //! \brief Setter. Return the reference of CPU data in write mode. New value
    //! will be transfered to GPU memory.
    //! \fixme Could be nice to avoid transfering to the GPU if we set the same val
    //--------------------------------------------------------------------------
    inline operator T&()
    {
        m_need_update = true;
        return m_data;
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Create a new OpenGL Uniform.
    //! \return always false (success).
    //--------------------------------------------------------------------------
    virtual bool onCreate() override
    {
        m_handle = glCheck(glGetUniformLocation(m_program, cname()));
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Bind the OpenGL Uniform. This is a dummy method. No
    //! action is made.
    //--------------------------------------------------------------------------
    virtual void onActivate() override
    {}

    //--------------------------------------------------------------------------
    //! \brief Setup the behavior of the instance. This is a dummy
    //! method. No action is made.
    //! \return always false (success).
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Transfer the CPU data to the GPU data.
    //! \return always false (success).
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        apply(GLUniform<T>::m_data);
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Unbind the OpenGL Uniform. This is a dummy method. No
    //! action is made.
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override
    {}

    //--------------------------------------------------------------------------
    //! \brief Destroy the OpenGL Uniform. This is a dummy method. No
    //! action is made.
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {}

    //--------------------------------------------------------------------------
    //! \brief Transfer the CPU data to the GPU data.
    //--------------------------------------------------------------------------
    inline void apply(const T& value) const;

protected:

    T m_data {};
};

template<>
inline void GLUniform<float>::apply(const float& value) const
{
    glCheck(glUniform1f(m_handle, value));
}

template<>
inline void GLUniform<Vector2f>::apply(const Vector2f& value) const
{
    glCheck(glUniform2f(m_handle, value.x, value.y));
}

template<>
inline void GLUniform<Vector3f>::apply(const Vector3f& value) const
{
    glCheck(glUniform3f(m_handle, value.x, value.y, value.z));
}

template<>
inline void GLUniform<Vector4f>::apply(const Vector4f& value) const
{
    glCheck(glUniform4f(m_handle, value.x, value.y, value.z, value.w));
}

template<>
inline void GLUniform<int>::apply(const int& value) const
{
    glCheck(glUniform1i(m_handle, value));
}

template<>
inline void GLUniform<Vector2i>::apply(const Vector2i& value) const
{
    glCheck(glUniform2i(m_handle, value.x, value.y));
}

template<>
inline void GLUniform<Vector3i>::apply(const Vector3i& value) const
{
    glCheck(glUniform3i(m_handle, value.x, value.y, value.z));
}

template<>
inline void GLUniform<Vector4i>::apply(const Vector4i& value) const
{
    glCheck(glUniform4i(m_handle, value.x, value.y, value.z, value.w));
}

template<>
inline void GLUniform<unsigned int>::apply(const unsigned int& value) const
{
    glCheck(glUniform1ui(m_handle, value));
}

template<>
inline void GLUniform<Vector2u>::apply(const Vector2u& value) const
{
    glCheck(glUniform2ui(m_handle, value.x, value.y));
}

template<>
inline void GLUniform<Vector3u>::apply(const Vector3u& value) const
{
    glCheck(glUniform3ui(m_handle, value.x, value.y, value.z));
}

template<>
inline void GLUniform<Vector4u>::apply(const Vector4u& value) const
{
    glCheck(glUniform4ui(m_handle, value.x, value.y, value.z, value.w));
}

template<>
inline void GLUniform<Matrix22f>::apply(const Matrix22f& m) const
{
    // GL_FALSE because our matrices are already transposed (column-major).
    glCheck(glUniformMatrix2fv(m_handle, 1, GL_FALSE, m));
}

template<>
inline void GLUniform<Matrix33f>::apply(const Matrix33f& m) const
{
    // GL_FALSE because our matrices are already transposed (column-major).
    glCheck(glUniformMatrix3fv(m_handle, 1, GL_FALSE, m));
}

template<>
inline void GLUniform<Matrix44f>::apply(const Matrix44f& m) const
{
    // GL_FALSE because our matrices are already transposed (column-major).
    glCheck(glUniformMatrix4fv(m_handle, 1, GL_FALSE, m));
}

#endif // OPENGLCPPWRAPPER_GLUNIFORM_HPP
