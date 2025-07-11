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

#include "main.hpp"
#define protected public
#define private public
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "Components/Components.hpp"
#pragma GCC diagnostic pop
#undef protected
#undef private

// -----------------------------------------------------------------------------
class NamedComponent: public Component
{
public:

    NamedComponent(std::string&& name) : m_name(name) {}

    std::string const& name() const
    {
        return m_name;
    }

private:

    std::string m_name;
};

// -----------------------------------------------------------------------------
class Collider: public NamedComponent
{
public:

    Collider(std::string&& name) : NamedComponent(std::move(name)) {}

    virtual bool isClassType(const std::size_t classType) const
    {
        if (classType == Collider::Type)
            return true;
        return Component::isClassType(classType);
    }

    int doCollider()
    {
        return 42;
    }

public:

    static const std::size_t Type;
};

const std::size_t Collider::Type = "Collider"_hash;

// -----------------------------------------------------------------------------
class BoxCollider: public Collider
{
public:

    BoxCollider(std::string&& name) : Collider(std::move(name)) {}

    virtual bool isClassType(const std::size_t classType) const
    {
        if (classType == BoxCollider::Type)
            return true;
        return Collider::isClassType(classType);
    }

    int doBoxCollider()
    {
        return 66;
    }

public:

    static const std::size_t Type;
};

const std::size_t BoxCollider::Type = "BoxCollider"_hash;

//--------------------------------------------------------------------------
TEST(TestComponents, testInheritanceType)
{
    // Base class
    {
        Component c;

        ASSERT_EQ(Component::Type, "Component"_hash);
        ASSERT_EQ(c.Type, "Component"_hash);

        ASSERT_EQ(c.isClassType("Component"_hash), true);
        ASSERT_EQ(c.isClassType("Componen"_hash), false);
    }

    // Collider class
    {
        Collider c("A");

        ASSERT_EQ(Collider::Type, "Collider"_hash);
        ASSERT_EQ(c.Type, "Collider"_hash);

        ASSERT_EQ(c.isClassType("Collider"_hash), true);
        ASSERT_EQ(c.isClassType("Component"_hash), true);
        ASSERT_EQ(c.isClassType("foo"_hash), false);
    }

    // BoxCollider class
    {
        BoxCollider c("B");

        ASSERT_EQ(BoxCollider::Type, "BoxCollider"_hash);
        ASSERT_EQ(c.Type, "BoxCollider"_hash);

        ASSERT_EQ(c.isClassType("BoxCollider"_hash), true);
        ASSERT_EQ(c.isClassType("Collider"_hash), true);
        ASSERT_EQ(c.isClassType("Component"_hash), true);
        ASSERT_EQ(c.isClassType("bar"_hash), false);
    }
}

//--------------------------------------------------------------------------
TEST(TestComponents, testConstructor)
{
    // Component holder. Check it has no component.
    Components client;
    ASSERT_EQ(client.m_components.size(), 0u);
}

