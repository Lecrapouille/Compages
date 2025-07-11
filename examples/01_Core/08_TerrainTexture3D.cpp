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

#include "08_TerrainTexture3D.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

//------------------------------------------------------------------------------
TerrainTexture3D::TerrainTexture3D(uint32_t const width,
                                   uint32_t const height,
                                   const char* title)
    : GLWindow(width, height, title), m_vao("VAO_terrain"), m_prog("prog")
{
    std::cout << "Hello TerrainTexture3D: " << info() << std::endl;
}

//------------------------------------------------------------------------------
TerrainTexture3D::~TerrainTexture3D()
{
    std::cout << "Bye TerrainTexture3D" << std::endl;
}

//------------------------------------------------------------------------------
void TerrainTexture3D::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    m_prog.matrix44f("projection") = matrix::perspective(
        60.0_deg, width<float>() / height<float>(), 0.1f, 10.0f);
}

//------------------------------------------------------------------------------
bool TerrainTexture3D::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_vertex_shader.read("01_Core/shaders/08_TerrainTexture3D.vs");
    m_fragment_shader.read("01_Core/shaders/08_TerrainTexture3D.fs");
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    // Init Model-View matrices (shader uniforms). Note that projection matrix
    // is init inside onWindowResized() which is called just after this method.
    m_prog.matrix44f("model") = Matrix44f(matrix::Identity);
    m_prog.matrix44f("view") = matrix::lookAt(Vector3f(0.75, -0.75, 0.75),
                                              Vector3f(0.0, 0.0, 0.0),
                                              Vector3f(0.0, 0.0, 1.0));

    // Create the terrain
    return createTerrain();
}

//------------------------------------------------------------------------------
bool TerrainTexture3D::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    if (!m_vao.draw(Mode::TRIANGLE_STRIP, 0, m_nbVertices))
    {
        std::cerr << "Terrain not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool TerrainTexture3D::createTerrain()
{
    const uint32_t dim = 40;

    if (!m_prog.bind(m_vao))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    // Load all 2D textures into a single big 3D texture
    m_vao.texture3D("tex3d").wrap(GLTexture::Wrap::CLAMP_TO_BORDER);
    if (!m_vao.texture3D("tex3d").load<SOIL>({
            "external/assets/deep_water.png",
            "external/assets/shallow_water.png",
            "external/assets/shore.png",
            "external/assets/fields.png",
            "external/assets/rocks.png",
            "external/assets/snow.png",
        }))
    {
        return false;
    }

    // Create the terrain
    generateAltitudes(dim);
    loadTerrain(dim);
    return true;
}

//------------------------------------------------------------------------------
//! \brief Generate terrain altitude.
//------------------------------------------------------------------------------
void TerrainTexture3D::generateAltitudes(const uint32_t dim)
{
    // Create random values
    m_altitudes.resize(dim * dim);

    for (uint32_t r = 0; r < dim * dim; ++r)
    {
        m_altitudes[r] = fabsf(float(rand())) / float(RAND_MAX);
    }

    // Generate smooth m_altitudes values
    std::vector<float> smoothTerrain(dim * dim);
    for (unsigned k = 0; k < 5; ++k)
    {
        float maxVal = 0.0f;
        float minVal = 1.0f;
        for (uint32_t x = 0; x < dim; ++x)
        {
            for (uint32_t y = 0; y < dim; ++y)
            {
                if (x == 0 || x == dim - 1)
                    m_altitudes[x * dim + y] = 0.0f;
                else if (y == 0 || y == dim - 1)
                    m_altitudes[x * dim + y] = 0.0f;
                else
                {
                    float a = 0.0f;
                    uint32_t counter = 0;
                    for (uint32_t s = 0; s <= 2; ++s)
                    {
                        for (uint32_t r = 0; r <= 2; ++r)
                        {
                            a += m_altitudes[(x + s - 1) * dim + (y + r - 1)];
                            ++counter;
                        }
                    }
                    float val = a / float(counter);
                    smoothTerrain[x * dim + y] = val;
                    if (val > maxVal)
                        maxVal = val;
                    if (val < minVal)
                        minVal = val;
                }
            }
        }
        for (uint32_t r = 0; r < dim * dim; ++r)
        {
            m_altitudes[r] = (smoothTerrain[r] - minVal) / (maxVal - minVal);
        }
    }
}

//------------------------------------------------------------------------------
//! \brief Fill VBOs with mesh position and 3d texture position.
//------------------------------------------------------------------------------
void TerrainTexture3D::loadTerrain(const uint32_t dim)
{
    float maxHeight = 0.2f;
    float texHeight = 0.9f;

    // Cache references to avoid calling the hash function during the loop
    auto& pos = m_vao.vector3f("position");
    auto& uv = m_vao.vector3f("UV");

    // Reserve VBO sizes (optional but avoid to reallocate several times)
    // FIXME: pass m_nbVertices when binding VAO ?
    m_nbVertices = (dim - 1) * (dim - 1) * 4;
    pos.reserve(m_nbVertices);
    uv.reserve(m_nbVertices);

    for (unsigned x = 1; x < dim; ++x)
    {
        for (unsigned y = 1; y < dim; ++y)
        {
            // Texture3D
            uv.append(
                Vector3f(float(x - 1) / float(dim),
                         float(y - 1) / float(dim),
                         m_altitudes[(x - 1) * dim + (y - 1)] * texHeight));
            uv.append(Vector3f(float(x) / float(dim),
                               float(y - 1) / float(dim),
                               m_altitudes[x * dim + (y - 1)] * texHeight));
            uv.append(Vector3f(float(x - 1) / float(dim),
                               float(y) / float(dim),
                               m_altitudes[(x - 1) * dim + y] * texHeight));
            uv.append(Vector3f(float(x) / float(dim),
                               float(y) / float(dim),
                               m_altitudes[x * dim + y] * texHeight));

            // Meshes
            pos.append(
                Vector3f(float(x - 1) / float(dim) - 0.5f,
                         float(y - 1) / float(dim) - 0.5f,
                         m_altitudes[(x - 1) * dim + (y - 1)] * maxHeight));
            pos.append(Vector3f(float(x) / float(dim) - 0.5f,
                                float(y - 1) / float(dim) - 0.5f,
                                m_altitudes[x * dim + (y - 1)] * maxHeight));
            pos.append(Vector3f(float(x - 1) / float(dim) - 0.5f,
                                float(y) / float(dim) - 0.5f,
                                m_altitudes[(x - 1) * dim + y] * maxHeight));
            pos.append(Vector3f(float(x) / float(dim) - 0.5f,
                                float(y) / float(dim) - 0.5f,
                                m_altitudes[x * dim + y] * maxHeight));
        }
    }
}

//------------------------------------------------------------------------------
void TerrainTexture3D::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the onSetup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void TerrainTexture3D::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
