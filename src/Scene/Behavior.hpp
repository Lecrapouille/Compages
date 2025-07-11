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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_BEHAVIOR_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_BEHAVIOR_HPP

class Camera;

// *****************************************************************************
//! \brief Give some behavior for a game objects.
//! Inspired by Unity's MonoBehavior class.
// *****************************************************************************
class Behavior
{
public:

    virtual ~Behavior() = default;

    //--------------------------------------------------------------------------
    //! \brief Calback triggered when the object becomes enabled and active.
    //--------------------------------------------------------------------------
    virtual void onEnable() {}

    //--------------------------------------------------------------------------
    //! \brief Calback triggered when the object becomes disabled.
    //--------------------------------------------------------------------------
    virtual void onDisable() {}

    //--------------------------------------------------------------------------
    //! \brief Calback triggered just when the scene is loaded. Used it for
    //! configuring or restoring internal states of the object (setting,
    //! config).
    //! \return true when the setup succeeded, false on failure.
    //--------------------------------------------------------------------------
    virtual bool onSetup()
    {
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Callback triggered when visiting the scene for updating
    //! transformation matrices. This method does nothing by default but can be
    //! overrided by TreeNode3D's derived classes for animating/moving the
    //! current node (which will impact on position of descendant nodes).
    //!
    //! \note: the project https://github.com/vahlers/scg3 traverse the tree
    //! using the vistor design pattern. I did not follow because I am a big fan
    //! of visitors: in my personal opinion they make the code harder to
    //! understand. In the course https://research.ncl.ac.uk/game/mastersdegree/
    //! graphicsforgames/scenegraphs/Tutorial%206%20-%20Scene%20Graphs.pdf
    //! they let derived classes override the method update() but suppose the
    //! developper will call the method update() of the parent class. I think
    //! this is an error prone pratice. I think doUpdate() does the same thing
    //! in a more simpler way for the developper.
    //!
    //! \param[in] dt: delta time from the previous update.
    //--------------------------------------------------------------------------
    virtual void onUpdate(float const /*dt*/) {}

    //--------------------------------------------------------------------------
    //! \brief Callback triggered when drawing the scene.
    //! \param[in] modelMatrix: Model matrix.
    //! \param return true if paint succeeded.
    //--------------------------------------------------------------------------
    virtual bool onDraw(Matrix44f const& /*modelMatrix*/)
    {
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Callback triggered when the camera has moved.
    //! \param[in] camera.
    //--------------------------------------------------------------------------
    virtual void onCameraUpdated(Camera& /*camera*/) {}
};

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_BEHAVIOR_HPP
