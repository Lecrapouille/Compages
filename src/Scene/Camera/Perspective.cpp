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

#include "Scene/Camera/Perspective.hpp"
#include "Math/Transformation.hpp"

//------------------------------------------------------------------------------
Perspective::Perspective(units::angle::radian_t const fov,
                         float const near,
                         float const far)
    : m_projection(matrix::Identity), m_near(near), m_far(far), m_aspect(0.5f)
{
    setFieldOfView(fov);
}

//------------------------------------------------------------------------------
Matrix44f const& Perspective::matrix()
{
    if (m_dirty)
    {
        m_projection = matrix::perspective(m_fov, m_aspect, m_near, m_far);
        m_dirty = false;
    }

    return m_projection;
}

//------------------------------------------------------------------------------
void Perspective::setFieldOfView(units::angle::radian_t const fov)
{
    // Constrain between 1 deg and 179 deg
    float angle = maths::clamp(fov.to<float>(), 0.0174533f, 3.12414f);
    m_fov = units::angle::radian_t(angle);
    m_dirty = true;
}

//------------------------------------------------------------------------------
void Perspective::setClipping(float const near, float const far)
{
    m_near = near;
    m_far = far;
    m_dirty = true;
}

//------------------------------------------------------------------------------
void Perspective::setAspect(float const width, float const height)
{
    m_aspect = width / height;
    m_dirty = true;
}
