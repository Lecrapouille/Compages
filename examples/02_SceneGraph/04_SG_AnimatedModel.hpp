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

#ifndef EXAMPLE_04_SG_ANIMATED_MODEL_HPP
#define EXAMPLE_04_SG_ANIMATED_MODEL_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

// *****************************************************************************
//! \brief This example shows how to apply the depth material to a 3D shape.
// *****************************************************************************
class SGAnimatedModel: public GLWindow
{
public:

    SGAnimatedModel(uint32_t const width,
                    uint32_t const height,
                    const char* title);
    ~SGAnimatedModel();

    static std::string info()
    {
        return "Show an animted model";
    }

private:

    virtual bool onSetup() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual bool onPaint() override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

    Camera m_camera;
    AnimatedModel::Ptr m_model;
    SceneTree m_scene;
};

#endif // EXAMPLE_04_SG_ANIMATED_MODEL_HPP
