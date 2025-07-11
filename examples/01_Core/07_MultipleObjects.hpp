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

#ifndef EXAMPLE_07_MULTIPLE_MOVING_OBJECTS_HPP
#define EXAMPLE_07_MULTIPLE_MOVING_OBJECTS_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief This example show differences between drawing multiple VAOs versus
//! drawing multiple VBOs. This example also shows diiferents way to draw an
//! object.
//------------------------------------------------------------------------------
class MultipleObjects: public GLWindow
{
public:

    struct Shape
    {
        Shape(std::string const& name) : vao(name) {}

        GLVAO vao;

        //! \brief Allow to specify and combine several transformation
        //! (translation, scaling, rotation) and get the transformation matrix
        //! 4x4 to apply to the shader.
        //! \note you can use the alias Transformable3D
        Transformable<float, 3U> transform;
    };

public:

    MultipleObjects(uint32_t const width,
                    uint32_t const height,
                    const char* title);
    ~MultipleObjects();

    static std::string info()
    {
        return "Render multiple moving objects";
    }

private:

    bool initCube(Shape& cube, const char* texture_path);
    bool initFloor(Shape& floor, const char* texture_path);
    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;
    Shape m_cube1;
    Shape m_cube2;
    Shape m_floor;
    GLProgram m_prog;
};

#endif // EXAMPLE_07_MULTIPLE_MOVING_OBJECTS_HPP
