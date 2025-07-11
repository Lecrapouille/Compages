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

#include "Scene/Camera/CameraNode.hpp"

//------------------------------------------------------------------------------
Camera::Camera(std::string const& name, Type const type)
    : SceneObject(name),
      perspective(components.addComponent<Perspective>(/*"Perspective"*/)),
      orthographic(components.addComponent<Orthographic>(/*"Orthographic"*/)),
      m_type(type),
      m_viewport(0.0f, 0.0f, 1.0f, 1.0f)
{
    transform.lookAt(Vector3f::RIGHT, Vector3f::ZERO, Vector3f::UP);
}

//------------------------------------------------------------------------------
Matrix44f const& Camera::is(Type const type)
{
    m_type = type;
    if (m_type == Type::PERSPECTIVE)
    {
        return perspective.matrix();
    }
    else
    {
        return orthographic.matrix();
    }
}

//------------------------------------------------------------------------------
bool Camera::setViewPort(float x, float y, float w, float h)
{
    if (!(x >= 0.0f) && (x < 1.0f))
    {
        std::cerr << "Top-left x coordinate shall be within [0 .. 1["
                  << std::endl;
        return false;
    }

    if (!(x >= 0.0f) && (y < 1.0f))
    {
        std::cerr << "Top-left y coordinate shall be within [0 .. 1["
                  << std::endl;
        return false;
    }

    if (!(w > 0.0f) && (w <= 1.0f))
    {
        std::cerr << "Width shall be within ]0 .. 1]" << std::endl;
        return false;
    }

    if (!(h > 0.0f) && (h <= 1.0f))
    {
        std::cerr << "Height shall be within ]0 .. 1]" << std::endl;
        return false;
    }

    if (x + w > 1.0f)
    {
        std::cerr << "Top-left x coordinate + width shall be <= 1" << std::endl;
        return false;
    }

    if (y + h > 1.0f)
    {
        std::cerr << "Top-left y coordinate + height shall be <= 1"
                  << std::endl;
        return false;
    }

    m_viewport[0] = x;
    m_viewport[1] = y;
    m_viewport[2] = w;
    m_viewport[3] = h;

    return true;
}

//------------------------------------------------------------------------------
Matrix44f const& Camera::projection(float const width, float const height)
{
    perspective.setAspect(width, height);
    orthographic.setAspect(width, height);
    return projection();
}

//------------------------------------------------------------------------------
Matrix44f const& Camera::projection()
{
    if (m_type == Type::PERSPECTIVE)
    {
        return perspective.matrix();
    }
    else
    {
        return orthographic.matrix();
    }
}

//------------------------------------------------------------------------------
Matrix44f const& Camera::view()
{
    // FIXME idea to avoid computing it everytimes
    // if (transform.dirty()) {

    // Negative translation because when camera is moving we move world in
    // the opposite direction. For more explanaitions see this document:
    // https://research.ncl.ac.uk/game/mastersdegree/graphicsforgames/
    // Chapiter The View Matrix
    Matrix44f I(matrix::Identity);
    Matrix44f transl = matrix::translate(I, -transform.position());

    // FIXME shall be rotation * translation but glm seems to inverse matrix
    // prodcut: A * B is made as B * A
    // https://stackoverflow.com/questions/18151845/converting-glmlookat-matrix-to-quaternion-and-back
    m_view = transl * transform.rotation();

    // } // if (transform.dirty())

    return m_view;
}
