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

#include "Scene/CameraRigNode.hpp"
#include "UI/Window.hpp"

//------------------------------------------------------------------------------
CameraRigNode::CameraRigNode(std::string const& name, Camera& camera)
    : SceneObject(name)
{
    m_fov = 0.785398f;
    control(camera);
}

//------------------------------------------------------------------------------
void CameraRigNode::control(Camera& camera)
{
    m_camera = &camera;
    m_newTransform = camera.transform;
}

//------------------------------------------------------------------------------
void CameraRigNode::onUpdate(float const dt)
{
    handleMouseInput();
    handleKeyBoardInput();
    handleMovementInput(dt);
}

//------------------------------------------------------------------------------
void CameraRigNode::handleMouseInput()
{
    float y = static_cast<float>(GLWindow::mouse().scroll.y);
    if (y != 0.0f)
    {
        m_newZoom += y * zoomAmount;
    }
}

//------------------------------------------------------------------------------
void CameraRigNode::handleKeyBoardInput()
{
    float speed =
        GLWindow::isKeyDown(GLFW_KEY_LEFT_SHIFT) ? fastSpeed : normalSpeed;

    if (GLWindow::isKeyDown(GLFW_KEY_UP))
    {
        // std::cout << "UP avant " << m_newTransform.position()
        //           << " + " << speed << " * " << transform.forward() <<
        //           std::endl;
        m_newTransform.position(m_newTransform.position() +
                                speed * transform.forward());
        // std::cout << "UP apres " << m_newTransform.position() << std::endl;
    }
    else if (GLWindow::isKeyDown(GLFW_KEY_DOWN))
    { // std::cout << "DW" << std::endl;
        m_newTransform.position(m_newTransform.position() -
                                speed * transform.forward());
    }

    if (GLWindow::isKeyDown(GLFW_KEY_RIGHT))
    { // std::cout << "RI" << std::endl;
        m_newTransform.position(m_newTransform.position() +
                                speed * transform.right());
    }
    else if (GLWindow::isKeyDown(GLFW_KEY_LEFT))
    { // std::cout << "LE" << std::endl;
        m_newTransform.position(m_newTransform.position() -
                                speed * transform.right());
    }

    if (GLWindow::isKeyDown(GLFW_KEY_F1))
    {
        m_newZoom += zoomAmount;
    }
    else if (GLWindow::isKeyDown(GLFW_KEY_F2))
    {
        m_newZoom -= zoomAmount;
    }
}

//------------------------------------------------------------------------------
void CameraRigNode::handleMovementInput(float const dt)
{
    /*  m_camera->transform.position(
        vector::lerp(m_camera->transform.position(),
                     m_newTransform.position(),
                     dt * movementTime));
    */
    m_camera->transform.position(m_newTransform.position());
    // std::cout << "camera " << m_camera->transform.position() << std::endl;

    m_fov = maths::lerp(m_fov, m_newZoom, dt * movementTime);
    m_camera->perspective.setFieldOfView(
        units::angle::radian_t(m_fov)); // TODO ortho
    // std::cout << "Zoom " << m_fov << std::endl;
}
