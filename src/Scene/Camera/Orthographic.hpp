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

#ifndef SCENE_CAMERA_ORTHOGRAPHIC_HPP
#define SCENE_CAMERA_ORTHOGRAPHIC_HPP

#include "Components/Components.hpp"
#include "Math/Transformable.hpp"

// *****************************************************************************
//! \brief Define orthographic computation needed for the Camera
// *****************************************************************************
class Orthographic: public Component
{
public:

    //--------------------------------------------------------------------------
    //! \brief Initialize states with good default values.
    //--------------------------------------------------------------------------
    Orthographic(float const left = -1.0f,
                 float const right = 1.0f,
                 float const bottom = -1.0f,
                 float const top = 1.0f,
                 float const near = -1.0f,
                 float const far = 1.0f);

    //--------------------------------------------------------------------------
    //! \brief Initialize orthographic projection.
    //! Aspect ratio is determined in updateProjection() from current viewport
    //! dimensions, left and right borders are adjusted to comply with m_aspect
    //! ratio.
    //!
    //! \param left left border
    //! \param right right border
    //! \param bottom bottom border
    //! \param top top border
    //! \param near near distance
    //! \param far far distance
    //--------------------------------------------------------------------------
    void setPlanes(float const left = -1.0f,
                   float const right = 1.0f,
                   float const bottom = -1.0f,
                   float const top = 1.0f,
                   float const near = -1.0f,
                   float const far = 1.0f);

    //--------------------------------------------------------------------------
    //! \brief Set the screen dimension
    //! \param[in] width: screen width
    //! \param[in] height: screen height
    //--------------------------------------------------------------------------
    void setAspect(float const width, float const height);

    void setFrustumSize(float const size);

    //--------------------------------------------------------------------------
    //! \brief Compute if needed and return the projection matrix.
    //! \retun The Matrix 4x4 of float.
    //--------------------------------------------------------------------------
    Matrix44f const& matrix();

protected:

    Matrix44f m_projection;
    float m_left;
    float m_right;
    float m_bottom;
    float m_top;
    float m_near;
    float m_far;
    float m_aspect;
    float m_frustum_size;
    bool m_dirty;
};

#endif // SCENE_CAMERA_ORTHOGRAPHIC_HPP
