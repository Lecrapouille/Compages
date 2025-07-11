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

#ifndef BULLETWRAPPER_HPP
#define BULLETWRAPPER_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wreorder-ctor"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#define BT_THREADSAFE 0
#include "bullet/btBulletDynamicsCommon.h"
#pragma GCC diagnostic pop

#include "Math/Transformable.hpp"
#include "Math/Vector.hpp"
#include <forward_list>

class RigidBody;

// *****************************************************************************
//! \brief Responsible class for tracking, updating, and synchronizing the state
//! of the physical world and all non-static geometry in the scene. The physical
//! world in this case consists of any objects which can be manipulated
//! (kinematically or dynamically) or simulated and anything such objects must
//! be aware of (e.g. static scene collision geometry).
// *****************************************************************************
class PhysicsManager
{
public:

    //--------------------------------------------------------------------------
    //! \brief Initiate assets from the Bullet3 library needed for tracking,
    //! updating, and synchronizing the state of the physical world and all
    //! non-static geometry in the scene.
    //!
    //! \param[in] gravity: Set the gravity of the physical world. Default
    //! direction is X = 0, Y = -9.8 and Z = 0. Like OpenGL, the X-axis shows
    //! the right direction, the Y shows the up direction and the Z-axis is
    //! facing the player.
    //--------------------------------------------------------------------------
    PhysicsManager(Vector3f const& gravity = Vector3f(0.0f, -9.8f, 0.0f));

    //--------------------------------------------------------------------------
    //! \brief Getter. Return the DiscreteDynamicsWorld used.
    //--------------------------------------------------------------------------
    btDiscreteDynamicsWorld& world()
    {
        return *m_dynamicsWorld;
    }

    //--------------------------------------------------------------------------
    //! \brief Add a object in the list of objects to update the dynamic.
    //--------------------------------------------------------------------------
    void attach(RigidBody& obj);

    //--------------------------------------------------------------------------
    //! \brief Remove a object from the list of objects to update the dynamic.
    //--------------------------------------------------------------------------
    void detach(RigidBody& obj);

    //--------------------------------------------------------------------------
    //! \brief Save the current transform states (position and attitude) of each
    //! 3d models into their associated RigidBody instance. This transform is
    //! needed by this manager instance for initializing and reseting objects
    //! positions and attitude in the solver.
    //!
    //! Call this method between the initialisation of the transform of the 3d
    //! model and the first call of update(). If not done the updat() will call
    //! at the first iteration and only once. To change initial states. Call
    //! this method explicitly.
    //!
    //! \code
    //! SceneTree scene; PhysicsManager pm;
    //! auto& node = scene.root.attach<Shape<Model, Material>>(...);
    //! node.transform.xxx
    //! pm.attach(node);
    //! pm.update(...); // will call pm.memorizeStates();
    //! node.transform.xxx
    //! pm.memorizeStates();
    //! pm.update(...);
    //! \endcode
    //--------------------------------------------------------------------------
    void memorizeStates();

    //--------------------------------------------------------------------------
    //! \brief Do a simulation step.
    //! \param[in] dt: the delta time from the previous call. Shall be seconds.
    //--------------------------------------------------------------------------
    void update(float dt); // FIXME in seconds

    //--------------------------------------------------------------------------
    //! \brief Restore solver to initial states for each objects.
    //--------------------------------------------------------------------------
    void reset();

private:

    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btDbvtBroadphase> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
    std::forward_list<RigidBody*> m_objects;
    bool m_initialTransformSaved = false;
};

// *****************************************************************************
//! \brief Base class for a moving SceneTree node. Integrate a physical object
//! (btRigidBody), a collision shape (btCollisionShape) and the transform
//! instance of the  SceneNode owning this instance.
// *****************************************************************************
class RigidBody
{
    friend class PhysicsManager;

public:

