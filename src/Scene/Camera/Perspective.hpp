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

#ifndef SCENE_CAMERA_PERSPECTIVE_HPP
#define SCENE_CAMERA_PERSPECTIVE_HPP

#include "Components/Components.hpp"
#include "Math/Transformable.hpp"

using namespace units::literals;

// *****************************************************************************
//! \brief Define perspective computation needed for the Camera
// *****************************************************************************
class Perspective: public Component
{
public:

    //--------------------------------------------------------------------------
    //! \brief Initialize states with good default values.
    //!
    //! \param[in] fov: Field of View of the camera (in degrees).
    //! \param[in] near:
    //! \param[in] far:
    //--------------------------------------------------------------------------
    Perspective(units::angle::radian_t const fov = 45.0_deg,
                float const near = 0.1f,
                float const far = 1000.0f);

    //--------------------------------------------------------------------------
    //! \brief set the field of view of the camera.
    //! \param[in] fov: Field of View of the camera (in degrees).
    //--------------------------------------------------------------------------
    void setFieldOfView(units::angle::radian_t const fov = 45.0_deg);

    //--------------------------------------------------------------------------
    //! \brief
    //! \param[in] near:
    //! \param[in] far:
    //--------------------------------------------------------------------------
    void setClipping(float const near = 0.1f, float const far = 1000.0f);

    //--------------------------------------------------------------------------
    //! \brief Set the screen dimension
    //! \param[in] width: screen width
    //! \param[in] height: screen height
    //--------------------------------------------------------------------------
    void setAspect(float const width, float const height);

    //--------------------------------------------------------------------------
    //! \brief Compute if needed and return the projection matrix.
    //! \retun The Matrix 4x4 of float.
    //--------------------------------------------------------------------------
    Matrix44f const& matrix();

protected:

    Matrix44f m_projection;
    units::angle::radian_t m_fov;
    float m_near, m_far, m_aspect;
    bool m_dirty;
};

#endif // SCENE_CAMERA_PERSPECTIVE_HPP
