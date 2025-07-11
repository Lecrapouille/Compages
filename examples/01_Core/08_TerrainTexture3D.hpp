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

#ifndef EXAMPLE_08_TERRAIN_TEXTURE3D_HPP
#define EXAMPLE_08_TERRAIN_TEXTURE3D_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \file this example paints a 3d lanscape from a 3d texture.
//!
//! \note This example takes its inspiration from
//! http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/graphics_7_1_eng_web.html#1
//! and:
//! https://www.uni-marburg.de/fb12/en/researchgroups/grafikmultimedia/lectures/graphics
//------------------------------------------------------------------------------
class TerrainTexture3D: public GLWindow
{
public:

    TerrainTexture3D(uint32_t const width,
                     uint32_t const height,
                     const char* title);
    ~TerrainTexture3D();

    static std::string info()
    {
        return "Render a terrain built from a texture 3D";
    }

private:

    bool createTerrain();
    void generateAltitudes(const uint32_t dim);
    void loadTerrain(const uint32_t dim);

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;
    GLVAO m_vao;
    GLProgram m_prog;
    std::vector<float> m_altitudes;
    uint32_t m_nbVertices = 0;
};

#endif // EXAMPLE_08_TERRAIN_TEXTURE3D_HPP