//--------------------------------------------------------------------------
TEST(TestComponents, testNominalCases)
{
    Components client;

    // --- Insert the first component.
    BoxCollider& boxcollider1 = client.addComponent<BoxCollider>("BoxCollider");

    // Check it has been inserted.
    ASSERT_EQ(1u, client.m_components.size());
    ASSERT_EQ(1u, client.countComponents());
    ASSERT_EQ(&boxcollider1,
              static_cast<NamedComponent*>(client.m_components[0].get()));
    ASSERT_STREQ("BoxCollider",
                 static_cast<NamedComponent*>(client.m_components[0].get())
                     ->name()
                     .c_str());
    ASSERT_STREQ("BoxCollider", boxcollider1.name().c_str());
    ASSERT_EQ(42, boxcollider1.doCollider());
    ASSERT_EQ(66, boxcollider1.doBoxCollider());

    // Use API to check if the component has been inserted (direct class).
    BoxCollider& boxcollider2 = client.getComponent<BoxCollider>();
    ASSERT_EQ(&boxcollider1, &boxcollider2);
    ASSERT_STREQ("BoxCollider", boxcollider2.name().c_str());
    ASSERT_EQ(true, boxcollider2.isClassType("BoxCollider"_hash));
    ASSERT_EQ(42, boxcollider2.doCollider());
    ASSERT_EQ(66, boxcollider2.doBoxCollider());

    // Use API to check if the component has been inserted (ancestor class).
    Collider& collider1 = client.getComponent<Collider>();
    ASSERT_EQ(&boxcollider1, &collider1);
    ASSERT_STREQ("BoxCollider", collider1.name().c_str());
    ASSERT_EQ(true, collider1.isClassType("Collider"_hash));
    ASSERT_EQ(42, collider1.doCollider());

    // --- Insert the second component. Check it has been inserted.
    Collider& collider2 = client.addComponent<Collider>("Collider");
    ASSERT_EQ(2u, client.m_components.size());
    ASSERT_EQ(2u, client.countComponents());
    ASSERT_EQ(&boxcollider1,
              static_cast<NamedComponent*>(client.m_components[0].get()));
    ASSERT_EQ(&collider2,
              static_cast<NamedComponent*>(client.m_components[1].get()));
    ASSERT_STREQ("BoxCollider",
                 static_cast<NamedComponent*>(client.m_components[0].get())
                     ->name()
                     .c_str());
    ASSERT_STREQ("Collider",
                 static_cast<NamedComponent*>(client.m_components[1].get())
                     ->name()
                     .c_str());
    ASSERT_STREQ("Collider", collider2.name().c_str());
    ASSERT_EQ(42, collider2.doCollider());

    // Try to get to the BoxCollider component. Check the API returns the 1st
    // type matching element. In our case BoxCollider is returned since it's the
    // first element in the container with BoxCollider as type.
    BoxCollider& boxcollider3 = client.getComponent<BoxCollider>();
    ASSERT_EQ(&boxcollider3,
              static_cast<NamedComponent*>(client.m_components[0].get()));
    ASSERT_STREQ("BoxCollider", boxcollider3.name().c_str());
    ASSERT_EQ(true, boxcollider3.isClassType("BoxCollider"_hash));
    ASSERT_EQ(42, boxcollider3.doCollider());
    ASSERT_EQ(66, boxcollider3.doBoxCollider());

    // Try to get to the Collider component. Check the API returns the 1st type
    // matching element. In our case while we have a Collider in the second slot
    // of the container, the BoxCollider of the 1st slot is returned since it's
    // the first element in the container with Collider type as ancestor.
    Collider& collider3 = client.getComponent<Collider>();
    ASSERT_EQ(&collider3,
              static_cast<NamedComponent*>(client.m_components[0].get()));
    ASSERT_STREQ("BoxCollider", collider3.name().c_str());
    ASSERT_EQ(true, collider3.isClassType("Collider"_hash));
    ASSERT_EQ(42, collider3.doCollider());

    // Get the list of all components of given type
    std::vector<Collider*> components = client.getComponents<Collider>();
    ASSERT_EQ(2u, components.size());
    ASSERT_STREQ("BoxCollider", components[0]->name().c_str());
    ASSERT_STREQ("Collider", components[1]->name().c_str());

    // Remove existing components of given type
    size_t removed = client.removeComponents<Collider>();
    ASSERT_EQ(2u, removed);
    ASSERT_EQ(0u, client.m_components.size());
    ASSERT_EQ(0u, client.countComponents());

    try
    {
        Collider& collider4 = client.getComponent<Collider>();
        ADD_FAILURE()
            << "Should have thow an exception since no more component";
        std::cout << collider4.name() << std::endl;
    }
    catch (std::out_of_range const& e)
    {
        ASSERT_TRUE(true);
    }

    // Remove an element in a dummy container
    bool res = client.removeComponent<Collider>();
    ASSERT_EQ(false, res);
    std::vector<Collider*> components2 = client.getComponents<Collider>();
    ASSERT_EQ(0u, components2.size());

    //
    client.addComponent<BoxCollider>("BoxCollider");
    ASSERT_EQ(1u, client.countComponents());
    res = client.removeComponent<Collider>();
    ASSERT_EQ(true, res);
    ASSERT_EQ(0u, client.countComponents());
}
