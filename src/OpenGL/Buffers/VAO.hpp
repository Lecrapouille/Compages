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

#ifndef OPENGLCPPWRAPPER_GLVERTEX_ARRAY_HPP
#define OPENGLCPPWRAPPER_GLVERTEX_ARRAY_HPP

#include "OpenGL/Buffers/VBO.hpp"
#include "OpenGL/Shaders/Program.hpp"
#include "OpenGL/Textures/Textures.hpp"
#include <map>

//--------------------------------------------------------------------------
//! \brief Mode for drawing primitives (points, lines, triangles ...)
//--------------------------------------------------------------------------
enum class Mode : GLenum
{
    /* 0x0000 */ POINTS = GL_POINTS,
    /* 0x0001 */ LINES = GL_LINES,
    /* 0x0002 */ LINE_LOOP = GL_LINE_LOOP,
    /* 0x0003 */ LINE_STRIP = GL_LINE_STRIP,
    /* 0x0004 */ TRIANGLES = GL_TRIANGLES,
    /* 0x0005 */ TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
    /* 0x0006 */ TRIANGLE_FAN = GL_TRIANGLE_FAN,
    /* 0x000A */ LINES_ADJACENCY = GL_LINES_ADJACENCY,
    /* 0x000B */ LINE_STRIP_ADJACENCY = GL_LINE_STRIP_ADJACENCY,
    /* 0x000C */ TRIANGLES_ADJACENCY = GL_TRIANGLES_ADJACENCY,
    /* 0x000D */ TRIANGLE_STRIP_ADJACENCY = GL_TRIANGLE_STRIP_ADJACENCY,
    /* 0x000E */ PATCHES = GL_PATCHES,
};

//******************************************************************************
//! \brief Class Wrapping OpenGL VAO
//******************************************************************************
class GLVAO: public GLObject<GLenum>
{
    //! \brief GLProgram directly modifies GLVAO states.
    friend class GLProgram;

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor. Give a name to the instance. This name is mainly
    //! used for logs as debug purpose. This constructor makes no other actions.
    //!
    //! \param[in] name the name of the VAO instance.
    //! \param[in] usage for VBOs when they are created:
    //!   - BufferUsage::STREAM_DRAW: The data store contents will be modified
    //!     once and used at most a few times.
    //!   - BufferUsage::STATIC_DRAW: The data store contents will be modified
    //!     once and used many times.
    //!   - BufferUsage::DYNAMIC_DRAW: The data store contents will be modified
    //!     repeatedly and used many times.
    //! \param[in] reserve the number of elements when creating VBOs.
    //--------------------------------------------------------------------------
    GLVAO(std::string const& name,
          BufferUsage const usage = BufferUsage::DYNAMIC_DRAW,
          size_t const reserve = 3u)
        : GLObject(name, GL_ARRAY_BUFFER)
    {
        m_usage = usage;
        m_reserve = reserve;
    }

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements in CPU and GPU memories.
    //--------------------------------------------------------------------------
    virtual ~GLVAO() override
    {
        release();
    }

