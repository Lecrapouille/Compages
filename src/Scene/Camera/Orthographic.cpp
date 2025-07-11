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

#include "Scene/Camera/Orthographic.hpp"
#include <iostream>

//------------------------------------------------------------------------------
Orthographic::Orthographic(float const left,
                           float const right,
                           float const bottom,
                           float const top,
                           float const near,
                           float const far)
    : m_left(left),
      m_right(right),
      m_bottom(bottom),
      m_top(top),
      m_near(near),
      m_far(far),
      m_aspect(1.0f),
      m_frustum_size(1.0f)
{
    m_dirty = true;
}

//------------------------------------------------------------------------------
Matrix44f const& Orthographic::matrix()
{
    if (m_dirty)
    {
        m_projection = matrix::ortho(m_left * m_frustum_size * m_aspect,
                                     m_right * m_frustum_size * m_aspect,
                                     m_bottom * m_frustum_size * m_aspect,
                                     m_top * m_frustum_size * m_aspect,
                                     m_near * m_frustum_size * m_aspect,
                                     m_far * m_frustum_size * m_aspect);
        m_dirty = false;
    }

    return m_projection;
}

//------------------------------------------------------------------------------
void Orthographic::setPlanes(float const left,
                             float const right,
                             float const bottom,
                             float const top,
                             float const near,
                             float const far)
{
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    m_near = near;
    m_far = far;
    m_dirty = true;
}

//------------------------------------------------------------------------------
void Orthographic::setFrustumSize(float const size)
{
    m_frustum_size = std::abs(size);
}

//------------------------------------------------------------------------------
void Orthographic::setAspect(float const width, float const height)
{
    m_aspect = width / height;
    m_dirty = true;
}
