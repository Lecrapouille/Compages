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

#include "00_SG_Base.hpp"
#include <iostream>

//--------------------------------------------------------------------------
//! \brief
//--------------------------------------------------------------------------
class MySceneObject: public SceneObject
{
public:

    MySceneObject(std::string const& name) : SceneObject(name)
    {
        std::cout << name << ": onCreate()" << std::endl;
    }

    // FIXME private:

    // Called Called by SceneTree (usually on onSetup())
    virtual bool onSetup() override
    {
        std::cout << name() << ": onSetup()" << std::endl;
        return true;
    }

    // Called Called by SceneTree (usually on onPaint())
    virtual void onUpdate(float const dt) override
    {
        std::cout << name() << ": onUpdate(" << dt << ')' << std::endl;
    }

    // Called Called by SceneTree (usually on onPaint())
    virtual bool onDraw(Matrix44f const& modelMatrix) override
    {
        std::cout << name() << ": onDraw(" << modelMatrix << ')' << std::endl;
        return true;
    }

    // Called Called when you enable the node (traversable)
    virtual void onEnable() override
    {
        std::cout << name() << ": onEnable()" << std::endl;
    }

    // Called Called when you disable the node (non traversable)
    virtual void onDisable() override
    {
        std::cout << name() << ": onDisable()" << std::endl;
    }
};

//------------------------------------------------------------------------------
SGBase::SGBase(uint32_t const width, uint32_t const height, const char* title)
    : GLWindow(width, height, title)
{
    std::cout << "Hello SGBase: " << info() << std::endl;
}

//------------------------------------------------------------------------------
SGBase::~SGBase()
{
    std::cout << "Bye SGBase" << std::endl;
}

//------------------------------------------------------------------------------
bool SGBase::onSetup()
{
    // Create a root node. Note that ::Ptr is a std::unique_ptr but with shorter
    // name.
    MySceneObject::Ptr obj0 = MySceneObject::create<MySceneObject>("obj0");
    assert(obj0->parent == nullptr);
    assert(obj0->children.size() == 0u);

    // Return the number of elements
    std::cout << "Number of elements: " << obj0->size() << std::endl;
    assert(obj0->size() == 1u);

    // Show the tree content
    std::cout << "Debug obj0: " << std::endl;
    obj0->traverse([](SceneObject* node) { std::cout << *node << std::endl; });

    // Attach a child node (obj1) to the root node (obj0)
    MySceneObject& obj1 = obj0->attach<MySceneObject>("obj1");
    std::cout << "Number of elements: " << obj0->size() << std::endl
              << std::endl;
    assert(obj0->size() == 2u);
    assert(obj0->parent == nullptr);
    assert(obj0->children.size() == 1u);
    assert(obj1.parent == obj0.get());
    assert(obj1.children.size() == 0u);

    // Attach a child node (obj2) to the node (obj1)
    /*MySceneObject& obj2 =*/obj1.attach<MySceneObject>("obj2");
    assert(obj0->size() == 3u);
    std::cout << "First child: " << obj0->children[0]->name() << std::endl;
    std::cout << "First grandson: " << obj0->children[0]->children[0]->name()
              << std::endl
              << std::endl;

    // Alternate way to attach the second child node (obj3) to the node (obj1).
    MySceneObject::Ptr obj3 = MySceneObject::create<MySceneObject>("obj3");
    obj1.attach(
        std::move(obj3)); // BEWARE: do not use obj3: no longer exists !!!
    assert(obj0->size() == 4u);

    // Show the tree content
    std::cout << "Debug obj0: " << std::endl;
    obj0->traverse([](SceneObject* node) { std::cout << *node << std::endl; });

    // Release the node and children nodes
    std::cout << "Clear obj1:" << std::endl;
    obj1.clear();
    std::cout << "Number of elements: " << obj0->size() << std::endl;
    assert(obj0->size() == 2u);

    // Release the node and children nodes
    std::cout << "Clear obj0:" << std::endl;
    obj0->clear();
    assert(obj0->parent == nullptr);
    assert(obj0->children.size() == 0u);
    assert(obj0->size() == 1u);

    // Enable/disable node
    obj0->enable(false); // Not renderable, not travsersable
    assert(obj0->enabled() == false);
    obj0->enable(); // Equivalent to obj0->enable(true);
    assert(obj0->enabled() == true);

    // m_scene: SceneTree holds SceneObject but also performs some actions on
    // them
    assert(m_scene.root == nullptr);
    m_scene.root = std::move(obj0);
    m_scene.setup();

    return true;
}

//------------------------------------------------------------------------------
//! \brief Renderer our shape
//------------------------------------------------------------------------------
bool SGBase::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    m_scene.update(dt());
    m_scene.draw();

    return true;
}

void SGBase::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

void SGBase::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
