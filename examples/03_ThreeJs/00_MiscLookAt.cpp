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
// General Public License for more detailm_shape.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "00_MiscLookAt.hpp"
#include <iostream>

// https://github.com/mrdoob/three.js/blob/master/examples/misc_lookat.html

class MySphere: public Shape<Sphere, NormalsMaterial>
{
public:

    MySphere(std::string const& name, float const size)
        : Shape<Sphere, NormalsMaterial>(name),
          body(transform, size, units::mass::kilogram_t(0.01))
    {
        geometry.config.radius = size;
        if (!Shape<Sphere, NormalsMaterial>::compile())
        {
            throw GL::Exception("Failed create renderable");
        }
    }
public:

    rigidbody::Sphere body;
};

class MyBox: public Shape<Box, BasicMaterial>
{
public:

    MyBox(std::string const& name, Vector3f const& dimensions)
        : Shape<Box, BasicMaterial>(name),
          body(transform, dimensions, units::mass::kilogram_t(0.01))
    {
        Shape<Box, BasicMaterial>::material.color() = Color(0.5f, 0.0f, 0.0f).toVector3f();
        Shape<Box, BasicMaterial>::material.diffuse() = Color(0.4f, 0.0f, 0.0f).toVector3f();
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

//------------------------------------------------------------------------------
MiscLookAt::MiscLookAt(uint32_t const width, uint32_t const height, const char *title)
    : GLWindow(width, height, title),
      m_camera("camera", Camera::Type::PERSPECTIVE)
{
    std::cout << "Hello MiscLookAt: " << info() << std::endl;
}

//------------------------------------------------------------------------------
MiscLookAt::~MiscLookAt()
{
    std::cout << "Bye MiscLookAt" << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Callback when the window changed its size.
//------------------------------------------------------------------------------
void MiscLookAt::onWindowResized()
{
    // Note: height is never zero !
    float ratio = width<float>() / height<float>();

    // Make sure the viewport matches the new window dimensionm_shape.
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    for (auto& it: m_scene.root->children)
    {
        BaseShape* shape = dynamic_cast<BaseShape*>(it.get());
        shape->projectionMatrix() = m_camera.projection();
        shape->viewMatrix() = m_camera.view();
    }
}

// --------------------------------------------------------------
//! \brief Mouse event
// --------------------------------------------------------------
void MiscLookAt::onMouseMoved()
{
    float const dx = static_cast<float>(GLWindow::mouse().position.x) - m_camera.transform.position().x;
    float const dy = static_cast<float>(GLWindow::mouse().position.y) - m_camera.transform.position().y;

    m_mouse_position = Vector3f(dx * 0.05f, dy * 0.05f, 0.0f);
}

//------------------------------------------------------------------------------
//! \brief Init your a scene made of a moving cube and cones pointing to it.
//------------------------------------------------------------------------------
bool MiscLookAt::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // Enable IO callbacks
    reactTo(Event::MouseMove);
    hideCursor();

    // Place a camera
    m_camera.perspective.setClipping(0.1f, 15000.0f);
    m_camera.perspective.setFieldOfView(10.0_deg);
    m_camera.transform.position(Vector3f(0.0f, 0.0f, 3200.0f));
    m_camera.transform.lookAt(Vector3f(0.0f, 0.0f, 0.0f));

    //
    m_scene.root = AxesHelper::create<AxesHelper>("Axis", 10.0f);

    // Create a sphere
    MySphere& sphere = m_scene.root->attach<MySphere>("Sphere", 1.0f);

    // Create box (TODO cones)
    for (size_t i = 0u; i < 1000u; ++i) // FIXME pas la bonne facon de faire: ca creer plein de GLProgram inutillement: il faudrait que le m_prog soit partage.
    {
        MyBox& box = m_scene.root->attach<MyBox>("Cone" + std::to_string(i),
                                                 Vector3f(1.0f, 1.0f, 1.0f));
        Vector3f ran(dist::real<float>(), dist::real<float>(), dist::real<float>());
        box.transform.position(ran * 4000.0f - 2000.0f);
        box.transform.scaling(ran * 4.0f + 2.0f);
    }

    return true;
}

//------------------------------------------------------------------------------
void MiscLookAt::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void MiscLookAt::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
//! \brief
//------------------------------------------------------------------------------
bool MiscLookAt::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    static float time = 0.0f;
    time += dt();

    //sphere.transform.position(Vector3f(sinf(time * 0.7f) * 2000.0f,
    //                          cosf(time * 0.5f) * 2000.0f,
    //                          cosf(time * 0.3f) * 2000.0f));

    //for (auto it: m_scene->children())
    //  it->lookAt(m_cube->position());

    m_camera.transform.translate(m_mouse_position);
    m_camera.transform.lookAt(m_scene.root->transform.position());

    m_scene.update(dt());
    m_scene.draw(m_camera);

    return true;
}