    //--------------------------------------------------------------------------
    //! \brief Creates a new rigid body as a btRigidBody object.
    //! \param[in] reference to the transform attitude and position of the
    //!  SceneTree node owning this class instance.
    //! \param[in] collision_shape
    //! \param[in] mass
    //! \param[in] restitution
    //! \param[in] friction
    //--------------------------------------------------------------------------
    RigidBody(Transformable3D& transform,
              std::unique_ptr<btCollisionShape> collision_shape,
              units::mass::kilogram_t mass,
              float restitution,
              float friction);

    //--------------------------------------------------------------------------
    //! \brief Save the initial transform applied on reset events to restore
    //! states. Call this method after the constructor in the case the attitude
    //! and position passed as parameter of the constructor was not init.
    //! \param[in] transform the attitude and position to apply on reset.
    //! \param[in] reset if set to true for
    //--------------------------------------------------------------------------
    void setInitialTransform(Transformable3D const& transform,
                             bool reset = true);

    //--------------------------------------------------------------------------
    //! \brief Set the object's mass and replace the one used as param in the
    //! constructor.
    //! \param mass specifies mass in kg units.
    //! \note If mass is > 0 the object will be dynamic, else will be static.
    //--------------------------------------------------------------------------
    void setMass(units::mass::kilogram_t mass);

    //--------------------------------------------------------------------------
    //! \brief Return the Bullet3 rigid body instance
    //--------------------------------------------------------------------------
    inline btRigidBody& rigidBody()
    {
        // Never null since set by the constructor
        return *m_rigid_body;
    }

    //--------------------------------------------------------------------------
    //! \brief Reposition object to its inital state.
    //--------------------------------------------------------------------------
    void reset();

    //--------------------------------------------------------------------------
    //! \brief Update callback.
    //! This function is called automatically by PhysicsManager::update() for
    //! each registered object. It update position and attitude of graphical
    //! object from its physhical state.
    //--------------------------------------------------------------------------
    void update();

protected:

    //! \brief Transform instance of the SceneTree node owner (3d model).
    Transformable3D& m_transform;
    //! \brief Inital transform state (orientation and position). Applied on
    //! reset events to restore states.
    btTransform m_initial_transform;
    //! \brief Object's collision shape.
    std::unique_ptr<btCollisionShape> m_collision_shape;
    //! \brief Object's rigid body
    std::unique_ptr<btRigidBody> m_rigid_body;
    //! \brief Memorize this state to be released during the destruction of this
    //! instance
    std::unique_ptr<btDefaultMotionState> m_motion = nullptr;
    //! \brief Mass of the object (0 for static object)
    units::mass::kilogram_t m_mass;
};

namespace rigidbody
{

// *************************************************************************
//! \brief
// *************************************************************************
class WorldPlane: public RigidBody
{
public:

    WorldPlane(Transformable3D& transform,
               Vector3f const& thickness,
               float restitution = 0.0f,
               float friction = 0.5f);
};

// *************************************************************************
//! \brief
// *************************************************************************
class Sphere: public RigidBody
{
public:

    Sphere(Transformable3D& transform,
           float radius,
           units::mass::kilogram_t mass,
           float restitution = 0.0f,
           float friction = 0.5f);
};

// *************************************************************************
//! \brief
// *************************************************************************
class Box: public RigidBody
{
public:

    Box(Transformable3D& transform,
        Vector3f const& dimensions,
        units::mass::kilogram_t mass,
        float restitution = 0.0f,
        float friction = 0.5f);
};

// *************************************************************************
//! \brief
// *************************************************************************
class Capsule: public RigidBody
{
public:

    Capsule(Transformable3D& transform,
            float radius,
            float height,
            units::mass::kilogram_t mass,
            float restitution = 0.0f,
            float friction = 0.5f);
};

// *************************************************************************
//! \brief
// *************************************************************************
class Cylinder: public RigidBody
{
public:

    Cylinder(Transformable3D& transform,
             Vector3f const& dimensions,
             units::mass::kilogram_t mass,
             float restitution = 0.0f,
             float friction = 0.5f);
};
} // namespace rigidbody

#endif // BULLETWRAPPER_HPP
