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

#ifndef EXAMPLE_04_SCENEGRAPH_HPP
#define EXAMPLE_04_SCENEGRAPH_HPP

#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

using namespace glwrap;

// *****************************************************************************
//! \brief This class allows to display a GUI for debuging a scene
//! graph attached to it.
// *****************************************************************************
class GUI: public DearImGui
{
public:

    GUI() {}

    //------------------------------------------------------------------
    //! \brief Attach a scene graph for its monotoring
    //------------------------------------------------------------------
    inline void observeGraph(Node3D_SP const& scene)
    {
        m_scene = scene;
    }

protected:

    //------------------------------------------------------------------
    //! \brief override DearImGui method: debug the scenegraph
    //------------------------------------------------------------------
    virtual bool render() override;

    //------------------------------------------------------------------
    //! \brief Iterate on scene nodes for their display.
    //------------------------------------------------------------------
    void observeNode(Node3D const& node); // const;

private:

    Node3D_SP m_scene = nullptr;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
class MyCube: public Node3D
{
public:

    MyCube(const char* name);

    //! \brief Create a home-made cube
    static std::shared_ptr<MyCube> create(const char* name)
    {
        return std::make_shared<MyCube>(name);
    }

    //! \brief Render the cube
    virtual void draw(Matrix44f const& modelMatrix) override;

private:

    GLProgram m_prog;
    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;
    GLVAO m_vao;
};

DECLARE_CLASS(CubicRobot);

// *****************************************************************************
//! \brief a CubicRobot is an robot made of cubes. A CubicRobot is a
//! node of the scene graph. A robot is made of a head, body, legs
//! and arms which are also a node of the scene graph. To make it
//! simple we create a single 3D cube model for each nodes but feel
//! free to add more complex 3D objects.
// *****************************************************************************
class CubicRobot: public Node3D
{
public:

    CubicRobot(const char* name);
    ~CubicRobot();

    //! \brief Create a new Robot
    static CubicRobot_SP create(const char* name)
    {
        return std::make_shared<CubicRobot>(name);
    }

    //! \brief Animation of the robot
    virtual void doUpdate(float const dt) override;

private:

    Node3D_SP m_cube;
    Node3D_SP m_body;
    Node3D_SP m_head;
    Node3D_SP m_leftArm;
    Node3D_SP m_rightArm;
    Node3D_SP m_leftLeg;
    Node3D_SP m_rightLeg;
    float radiansRotated = 0.0f;
};

// *****************************************************************************
//! \brief Display a scere graphe made of 3 moving robots. Each robots
//! are nodes of the scere graph. Each element of robots is also a
//! part of the scene graph.
// *****************************************************************************
class RobotsSceneGraph: public IGLWindow
{
public:

    RobotsSceneGraph() : m_camera(Camera3D::Type::PERSPECTIVE) {}

    ~RobotsSceneGraph() {}

private:

    virtual void onWindowSizeChanged() override;
    virtual bool setup() override;
    virtual bool draw() override;

private:

    CameraController m_camera;
    Node3D_SP m_scene;
    GUI m_imgui;
};

#endif // EXAMPLE_04_SCENEGRAPH_HPP
