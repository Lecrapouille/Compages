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

#include "OpenGL/Buffers/VAO.hpp"

//--------------------------------------------------------------------------
// Using .size() is not a good idea since Vector3f = 3 float.
// So compare bytes instead
bool GLVAO::checkVBOSizes()
{
    if (likely(!m_need_update))
        return true;

    if ((!isBound()) || m_vbos.empty())
    {
        std::cerr << "VAO " << name() << " is not yet bound to a GLProgram";
        return false;
    }

    bool consistent_vbo_sizes = true;
    m_count = m_vbos.begin()->second->size();
    for (auto& it : m_vbos)
    {
        if (m_count != it.second->size())
        {
            std::cerr << "VAO " << name()
                      << " does not have all of its VBOs with the same size:"
                      << std::endl;
            for (auto& itt : m_vbos)
            {
                std::cerr << "VBO " << itt.first << " size is "
                          << itt.second->size() << std::endl;
            }
            consistent_vbo_sizes = false;
        }
    }

    return consistent_vbo_sizes;
}

//----------------------------------------------------------------------------
// TODO: manage integers
void GLVAO::createVBOsFromAttribs(GLProgram::Attributes const& attributes)
{
    for (auto const& it : attributes)
    {
        const char* name = it.first.c_str();
        const GLint size = it.second->size(); // FIXME shall be unsigned
        const GLenum gltype = it.second->target();
        std::cout << name << std::endl;

        if (gltype == GL_FLOAT)
        {
            switch (size)
            {
                case 1u:
                    createVBO<float>(name);
                    break;
                case 2u:
                    createVBO<Vector2f>(name);
                    break;
                case 3u:
                    createVBO<Vector3f>(name);
                    break;
                case 4u:
                    createVBO<Vector4f>(name);
                    break;
                default:
                    throw GL::Exception(
                        "Attribute with dimension > 4 is not managed");
                    break;
            }
        }
        else if (gltype == GL_INT)
        {
            switch (size)
            {
                case 1u:
                    createVBO<int>(name);
                    break;
                case 2u:
                    createVBO<Vector2i>(name);
                    break;
                case 3u:
                    createVBO<Vector3i>(name);
                    break;
                case 4u:
                    createVBO<Vector4i>(name);
                    break;
                default:
                    throw GL::Exception(
                        "Attribute with dimension > 4 is not managed");
                    break;
            }
        }
        else if (gltype == GL_UNSIGNED_INT)
        {
            switch (size)
            {
                case 1u:
                    createVBO<unsigned int>(name);
                    break;
                case 2u:
                    createVBO<Vector2u>(name);
                    break;
                case 3u:
                    createVBO<Vector3u>(name);
                    break;
                case 4u:
                    createVBO<Vector4u>(name);
                    break;
                default:
                    throw GL::Exception(
                        "Attribute with dimension > 4 is not managed");
                    break;
            }
        }
        else
        {
            throw GL::Exception("Attribute with dimension > 4 is not managed");
        }
    }
}

//--------------------------------------------------------------------------
void GLVAO::createTexturesFromSamplers(GLProgram::Samplers const& samplers)
{
    for (auto const& it : samplers)
    {
        const char* name = it.first.c_str();
        const GLenum gltype = it.second->target();
        std::cout << name << std::endl;

        switch (gltype)
        {
            case GL_SAMPLER_1D:
                createTexture<GLTexture1D>(name);
                break;
            case GL_SAMPLER_2D: // TODO GLTextureDepth2D
                createTexture<GLTexture2D>(name);
                break;
            case GL_SAMPLER_3D:
                createTexture<GLTexture3D>(name);
                break;
            case GL_SAMPLER_CUBE:
                createTexture<GLTextureCube>(name);
                break;
            default:
                throw GL::Exception("This kind of sampler is not managed: " +
                                    std::to_string(gltype));
        }
    }
}

//--------------------------------------------------------------------------
bool GLVAO::draw(Mode const mode, size_t const first, size_t const count)
{
    if (likely(m_program != nullptr))
    {
        m_program->begin(); // glCheck(glUseProgram(m_program->handle()));
        begin();            // Optim: glBindVertexArray(m_vao->handle());

        // Activate textures
        for (auto& it : m_program->m_samplers)
        {
            it.second->begin();
            m_textures[it.first]->begin();
        }

        // Draw
        glCheck(glDrawArrays(static_cast<GLenum>(mode),
                             static_cast<GLint>(first),
                             static_cast<GLsizei>(count)));
        return true; // FIXME not always the case
    }
    else
    {
        std::cerr << "Failed OpenGL VAO has not been bound to a GLProgram"
                  << std::endl;
        return false;
    }
}

//--------------------------------------------------------------------------
bool GLVAO::onUpdate()
{
    if (!checkVBOSizes())
        return true;

    //    if (!m_vao->checkLoadTextures())
    //    return true;

    // std::cout << "GLVAO::onUpdate()" << std::endl;
    // m_vao->begin();

    // if

    for (auto& it : m_program->m_attributes)
    {
        m_vbos[it.first]->begin();
        it.second->begin();
    }

    // m_vao->end();

    return false;
}

//--------------------------------------------------------------------------
size_t GLVAO::getVBONames(std::vector<std::string>& list,
                          bool const clear) const
{
    if (clear)
    {
        list.clear();
    }
    list.reserve(m_vbos.size());
    for (auto& it : m_vbos)
    {
        list.push_back(it.second->name());
    }
    return list.size();
}

//--------------------------------------------------------------------------
size_t GLVAO::getTexturesNames(std::vector<std::string>& list,
                               bool const clear) const
{
    if (clear)
    {
        list.clear();
    }
    list.reserve(m_textures.size());
    for (auto& it : m_textures)
    {
        list.push_back(it.second->name()); // FIXME filename
    }
    return list.size();
}

//--------------------------------------------------------------------------
size_t GLVAO::getUnloadedTextures(std::vector<std::string>& list,
                                  bool const clear) const
{
    if (clear)
    {
        list.clear();
    }
    list.reserve(m_textures.size());
    for (auto& it : m_textures)
    {
        if (!it.second->loaded())
        {
            list.push_back(it.second->name()); // TODO filename()
        }
    }
    return list.size();
}
