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

#include "02_SG_MaterialsAndShapes.hpp"
#include <iostream>

class WorldGround: public Shape<Box, BasicMaterial> // SceneObject
{
public:

    WorldGround(std::string const& name, Vector3f const& dimensions)
        : Shape<Box, BasicMaterial>(name),
          body(transform, dimensions, units::mass::kilogram_t(0))
    {
        Shape<Box, BasicMaterial>::material.color() =
            Color(0.5f, 0.0f, 0.0f).toVector3f();
        Shape<Box, BasicMaterial>::material.diffuse() =
            Color(0.4f, 0.0f, 0.0f).toVector3f();
        Shape<Box, BasicMaterial>::material.opacity() = 1.0f;

        geometry.config.width = dimensions[0];
        geometry.config.height = dimensions[1];
        geometry.config.depth = dimensions[2];
        if (!Shape<Box, BasicMaterial>::compile())
        {
            throw GL::Exception("Failed create renderable");
        }
    }

public:

    rigidbody::Box body;
};

class MySphere: public Shape<Sphere, NormalsMaterial>
{
public:

    MySphere(std::string const& name, float const size)
        : Shape<Sphere, NormalsMaterial>(name),
          body(transform, size, units::mass::kilogram_t(0.01))
    {
        geometry.config.radius = 1.0f;
        if (!Shape<Sphere, NormalsMaterial>::compile())
        {
            throw GL::Exception("Failed create renderable");
        }
    }

public:

    rigidbody::Sphere body;
};

class MyBox: public Shape<Box, NormalsMaterial>
{
public:

    MyBox(std::string const& name, Vector3f const& dimensions)
        : Shape<Box, NormalsMaterial>(name),
          body(transform, dimensions, units::mass::kilogram_t(0.1))
    {
        geometry.config.width = 2.0f * dimensions[0];
        geometry.config.height = 2.0f * dimensions[1];
        geometry.config.depth = 2.0f * dimensions[2];
        if (!Shape<Box, NormalsMaterial>::compile())
        {
            throw GL::Exception("Failed create renderable");
        }
    }

public:

    rigidbody::Box body;
};

//------------------------------------------------------------------------------
//! \brief
//------------------------------------------------------------------------------
template <class Material>
class MyShape: public Shape<Model, Material>
{
public:

    MyShape(std::string const& name, std::string const& path)
        : Shape<Model, Material>(name),
          body(Shape<Model, Material>::transform,
               1.0f,
               units::mass::kilogram_t(0.01))
    {
        Shape<Model, Material>::geometry.config.path = path;
        if (!Shape<Model, Material>::compile())
        {
            throw GL::Exception("Failed create renderable");
        }
        // FIXME: devrait pouvoir l'appeller avant compile()
        initMaterial();
    }

    inline std::string const& name() const
    {
        return Shape<Model, Material>::name();
    }

private:

    inline void initMaterial();

public:

    rigidbody::Sphere body;
};

template <>
void MyShape<DepthMaterial>::initMaterial()
{
    Shape<Model, DepthMaterial>::material.near() = 3.0f;
    Shape<Model, DepthMaterial>::material.far() = 5.0f;
}

template <>
void MyShape<NormalsMaterial>::initMaterial()
{
    // Nothing to do
}

template <>
void MyShape<BasicMaterial>::initMaterial()
{
    Shape<Model, BasicMaterial>::material.color() =
        Color(0.5f, 0.0f, 0.0f).toVector3f();
    Shape<Model, BasicMaterial>::material.diffuse() =
        Color(0.4f, 0.0f, 0.0f).toVector3f();
    Shape<Model, BasicMaterial>::material.opacity() = 1.0f;
}

//------------------------------------------------------------------------------
SGMatAndShape::SGMatAndShape(uint32_t const width,
                             uint32_t const height,
                             const char* title)
    : GLWindow(width, height, title), m_camera("camera")
{
    reactTo(Event::MouseMove | Event::MouseScroll | Event::MouseButton |
            Event::Keyboard);

    m_camera.transform.position(Vector3f(10, 0, 5));
    m_camera.transform.lookAt(Vector3f(0, 0, 0));
    m_camera.is(Camera::Type::PERSPECTIVE);

    std::cout << "Hello Material: " << info() << std::endl;
}

//------------------------------------------------------------------------------
SGMatAndShape::~SGMatAndShape()
{
    std::cout << "Bye Material" << std::endl;
}

//------------------------------------------------------------------------------
void SGMatAndShape::onWindowResized()
{
    m_camera.projection(width<float>(), height<float>());
}

