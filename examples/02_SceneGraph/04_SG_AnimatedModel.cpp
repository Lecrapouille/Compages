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

#include "04_SG_AnimatedModel.hpp"
#include <iostream>

//------------------------------------------------------------------------------
SGAnimatedModel::SGAnimatedModel(uint32_t const width,
                                 uint32_t const height,
                                 const char* title)
    : GLWindow(width, height, title), m_camera("camera")
{
    std::cout << "Hello SGAnimatedModel: " << info() << std::endl;
}

//------------------------------------------------------------------------------
SGAnimatedModel::~SGAnimatedModel()
{
    std::cout << "Bye SGAnimatedModel" << std::endl;
}

//------------------------------------------------------------------------------
void SGAnimatedModel::onWindowResized()
{
    m_camera.projection(width<float>(), height<float>());
}

//------------------------------------------------------------------------------
bool SGAnimatedModel::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    // glCheck(glDepthFunc(GL_LESS));
    // glCheck(glEnable(GL_BLEND));
    // glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_model = AnimatedModel::create<AnimatedModel>(
        "/home/qq/MyGitHub/OpenGLCppWrapper/examples/external/assets/"
        "cowboy.json");
    m_model->getAnimator().startAnimation("");
    m_scene.root = std::move(m_model);

    m_camera.transform.lookAt(
        Vector3f(0.0, -15.0, 5.0), Vector3f(0, 0, 0), Vector3f(0, 1, 0));
    m_camera.is(Camera::Type::PERSPECTIVE);

    // m_scene.setup();

    return true;
}

//------------------------------------------------------------------------------
//! \brief Renderer our shape
//------------------------------------------------------------------------------
bool SGAnimatedModel::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    m_scene.update(dt());
    m_scene.draw(m_camera);

    return true;
}

void SGAnimatedModel::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

void SGAnimatedModel::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
