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

#ifndef CAMERA_NODE_HPP
#define CAMERA_NODE_HPP

#include "Scene/Camera/Orthographic.hpp"
#include "Scene/Camera/Perspective.hpp"
#include "Scene/SceneTree.hpp"

// *****************************************************************************
//! \brief Define a basic 3D Camera that can be stored inside a scene tree.
// *****************************************************************************
class Camera: public SceneObject
{
public:

    enum class Type
    {
        ORTHOGRAPHIC,
        PERSPECTIVE
    };

    //--------------------------------------------------------------------------
    //! \brief Initialize camera state with a given name and select a type of
    //! projection (perspective (by default) or orthographic). The view port is
    //! set to (x,y,w,h) = (0,0,1,1). Matrices for projection and view are set
    //! to identity. The camera position is placed to (1,0,0) looks at (0,0,0)
    //! and its normal is (0,1,0).
    //!
    //! \param[in] name: the name of the camera.
    //! \param[in] mode: swith to orthographic or perspective mode.
    //--------------------------------------------------------------------------
    Camera(std::string const& name, Type const type = Type::PERSPECTIVE);

    //--------------------------------------------------------------------------
    //! \brief Swith the type of projection (perspective or orthographic).
    //!
    //! \param[in] type: swith to Orthographic or Perspective mode.
    //! \return the projection matrix of the new type of view.
    //--------------------------------------------------------------------------
    Matrix44f const& is(Type const type);

    //--------------------------------------------------------------------------
    //! \brief Define what region is shown on screen (GLWindow class) for
    //! displaying the 3D scene. The view is defined by the coordinate of the
    //! top-left corner and its dimension (width and height). Values of the view
    //! shall be within the range [0 .. 1] to be independent of the screen
    //! dimension. For example setViewPort(0.25f, 0.25f, 0.75f, 0.75f) with a
    //! screen of dimension 800 x 600 will give a window with the following
    //! coordinates: top-left corner (0.25 * 800, 0.25 * 600) and bottom-right
    //! corner (0.75 * 800, 0.75 * 600).
    //!
    //! \param[in] x: X-coordinate of the top-left corner of the view port.
    //!   Value shall be >= 0 and < 1.
    //! \param[in] y: Y-coordinate of the top-left corner of the view port.
    //!   Value shall be >= 0 and < 1.
    //! \param[in] w: the width of the view port. Value shall be > 0 and <= 1.
    //! \param[in] y: the height of the view port. Value shall be > 0 and <= 1.
    //!
    //! \return true if inputs are within [0 .. 1], else return false.
    //--------------------------------------------------------------------------
    bool setViewPort(float x, float y, float w, float h);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline Vector4f const& getViewPort() const
    {
        return m_viewport;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the projection matrix. This method return the matrix that
    //! shall have been computed by the method updateProjectionMatrix(). The
    //! returned matrix can be directly be applied to OpenGL GLSL shader
    //! (uniform variable).
    //!
    //! To be called when the screen (GLWindow class) has been resized.
    //!
    //! \param[in] width: new screen width.
    //! \param[in] height: new screen height.
    //! \retun The Matrix 4x4 of float.
    //--------------------------------------------------------------------------
    Matrix44f const& projection(float const width, float const height);
    Matrix44f const& projection();

    //--------------------------------------------------------------------------
    //! \brief Return the view transformation matrix according to its position
    //! and orientation. This method return the matrix that shall have been
    //! computed by the method updateViewMatrix(). The returned matrix can be
    //! directly be applied to OpenGL GLSL shader (uniform variable).
    //!
    //! \retun The Matrix 4x4 of float.
    //--------------------------------------------------------------------------
    Matrix44f const& view();

public:

    //! \brief Reference to the component applying the perspective projection.
    Perspective& perspective;
    //! \brief Reference to the component applying the orthographic projection.
    Orthographic& orthographic;

protected:

    //! \brief Current type of projection (perspective or orthographic).
    Type m_type;
    //! \brief The region shown on screen
    Vector4f m_viewport;
    //! \brief View matrix
    Matrix44f m_view;
    //! \brief Memorize the screen dimension
    float m_width, m_height;
};

#endif