    //--------------------------------------------------------------------------
    //! \brief Set the usage for VBOs when they are created:
    //!   - BufferUsage::STREAM_DRAW: The data store contents will be modified
    //!     once and used at most a few times.
    //!   - BufferUsage::STATIC_DRAW: The data store contents will be modified
    //!     once and used many times.
    //!   - BufferUsage::DYNAMIC_DRAW: The data store contents will be modified
    //!     repeatedly and used many times.
    //--------------------------------------------------------------------------
    void usage(BufferUsage const usage)
    {
        m_usage = usage;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the number of elements to reserve when creating VBOs.
    //--------------------------------------------------------------------------
    void reserve(size_t const reserve)
    {
        m_reserve = reserve;
    }

    //--------------------------------------------------------------------------
    //! \brief Wrap the glDrawArrays() function
    //! \brief \param[in] mode:
    //--------------------------------------------------------------------------
    bool draw(Mode const mode, size_t const first, size_t const count);

    //--------------------------------------------------------------------------
    //! \brief Wrap the glDrawArrays() function
    //--------------------------------------------------------------------------
    inline bool draw(Mode const mode = Mode::TRIANGLES, size_t const first = 0u)
    {
        return GLVAO::draw(mode, first, m_count);
    }

    //--------------------------------------------------------------------------
    //! \brief Check if this instance has VBOs.
    //!
    //! \note Having no VBO generally means that this instance of VAO have not
    //! yet been bound to a GLProgram. See the bind() method.
    //!
    //! \return the number of VBOs.
    //--------------------------------------------------------------------------
    inline size_t hasVBOs() const
    {
        return m_vbos.size();
    }

    //--------------------------------------------------------------------------
    //! \brief Check if this instance holds a VBO named with the name passed as
    //! parameter.
    //!
    //! \return true if contains the named VBO, else return false.
    //--------------------------------------------------------------------------
    template <class T>
    bool hasVBO(const char* name)
    {
        assert(name != nullptr);

        auto it = m_vbos.find(name);
        if (it == m_vbos.end())
            return false;
        GLVertexBuffer<T>* vbo =
            dynamic_cast<GLVertexBuffer<T>*>(it->second.get());
        return (vbo != nullptr);
    }

    //--------------------------------------------------------------------------
    //! \brief Check if this instance has textures.
    //!
    //! \note Having no textures generally means that the shader has no sampler
    //! or this instance of VAO have not yet been bound to a GLProgram. See the
    //! bind() method.
    //!
    //! \return the number of textures.
    //--------------------------------------------------------------------------
    inline size_t hasTextures() const
    {
        return m_textures.size();
    }

    //--------------------------------------------------------------------------
    //! \brief Check if this instance holds a sampler named with the name passed
    //! as parameter.
    //!
    //! \return true if contains the named sampler, else return false.
    //--------------------------------------------------------------------------
    inline bool hasTexture(const char* name) const
    {
        assert(name != nullptr);
        return m_textures.find(name) != m_textures.end();
    }

    //--------------------------------------------------------------------------
    //! \brief Return the list of VBO names. VBO names come from names of
    //! attributes variables used inside GLSL shaders. This method is mainly
    //! used for debug purpose.
    //!
    //! \note if the VAO has never been bound to a GLProgram this method will
    //! return an empty list.
    //!
    //! \param[in,out] list the list where to insert VBO names.
    //! \param[in] if the list has to be cleared before being filled.
    //!
    //! \return the number of inserted elements.
    //--------------------------------------------------------------------------
    size_t getVBONames(std::vector<std::string>& list,
                       bool const clear = true) const;

    //--------------------------------------------------------------------------
    //! \brief Return the list of sampler names. Samplers come from names of
    //! uniform variables used in GLSL shaders. This is method is mainly used
    //! for debug purpose.
    //!
    //! \note if the VAO has never been bound to a GLProgram this method will
    //! return an empty list.
    //!
    //! \param[in,out] list the list where to insert sampler names.
    //! \param[in] if the list has to be cleared before being filled.
    //!
    //! \note Do not be confused with the sense of 'sampler name'. We do not
    //! refer to the name of the jpeg, png or bmp file. We refer to the shader
    //! sampler name (GLSL code). The GLTexture holds the acces of the file. To
    //! access to it do this VAO.texture2D["sampler_name"].name().
    //!
    //! \todo: get the tuple sampler name and texture name ?
    //! \return the number of inserted elements.
    //--------------------------------------------------------------------------
    size_t getTexturesNames(std::vector<std::string>& list,
                            bool const clear = true) const;
    size_t getUnloadedTextures(std::vector<std::string>& list,
                               bool const clear = true) const;

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 4D vector of type
    //! unsigned int.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<Vector4u>& vector4u(const char* name)
    {
        return getVBO<Vector4u>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 3D vector of type
    //! unsigned int.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<Vector3u>& vector3u(const char* name)
    {
        return getVBO<Vector3u>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 2D vector of type
    //! unsigned int.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<Vector2u>& vector2u(const char* name)
    {
        return getVBO<Vector2u>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a unsigned int
    //! scalar.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<unsigned int>& scalaru(const char* name)
    {
        return getVBO<unsigned int>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 4D vector of type
    //! float.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<Vector4f>& vector4f(const char* name)
    {
        return getVBO<Vector4f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 3D vector of type
    //! float.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<Vector3f>& vector3f(const char* name)
    {
        return getVBO<Vector3f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 2D vector of type
    //! float.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<Vector2f>& vector2f(const char* name)
    {
        return getVBO<Vector2f>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a float scalar.
    //!
    //! This method wraps the \a VBO() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the VBO does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLVertexBuffer<float>& scalarf(const char* name)
    {
        return getVBO<float>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 1D texture.
    //!
    //! This method wraps the \a texture() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the texture does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLTexture1D& texture1D(const char* name)
    {
        return getTexture<GLTexture1D>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 2D texture.
    //!
    //! This method wraps the \a texture() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the texture does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLTexture2D& texture2D(const char* name)
    {
        return getTexture<GLTexture2D>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a 3D texture.
    //!
    //! This method wraps the \a texture() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the texture does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLTexture3D& texture3D(const char* name)
    {
        return getTexture<GLTexture3D>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the named VBO holding a cubic texture.
    //!
    //! This method wraps the \a texture() method hidding the misery of the
    //! template.
    //!
    //! \throw GL::Exception if the texture does not exist or does not have the
    //! correct type.
    //--------------------------------------------------------------------------
    inline GLTextureCube& textureCube(const char* name)
    {
        return getTexture<GLTextureCube>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if this instance of VAO is bound to a GLProgram
    //--------------------------------------------------------------------------
    inline bool isBound() const
    {
        return (m_program != nullptr) && (m_program->handle() != 0u);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Return true if this instance of VAO is bound to the given
    //! GLProgram
    //--------------------------------------------------------------------------
    inline bool isBoundTo(GLenum const prog_id) const
    {
        return (m_program != nullptr) && (m_program->handle() == prog_id);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the number of elements in buffers.
    //--------------------------------------------------------------------------
    inline size_t count() const
    {
        return m_count;
    }

    //--------------------------------------------------------------------------
    //! \brief Populate VBOs
    //--------------------------------------------------------------------------
    void createTexturesFromSamplers(GLProgram::Samplers const& samplers);
    void createVBOsFromAttribs(GLProgram::Attributes const& attributes);

    //--------------------------------------------------------------------------
    //! \brief Create a new OpenGL VAO.
    //--------------------------------------------------------------------------
    virtual bool onCreate() override
    {
        glCheck(glGenVertexArrays(1, &m_handle));
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Bind the VAO to OpenGL.
    //--------------------------------------------------------------------------
    virtual void onActivate() override
    {
        glCheck(glBindVertexArray(m_handle));
    }

    //--------------------------------------------------------------------------
    //! \brief Dummy method. Nothing is made.
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override;

    //--------------------------------------------------------------------------
    //! \brief Unbind the VAO from OpenGL.
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override
    {
        glCheck(glBindVertexArray(0U));
    }

    //--------------------------------------------------------------------------
    //! \brief Destroy the OpenGL VAO.
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {
        glCheck(glDeleteVertexArrays(1, &m_handle));
    }

    //--------------------------------------------------------------------------
    //! \brief All VBOs shall have the same number of elements
    //--------------------------------------------------------------------------
    bool checkVBOSizes();

    //--------------------------------------------------------------------------
    //! \brief Create a Vertex Buffer Object. Called by GLProgram when
    //! populating the bound VAO with VBOs from attribute names used in shader
    //! GLSL code.
    //!
    //! \param[in] name the attribute name for the VBO.
    //! \note \a name duplicata is not managed because this case should never
    //! happen (meaning two attribute names are used for different type which is
    //! not allowed and detected by the GLSL compiler).
    //! \return true if the VBO has been created, else return false.
    //--------------------------------------------------------------------------
    template <typename T>
    void createVBO(const char* name)
    {
        assert(name != nullptr);

        auto it = m_vbos.find(name);
        if (it != m_vbos.end())
            return;

        m_vbos[name] =
            std::make_unique<GLVertexBuffer<T>>(name, m_reserve, m_usage);
    }

    //--------------------------------------------------------------------------
    //! \brief Create a texture. Called by GLProgram when populating the bound
    //! VAO with textures frome sampler names used in shader GLSL code.
    //!
    //! \param[in] name the sampler name for the texture.
    //! \tparam T GLTexture1D or GLTexture2D or GLTexture3D or GLTextureCube.
    //!
    //! \note name duplicata is not managed because this case should never
    //! happen (meaning two attribute names are used for different type which is
    //! not allowed and detected by the GLSL compiler).
    //!
    //! \return true if the texture has been created, else return false.
    //--------------------------------------------------------------------------
    template <typename T>
    void createTexture(const char* name)
    {
        assert(name != nullptr);

        auto it = m_textures.find(name);
        if (it != m_textures.end())
            return;

        m_textures[name] = std::make_unique<T>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Find and return a VBO. Create and store a VBO if and only if the
    //! VAO is not yet bound to a GLProgram.
    //--------------------------------------------------------------------------
    template <class T>
    GLVertexBuffer<T>& getVBO(const char* name) // TODO const:  foo = getVBO(()
    {
        assert(name != nullptr);

        if (isBound())
        {
            auto it = m_vbos.find(name);
            if (it != m_vbos.end())
            {
                GLVertexBuffer<T>* vbo =
                    dynamic_cast<GLVertexBuffer<T>*>(it->second.get());
                if (vbo != nullptr)
                {
                    m_need_update = true; // TODO const:  foo = getVBO()
                    return *vbo;
                }

                throw GL::Exception("GLVertexBuffer " + std::string(name) +
                                    " exists but has wrong template type");
            }
            throw GL::Exception("GLVertexBuffer " + std::string(name) +
                                " does not exist");
        }
        else
        {
            // The API allows the user to define VBOs before compiling the
            // GLProgram and bounding VAO to this program.
            if (m_vbos.find(name) == m_vbos.end())
                createVBO<T>(name);

            GLVertexBuffer<T>* vbo =
                dynamic_cast<GLVertexBuffer<T>*>(m_vbos[name].get());
            m_need_update = true;
            return *vbo;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Locate and get the reference of ther texture refered by its type
    //! T and by the given the sampler name used in GLSL code.
    //! \fixme m_textures_verified is always changed to false but shall be only
    //! bed modified when PendingContainer has been changed.
    //! \note Do not confuse sampler name with texture filename.
    //! param name is the GLSL sampler name.
    //! \return the reference of the texture if it exists.
    //! \throw GL::Exception if the texture is not in the list or if the type
    //! T does not match or by a typo in the name of the sampler.
    //--------------------------------------------------------------------------
    template <typename T>
    T& getTexture(const char* name)
    {
        assert(name != nullptr);
        if (isBound())
        {
            auto it = m_textures.find(name);
            if (m_textures.end() != it)
            {
                T* tex = dynamic_cast<T*>(it->second.get());
                if (tex != nullptr)
                {
                    m_need_update = true; // TODO gerer foo = getTexture()
                    return *tex;
                }

                throw GL::Exception("GLTexture '" + std::string(name) +
                                    "' exists but has wrong template type");
            }
            throw GL::Exception("GLTexture '" + std::string(name) +
                                "' does not exist");
        }
        else
        {
            // The API allows the user to define textures before compiling the
            // GLProgram.
            if (m_textures.find(name) == m_textures.end())
                createTexture<T>(name);

            T* tex = dynamic_cast<T*>(m_textures[name].get());
            m_need_update = true;
            return *tex;
        }
    }

protected:

    using VBOs = std::map<std::string, std::unique_ptr<IGLBuffer>>;
    using Textures = std::map<std::string, std::unique_ptr<GLTexture>>;

    VBOs m_vbos;
    Textures m_textures;
    GLProgram* m_program = nullptr;
    size_t m_count = 0u;
    BufferUsage m_usage;
    size_t m_reserve;
};

#endif // OPENGLCPPWRAPPER_GLVERTEX_ARRAY_HPP
