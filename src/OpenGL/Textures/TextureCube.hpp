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

#ifndef OPENGLCPPWRAPPER_GLTEXTURE_CUBE_HPP
#define OPENGLCPPWRAPPER_GLTEXTURE_CUBE_HPP

// *****************************************************************************
//! \file GLTexture3D.hpp file implements:
//!   - GLTexture3D: which is a set of 2D Textures.
//!   - GLTextureCube: A 3D Texture specialized for rendering skybox.
// *****************************************************************************

#include "OpenGL/Textures/Texture2D.hpp"
#include <array>

// *****************************************************************************
//! \brief A 3D Texture specialized for rendering skybox.
// *****************************************************************************
class GLTextureCube: public GLTexture
{
public:

    //! \brief Cubic Texture.
    enum class Map : GLenum
    {
        /* 0x8515 */ POSITIVE_X = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        /* 0x8516 */ NEGATIVE_X = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        /* 0x8517 */ POSITIVE_Y = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        /* 0x8518 */ NEGATIVE_Y = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        /* 0x8519 */ POSITIVE_Z = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        /* 0x851A */ NEGATIVE_Z = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //!
    //! Give a name to the instance. This constructor makes no other
    //! actions.
    //!
    //! \param name the name of this instance used by GLProgram and GLVAO.
    //--------------------------------------------------------------------------
    GLTextureCube(std::string const& name)
        : GLTexture(3u, name, GL_TEXTURE_CUBE_MAP)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief \brief Allow to know if data have been transfered into
    //! the CPU memory.
    //!
    //! \return true if the 6 textures 2D (one by face of the cube) have
    //! been loaded.
    //--------------------------------------------------------------------------
    virtual inline bool loaded() const override
    {
        for (uint8_t i = 0; i < MAX_TEXTURES; ++i)
        {
            if (unlikely(false == m_textures[i]->loaded()))
                return false;
        }
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Load a texture 2D at the given location on the cube.
    //!
    //! \note to be a valid GLTextureCube all faces of the cube shall
    //! have loaded (so this method has to be called 6 times with the
    //! correct \p target).
    //!
    //! \param target select one of the 6 faces of the cube.
    //! \param filename the path of a jpeg or bmp or png file.
    //! \tparam L: class deriving from TextureLoader (ie SOIL).
    //!
    //! \return false if the texture failed to be loaded.
    //--------------------------------------------------------------------------
    template <class L>
    bool load(Map const target, const char* const filename)
    {
        const size_t index =
            static_cast<size_t>(target) -
            static_cast<size_t>(GL_TEXTURE_CUBE_MAP_POSITIVE_X);

        return m_textures[index]->load<L>(filename);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Apply OpenGL texture settings.
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        // Check if all textures have been loaded
        m_depth = countLoadedTextures();
        if (MAX_TEXTURES != m_depth)
        {
            // ERROR("Cannot setup texture '%s'. Reason '6 Textures2D shall have
            // been loaded'", cname());
            return true;
        }

        size_t i = MAX_TEXTURES;
        while (i--)
        {
            m_textures[i]->m_target =
                GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
            m_textures[i]->options(m_options);
            m_textures[i]->specifyTexture2D();
        }
        applyTextureParam();
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Upload dirty CPU data to the GPU.
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        glCheck(glBindTexture(m_target, m_handle));
        size_t i = MAX_TEXTURES;
        while (i--)
        {
            m_textures[i]->onUpdate();
        }
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    size_t countLoadedTextures()
    {
        size_t count = 0u;
        size_t i = MAX_TEXTURES;
        while (i--)
        {
            if (likely(m_textures[i]->loaded()))
                ++count;
        }
        return count;
    }

private:

    static constexpr size_t MAX_TEXTURES = 6u;

    //--------------------------------------------------------------------------
    //! \brief Create the 6 textures use for mapping the cube.
    //!
    //! \note: unique_ptr is to avoid calling the deleted GLTexture2D copy
    //! constructor (GLObject are non copyable).
    //--------------------------------------------------------------------------
    std::array<std::unique_ptr<GLTexture2D>, MAX_TEXTURES> m_textures{ {
        std::make_unique<GLTexture2D>("CUBEMAP_POSITIVE_X"),
        std::make_unique<GLTexture2D>("CUBEMAP_NEGATIVE_X"),
        std::make_unique<GLTexture2D>("CUBEMAP_POSITIVE_Y"),
        std::make_unique<GLTexture2D>("CUBEMAP_NEGATIVE_Y"),
        std::make_unique<GLTexture2D>("CUBEMAP_POSITIVE_Z"),
        std::make_unique<GLTexture2D>("CUBEMAP_NEGATIVE_Z"),
    } };
};

#endif // OPENGLCPPWRAPPER_GLTEXTURE_CUBE_HPP
