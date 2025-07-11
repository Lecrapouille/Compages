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
// OpenGLCppWrapper is distributed in the hope that it will be useful, but
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

#ifndef OPENGLCPPWRAPPER_GLOBJECT_HPP
#define OPENGLCPPWRAPPER_GLOBJECT_HPP

#include "OpenGL/Context/OpenGL.hpp"
#include <cassert>

// FIXME: peut on supprimer les virtual onCreate() par un Curiously recurring
// template pattern ??
// ***************************************************************************
//! \class GLObject GLObject.hpp
//! \ingroup OpenGL
//!
//! \brief GLObject is an interface and ancestor class for wrapping OpenGL
//! objects such as VAO, VBO, Uniform, Attribute, Shader, Program, Texture,
//! FrameBuffer, and more ...
//!
//! Based on the observation that routines, for any kind of OpenGL objects (VAO,
//! VBO, Uniform ...), can be grouped into six main categories: creation/relase,
//! setup/update, activation/deactivation (see the table depicted in
//! doc/Architecture.md for more information), this class will hide the call of
//! these routines through public methods begin() and end() implementing the
//! template method pattern: these methods implement an algorithm calling
//! virtual methods onCreate(), onRelease(), onSetup(), onActivate(),
//! onUpdate(), onActivate(), onDeactivate() which will implemented in derived
//! classes (VAO, VBO, Uniform ...).
//!
//! \tparam T is the type of the OpenGL handle: either GLenum (aka GLunit) or
//! GLint. C++ is strongly typed and OpenGL API not fully consistent on this
//! type and change depending on the object (ie GLUniform). T avoid to produce
//! signed vs. unsigned conversion compilation warnings or usage of explicit
//! static_cast.
// ***************************************************************************
template <class T>
class GLObject: private NonCopyable
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor with the name of the object. Initialize private
    //! states of the sate machine begin() calling methods onCreate(),
    //! onActivate(), onSetup(), onUpdate().
    //!
    //! \param[in] name is the name of the instance. It is mainly used as key in
    //! lookup tables (such as GLProgram). It is also used in logs for debug
    //! purpose.
    //! \param[in] target: the target type.
    //--------------------------------------------------------------------------
    GLObject(std::string const& name, const GLenum target)
        : m_name(name), m_target(target)
    {
        m_handle = initialHandleValue();
    }

    //--------------------------------------------------------------------------
    //! \brief Virtual destructor because of pure virtual methods.
    //--------------------------------------------------------------------------
    virtual ~GLObject() = default;

    //--------------------------------------------------------------------------
    //! \brief Return the const reference of the name of this instance.
    //--------------------------------------------------------------------------
    inline const std::string& name() const
    {
        return m_name;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the const char* of the name of this instance.
    //--------------------------------------------------------------------------
    inline const char* cname() const
    {
        return m_name.c_str();
    }

    //--------------------------------------------------------------------------
    //! \brief Return the target type of the wrapped OpenGL object.
    //--------------------------------------------------------------------------
    inline GLenum target() const
    {
        return m_target;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the OpenGL object identifier that this instance wraps.
    //--------------------------------------------------------------------------
    inline T handle() const
    {
        return m_handle;
    }

    //--------------------------------------------------------------------------
    //! \brief Template method pattern. Perform some pending operations
    //! (creation, setup, update) when needed. This method is kind of state
    //! machine and only calls pure virtual methods such as onCreate(),
    //! onSetup(), onUpdate() ... which have to be implemented in derived
    //! classes.
    //--------------------------------------------------------------------------
    void begin()
    {
        if (m_need_create)
        {
#ifdef CHECK_OPENGL
            // OpenGL context shall be present
            m_context = GL::Context::getCurrentContext();
            if (m_context == nullptr)
                return;
#endif

            // Usually call glCreateXXX()
            m_need_create = onCreate();
        }

#ifdef CHECK_OPENGL
        assert(m_context == GL::Context::getCurrentContext() &&
               "You are trying to manipulate an OpenGL object "
               "that has been created from a different context");
#endif

        // Usually call glBindXXX()
        onActivate();

        // Configure (shaders, textures, VBOs ...)
        if (m_need_setup)
        {
            m_need_setup = onSetup();
            if (m_need_setup)
            {
                return;
            }
        }

        // Transfer CPU data to GPU data (texture, VBOs ...).
        // Note: We do not use if (m_need_update) because of particular case
        // needed with textures.
        if (needUpdate())
        {
            m_need_update = onUpdate();
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Deactivate the object on the GPU.
    //--------------------------------------------------------------------------
    inline void end()
    {
#ifdef CHECK_OPENGL
        assert(m_context == GL::Context::getCurrentContext() &&
               "You are trying to manipulate an OpenGL object "
               "that has been created from a different context");
#endif

        if (m_handle > initialHandleValue())
        {
            onDeactivate();
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Release the memory allocated only for GPU, states of the instance
    //! wrapping the OpenGL are reseted. This means the instance is still
    //! reusable.
    //!
    //! \note the OpenGL object will be created back after a call of begin().
    //--------------------------------------------------------------------------
    void release()
    {
#ifdef CHECK_OPENGL
        assert(((m_context == GL::Context::getCurrentContext()) ||
                (m_context == nullptr)) &&
               "You are trying to manipulate an OpenGL object "
               "that has been created in a different context");
        m_context = nullptr;
#endif

        if (m_handle > initialHandleValue())
        {
            onDeactivate();
            onRelease();
        }

        m_handle = initialHandleValue();
        m_target = 0U;
        m_need_setup = true;
        m_need_create = true;
        m_need_update = false;
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Return the reseted value of the handle of the OpenGL object.
    //!
    //! Because the OpenGL API is sometimes not fully consistent, the handle
    //! type may change depending on OpenGL objects as therefore its initial
    //! value. This method makes all behaviors consistent.
    //--------------------------------------------------------------------------
    inline T initialHandleValue() const;

    //--------------------------------------------------------------------------
    //! \brief Pure virtual. Allocate resources on the GPU.
    //!
    //! \return false if the object has been created by the GPU and does
    //! not be created again. Return true if the GPU failed to create
    //! the object and onCreate() needs to be called again.
    //--------------------------------------------------------------------------
    virtual bool onCreate() = 0;

    //--------------------------------------------------------------------------
    //! \brief Pure virtual. Activate the object on the GPU.
    //--------------------------------------------------------------------------
    virtual void onActivate() = 0;

    //--------------------------------------------------------------------------
    //! \brief Pure virtual. Configure the object behvior on the GPU.
    //!
    //! \return false if the object has been setup by the GPU and does
    //! not be setup again. Return true if the GPU failed to setup
    //! the object and onSetup() needs to be called again.
    //--------------------------------------------------------------------------
    virtual bool onSetup() = 0;

    //--------------------------------------------------------------------------
    //! \brief Pure virtual. Update the object on the GPU.
    //!
    //! \return false if the object has been transferred to the GPU and
    //! does not need to transferred again. Return true if the object has
    //! to be transferred again.
    //--------------------------------------------------------------------------
    virtual bool onUpdate() = 0;

    //--------------------------------------------------------------------------
    //! \brief Pure virtual. Deactivate the object on the GPU.
    //--------------------------------------------------------------------------
    virtual void onDeactivate() = 0;

    //--------------------------------------------------------------------------
    //! \brief Pure virtual. Delete the object from GPU memory.
    //--------------------------------------------------------------------------
    virtual void onRelease() {};

    //--------------------------------------------------------------------------
    //! \brief Give the information if data of the object has been
    //! transferred to the GPU (made when the method onUpdate() is called).
    //!
    //! Example vertices position of a VBO has been changed. They are
    //! transferred to the GPU, the model is now updated.
    //!
    //! \return false if the object has been transferred to the GPU and
    //! does not need to transferred again. Return true if the object has
    //! to be transferred again.
    //--------------------------------------------------------------------------
    virtual inline bool needUpdate() const
    {
        return m_need_update;
    }

protected:

#ifdef CHECK_OPENGL
    //! \brief Memorize the OpenGL context (meaning in which windows the object
    //! has been created). To check if the OpenGL is always access by the same
    //! context.
    GL::Context::Window* m_context = nullptr;
#endif

    //! \brief Object name used as the same time as a key for lookup tables and
    //! for debuging (logs, ...)
    std::string m_name;
    //! \brief Hold the identifier of the OpenGL object.
    //! \tparam can be either GLint or GLenum (aka for GLuint).
    T m_handle;
    //! \brief the type of object on the GPU.
    GLenum m_target;
    //! \brief hold the information if the OpenGL object has to do its setup.
    bool m_need_setup = true;
    //! \brief hold the information if the OpenGL object has to be created.
    bool m_need_create = true;
    //! \brief hold the information if the OpenGL object has to be transfered to
    //! the GPU.
    bool m_need_update = false;
};

//--------------------------------------------------------------------------
template <>
inline GLenum GLObject<GLenum>::initialHandleValue() const
{
    return 0u;
}

//--------------------------------------------------------------------------
template <>
inline GLint GLObject<GLint>::initialHandleValue() const
{
    return -1;
}

#endif // OPENGLCPPWRAPPER_GLOBJECT_HPP
