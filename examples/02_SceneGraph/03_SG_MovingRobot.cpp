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

#include "04_SceneGraph.hpp"
#include <sstream>

//------------------------------------------------------------------------------
//! \file Display a scene graphe made of 3 animated robots. Each robots are
//! nodes of the scene graph. Each element of robots is also a part of the scene
//! graph.
//------------------------------------------------------------------------------

MyCube::MyCube(const char* name)
    : Node3D(name, true), m_prog("GLProgram"), m_vao("VAO_cube")
{
    // Load from ASCII file the vertex sahder (vs) as well the fragment shader
    m_vertex_shader.fromFile("02_Scene/shaders/04_SceneGraph.vs");
    m_fragment_shader.fromFile("02_Scene/shaders/04_SceneGraph.fs");

    // Compile shader as OpenGL program. This one will instanciate all OpenGL
    // objects for you.
    if (m_prog.attachShaders(m_vertex_shader, m_fragment_shader).compile())
    {
        // Init shader uniforms
        m_prog.vector4f("color") = Vector4f(0.2f, 0.2f, 0.2f, 0.2f);

        // Mandatory: bind VAO to program to get
        // it populated of VBOs.
        m_prog.bind(m_vao);

        // Fill the VBO for vertices
        m_vao.vector3f("position") = {
#include "geometry/cube_position.txt"
        };

        // We do not want a cube centered to (0,0,0).
        m_vao.vector3f("position") += Vector3f(0.0f, 1.0f, 0.0f);

        // Fill the VBO for texture coordiantes
        m_vao.vector2f("UV") = {
#include "geometry/cube_texture.txt"
        };

        // Create the texture
        m_vao.texture2D("texID").interpolation(TextureMinFilter::LINEAR,
                                               TextureMagFilter::LINEAR);
        m_vao.texture2D("texID").wrap(TextureWrap::CLAMP_TO_EDGE);
        m_vao.texture2D("texID").load<SOIL>("external/assets/wooden-crate.jpg");

        float ratio =
            1024.0f / 728.0f; // FIXME width<float>() / height<float>();
        m_prog.matrix44f("projection") =
            matrix::perspective(maths::toRadian(60.0f), ratio, 0.1f, 10000.0f);
        m_prog.matrix44f("view") = matrix::lookAt(
            Vector3f(0.0f, 10.0f, 100.0f), Vector3f(30), Vector3f(0, 1, 0));
    }
    else
    {
        std::cerr << "failed compiling OpenGL program. Reason was '"
                  << m_prog.getError() << "'" << std::endl;
    }
}

//------------------------------------------------------------------------------
void MyCube::draw(Matrix44f const& modelMatrix)
{
    m_prog.matrix44f("model") = modelMatrix;
    m_prog.draw(m_vao, Mode::TRIANGLES, 0, 36);
}

//------------------------------------------------------------------------------
CubicRobot::CubicRobot(const char* name) : Node3D(name, true)
{
    DEBUG("Create CubicRobot %s", name);

    // Body
    m_body = MyCube::create("Body");
    m_body->localScale(Vector3f(10.0f, 15.0f, 5.0f));
    m_body->position(Vector3f(0.0f, 35.0f, 0.0f));

    // Head
    m_head = MyCube::create("Head");
    m_head->localScale(Vector3f(5.0f));
    m_head->position(Vector3f(0.0f, 30.0f, 0.0f));

    // Left arm
    m_leftArm = MyCube::create("LeftArm");
    m_leftArm->localScale(Vector3f(3.0f, -18.0f, 3.0f));
    m_leftArm->position(Vector3f(-12.0f, 30.0f, -1.0f));

    // Right arm
    m_rightArm = MyCube::create("RightArm");
    m_rightArm->localScale(Vector3f(3.0f, -18.0f, 3.0f));
    m_rightArm->position(Vector3f(12.0f, 30.0f, -1.0f));

    // Left leg
    m_leftLeg = MyCube::create("LeftLeg");
    m_leftLeg->localScale(Vector3f(3.0f, -17.5f, 3.0f));
    m_leftLeg->position(Vector3f(-8.0f, 0.0f, 0.0f));

    // Right leg
    m_rightLeg = MyCube::create("RightLeg");
    m_rightLeg->localScale(Vector3f(3.0f, -17.5f, 3.0f));
    m_rightLeg->position(Vector3f(8.0f, 0.0f, 0.0f));

    // Robot
    m_body->attach(m_head);
    m_body->attach(m_leftArm);
    m_body->attach(m_rightArm);
    m_body->attach(m_leftLeg);
    m_body->attach(m_rightLeg);
    attach(m_body);

    // m_body->debug();
}