//------------------------------------------------------------------------------
bool SGMatAndShape::onSetup()
{
    m_layers.push_back(std::make_unique<SGMatAndShape::GUI>(*this));

    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glEnable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    // glCheck(glDisable(GL_CULL_FACE));

    m_scene.root = AxesHelper::create<AxesHelper>("Axis", 10.0f);
    // RigNode& rig = m_scene.root->attach<RigNode>("Rig", m_camera);

    WorldGround& wg = m_scene.root->attach<WorldGround>(
        "WorldGround", Vector3f(10.0f, 1.0f, 10.0f));
    // MyShape<BasicMaterial>& t0 =
    // m_scene.root->attach<MyShape<BasicMaterial>>("Tree0",
    // "external/assets/tree.obj");
    MyShape<DepthMaterial>& t1 = m_scene.root->attach<MyShape<DepthMaterial>>(
        "Tree1", "external/assets/tree.obj");
    // MyShape<NormalsMaterial>& t2 =
    // m_scene.root->attach<MyShape<NormalsMaterial>>("Tree2",
    // "external/assets/tree.obj");
    MySphere& t2 = m_scene.root->attach<MySphere>("Tree2", 1.0f);
    MyBox& t3 = m_scene.root->attach<MyBox>("Tree2", Vector3f(1, 1, 3));
    // MyShape<DepthMaterial>& t3 = t1.attach<MyShape<DepthMaterial>>("Tree1.0",
    // "external/assets/tree.obj"); MyShape<BasicMaterial>& t4 =
    // t1.attach<MyShape<BasicMaterial>>("Tree1.1", "external/assets/tree.obj");

    //      Y
    //     |
    //     |
    //     |________ X
    //    /
    //   /          Tree0
    //  /  Tree2             Tree1
    // Z             Tree1.0       Tree1.1
    //
    // wg.transform.position(Vector3f(0.0f, 0.0f, 0.0f));
    t1.transform.position(Vector3f(2.0f, 5.0f, 0.0f));
    t2.transform.position(Vector3f(0.0f, 3.0f, 2.0f));
    t3.transform.position(Vector3f(0.0f, 10.0f, 0.0f));
    // t4.transform.position(Vector3f(2.0f, 10.0f, 0.0f));

    // m_physics.attach(t0.body);
    m_physics.attach(t1.body);
    m_physics.attach(t2.body);
    m_physics.attach(t3.body);
    m_physics.attach(wg.body);

    // m_scene.debug();

    // Success
    return true;
}

//------------------------------------------------------------------
//! \brief Paint some DearImGui widgets.
//------------------------------------------------------------------
bool SGMatAndShape::GUI::onImGuiRender()
{
    static float new_near = 3.0f;
    static float previous_near = 0.0f;
    static float new_far = 7.0f;
    static float previous_far = 0.0f;

    ImGui::Begin("Hello, world!");
    ImGui::Text("Depth Material:");
    ImGui::SliderFloat("near", &new_near, 0.01f, 15.0f);
    ImGui::SliderFloat("far ", &new_far, 0.01f, 15.0f);
    ImGui::End();

    SGMatAndShape& win = owner<SGMatAndShape>();

    // Apply new depth near value to the shape
    if (abs(new_near - previous_near) > 0.001f)
    {
        previous_near = new_near;
        win.m_scene.root->traverse(
            [](SceneObject* node, float const near_)
            {
                auto n = dynamic_cast<MyShape<DepthMaterial>*>(node);
                if (n != nullptr)
                {
                    n->material.near() = near_;
                }
            },
            new_near);
    }

    // Apply new depth far value to the shape
    if (abs(new_far - previous_far) > 0.001f)
    {
        previous_far = new_far;
        win.m_scene.root->traverse(
            [](SceneObject* node, float const far_)
            {
                auto n = dynamic_cast<MyShape<DepthMaterial>*>(node);
                if (n != nullptr)
                {
                    n->material.far() = far_;
                }
            },
            new_far);
    }

    return true;
}

//------------------------------------------------------------------------------
//! \brief Renderer our shape
//------------------------------------------------------------------------------
bool SGMatAndShape::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Update parent-child transform matrix for nodes
    m_scene.update(dt());
    m_physics.update(dt());

    // Perspective camera 1st view
    m_camera.transform.lookAt(
        Vector3f(5, 5, 5), Vector3f(0, 0, 0), Vector3f(0, 1, 0));
    m_camera.is(Camera::Type::PERSPECTIVE);
    m_camera.setViewPort(0.0f, 0.0f, 0.5f, 1.0f);
    m_scene.draw(m_camera);

    // Perspective camera 2nd view
    m_camera.transform.position(Vector3f(10, 0, 5));
    m_camera.transform.lookAt(Vector3f(0, 0, 0));
    m_camera.is(Camera::Type::PERSPECTIVE);
    m_camera.setViewPort(0.5f, 0.0f, 0.5f, 0.5f);
    m_scene.draw(m_camera);

    // Orthographic camera 3th view
    // m_camera.transform.position(Vector3f(10,0,5));
    m_camera.transform.lookAt(Vector3f(0, 0, 0));
    m_camera.is(Camera::Type::ORTHOGRAPHIC);
    m_camera.orthographic.setPlanes(0, 800, 600, 0);
    m_camera.setViewPort(0.5f, 0.5f, 0.5f, 0.5f);
    m_scene.draw(m_camera);

    // Success
    return true;
}

//------------------------------------------------------------------------------
void SGMatAndShape::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void SGMatAndShape::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