CubicRobot::~CubicRobot()
{
    DEBUG("Destroy CubicRobot %s", name().c_str());
}

//------------------------------------------------------------------------------
//! \brief Move element of the robot body
//------------------------------------------------------------------------------
void CubicRobot::doUpdate(float const dt)
{
    DEBUG("%s", "Robot::update");

    // Speed: 36 degrees per second
    const float radiansPerSecond = maths::toRadian(36.0f);
    radiansRotated = dt * radiansPerSecond;
    radiansRotated = maths::wrapToPI(radiansRotated);

    rotateY(radiansRotated);
    m_head->rotateY(-radiansRotated);
    m_leftArm->rotateX(-radiansRotated);
    m_rightArm->rotateX(radiansRotated);
}

//------------------------------------------------------------------------------
//! \brief Paint the GUI
//------------------------------------------------------------------------------
void GUI::observeNode(Node3D const& node) // const
{
    std::string nodename("Node '" + node.name() + "'");
    if (ImGui::TreeNode(nodename.c_str()))
    {
        // Renderable = contains a mesh to be rendered.
        if (node.isRenderable())
        {
            std::string name("Not for rendering");
            ImGui::TextUnformatted(name.c_str());
        }
        else
        {
            ImGui::Text("Renderable");
        }

        ImGui::Text("Transf. Matrix:");
        std::stringstream ss;
        ss << node.worldTransform();
        ImGui::TextUnformatted(ss.str().c_str());

        ss.str("");
        ss << "Has " << node.children().size() << " child nodes:";
        if (ImGui::TreeNode(ss.str().c_str()))
        {
            for (auto const& i : node.children())
            {
                observeNode(*i);
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
}

//------------------------------------------------------------------------------
//! \brief Paint the GUI
//------------------------------------------------------------------------------
bool GUI::render()
{
    ImGui::SetNextTreeNodeOpen(true);
    ImGui::Begin("Scene graph");
    ImGui::SetNextItemOpen(true);
    observeNode(*m_scene);
    ImGui::End();

    return true;
}

//------------------------------------------------------------------------------
//! \brief Callback when the window changed its size.
//------------------------------------------------------------------------------
void RobotsSceneGraph::onWindowSizeChanged()
{
    // Make sure the viewport matches the new window dimensions.
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    // TODO
    // m_prog.matrix44f("projection") =
    //   m_camera.camera().updateProjectionMatrix();
}

//------------------------------------------------------------------------------
//! Create the cene graph made of 3 robots. Each robot is a scene node.
//------------------------------------------------------------------------------
bool RobotsSceneGraph::setup()
{
    DEBUG("%s", "Create graph scene");

    // Init the context of the DearIMgui library
    if (false == m_imgui.setup(*this))
        return false;

    // Enable some OpenGL states
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // Create 3 Scene nodes (robots)
    Node3D_SP robot1 = std::make_shared<CubicRobot>("CubicRobot1");
    Node3D_SP robot2 = std::make_shared<CubicRobot>("CubicRobot2");
    Node3D_SP robot3 = std::make_shared<CubicRobot>("CubicRobot3");

    // Place robots on the scene
    robot2->position(Vector3f(30.0f, 0.0f, 0.0f));
    robot3->position(Vector3f(60.0f, 0.0f, 0.0f));

    // Create the scene
    m_scene = Node3D::create("Root", false);
    m_scene->attach(robot1);
    m_scene->attach(robot2);
    m_scene->attach(robot3);

    m_scene->debug();

    // This is an example for searching a node.
    // Be careful: this is not a robust method: this function does not
    // manage nodes with duplicated identifier: it will halt on the
    // first id.
    {
        Node3D::getNode(m_scene, "CubicRobot1/Body/LeftLeg")->debug();
        //*(m_scene)["/Root/CubicRobot1/Body/LeftLeg"]->debug();
    }

    // Show the scene graph in the GUI. Note: this method is not safe
    // against tree reorganisation. This is just for the example !
    m_imgui.observeGraph(m_scene);

    return true;
}

//------------------------------------------------------------------------------
//! Draw the scene graph (made of robots)
//------------------------------------------------------------------------------
bool RobotsSceneGraph::draw()
{
    DEBUG("%s", "RobotsSceneGraph::draw()");

    // clear everything
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Traverse the scene graph for moving robots
    m_scene->update(dt());

    // Traverse the scene graph for drawing robots.
    m_scene->render(/*m_camera*/);

    // Paint the GUI
    if (false == m_imgui.draw())
        return false;

    return true;
}
