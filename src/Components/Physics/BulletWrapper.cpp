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

#include "Components/Physics/BulletWrapper.hpp"

using namespace units::mass;

//------------------------------------------------------------------------------
static btVector3 thickness2norm(Vector3f const& thickness)
{
    float dx = std::max(thickness.x, 0.01f);
    float dy = std::max(thickness.y, 0.01f);
    float dz = std::max(thickness.z, 0.01f);

    btVector3 norm;
    if (dx < dy && dx < dz)
        norm = btVector3(1.0f, 0.0f, 0.0f);
    else if (dy < dx && dy < dz)
        norm = btVector3(0.0f, 1.0f, 0.0f);
    else
        norm = btVector3(0.0f, 0.0f, 1.0f);

    return norm;
}

//------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, btVector3 const& v)
{
    return os << "[" << v[0] << ", " << v[1] << ", " << v[2] << ']';
}

//------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, btQuaternion const& q)
{
    return os << "(" << q.getW() << ", (" << q.getX() << ", " << q.getY()
              << ", " << q.getZ() << "))";
}

//------------------------------------------------------------------------------
inline static btVector3 cast_vector(Vector3f const& v)
{
    return btVector3(v[0], v[1], v[2]);
}

//------------------------------------------------------------------------------
inline static Vector3f cast_vector(btVector3 const& v)
{
    return Vector3f(v.getX(), v.getY(), v.getZ());
}

//------------------------------------------------------------------------------
inline static btQuaternion cast_quaternion(Quatf const& q)
{
    return btQuaternion(q.x, q.y, q.z, q.w);
}

//------------------------------------------------------------------------------
inline static Quatf cast_quaternion(btQuaternion const& q)
{
    return Quatf(q.getW(), q.getX(), q.getY(), q.getZ());
}

//------------------------------------------------------------------------------
PhysicsManager::PhysicsManager(Vector3f const& gravity)
{
    m_collision_configuration =
        std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(
        m_collision_configuration.get());
    m_broad_phase = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_dynamics_world = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(),
        m_broad_phase.get(),
        m_solver.get(),
        m_collision_configuration.get());
    m_dynamics_world->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}

// void PhysicsManager::debugDraw(int debugFlags)
//{
//     if (m_dynamicsWorldsWorld->getDebugDrawer())
//         m_dynamicsWorldsWorld->getDebugDrawer()->setDebugMode(debugFlags);
//     m_dynamicsWorldsWorld->debugDrawWorld();
// }

//------------------------------------------------------------------------------
void PhysicsManager::attach(RigidBody& obj)
{
    assert(m_dynamics_world != nullptr);
    m_dynamics_world->addRigidBody(&obj.rigidBody());
    m_objects.push_front(&obj);
    m_initial_transform_saved = false;
}

//------------------------------------------------------------------------------
void PhysicsManager::detach(RigidBody& obj)
{
    assert(m_dynamics_world != nullptr);
    m_dynamics_world->removeRigidBody(&obj.rigidBody());
    m_objects.push_front(&obj);
}

//------------------------------------------------------------------------------
void PhysicsManager::memorizeStates()
{
    for (auto it : m_objects)
    {
        it->setInitialTransform(it->m_transform, true);
    }
    m_initial_transform_saved = true;
}

//------------------------------------------------------------------------------
void PhysicsManager::update(float dt)
{
    assert(m_dynamics_world != nullptr);
    if (m_initial_transform_saved == false)
    {
        memorizeStates();
    }

    m_dynamics_world->stepSimulation(dt);

    for (auto it : m_objects)
    {
        it->update();
    }
}

//------------------------------------------------------------------------------
void PhysicsManager::reset()
{
    for (auto it : m_objects)
    {
        it->reset();
    }
}

//------------------------------------------------------------------------------
RigidBody::RigidBody(Transformable3D& transform,
                     std::unique_ptr<btCollisionShape> shape,
                     kilogram_t mass,
                     btScalar restitution,
                     btScalar friction)
    : m_transform(transform),
      m_initial_transform(cast_quaternion(transform.attitude()),
                          cast_vector(transform.position()))
{
    m_collision_shape = std::move(shape);
    m_motion = std::make_unique<btDefaultMotionState>(m_initial_transform);

    btRigidBody::btRigidBodyConstructionInfo info(
        mass.to<btScalar>(), m_motion.get(), m_collision_shape.get());
    info.m_restitution = restitution;
    info.m_friction = friction;
    m_rigid_body = std::make_unique<btRigidBody>(info);
    setMass(mass);
}

//------------------------------------------------------------------------------
void RigidBody::setInitialTransform(Transformable3D const& transform,
                                    bool apply)
{
    m_initial_transform = btTransform(cast_quaternion(transform.attitude()),
                                      cast_vector(transform.position()));
    if (apply)
    {
        reset();
    }
}

//------------------------------------------------------------------------------
void RigidBody::setMass(kilogram_t mass)
{
    btVector3 inertia(0, 0, 0);
    btScalar m = mass.to<btScalar>();

    if (m > 0) // Dynamic object
    {
        m_collision_shape->calculateLocalInertia(m, inertia);
    }

    m_rigid_body->setMassProps(m, inertia);
    m_mass = mass;
}

//------------------------------------------------------------------------------
void RigidBody::reset()
{
    m_rigid_body->setWorldTransform(m_initial_transform);
    m_rigid_body->getMotionState()->setWorldTransform(m_initial_transform);
    m_rigid_body->clearForces();

    if (m_mass.to<btScalar>() > 0) // Dynamic object
    {
        m_rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        m_rigid_body->setAngularVelocity(btVector3(0, 0, 0));
        m_rigid_body->activate(); // Required if the object was asleep
    }

    m_transform.attitude(cast_quaternion(m_initial_transform.getRotation()));
    m_transform.position(cast_vector(m_initial_transform.getOrigin()));
}

//------------------------------------------------------------------------------
void RigidBody::update()
{
    btTransform wTrans;
    m_rigid_body->getMotionState()->getWorldTransform(wTrans);
    m_transform.attitude(cast_quaternion(wTrans.getRotation()));
    m_transform.position(cast_vector(wTrans.getOrigin()));
}

namespace rigidbody
{
//--------------------------------------------------------------------------
WorldPlane::WorldPlane(Transformable3D& transform,
                       Vector3f const& thickness,
                       float restitution,
                       float friction)
    : RigidBody(
          transform,
          std::make_unique<btStaticPlaneShape>(thickness2norm(thickness), 0),
          kilogram_t(0),
          restitution,
          friction)
{
}

//--------------------------------------------------------------------------
Sphere::Sphere(Transformable3D& transform,
               float radius,
               kilogram_t mass,
               float restitution,
               float friction)
    : RigidBody(transform,
                std::make_unique<btSphereShape>(radius),
                mass,
                restitution,
                friction)
{
}

//--------------------------------------------------------------------------
Box::Box(Transformable3D& transform,
         Vector3f const& dimensions,
         kilogram_t mass,
         float restitution,
         float friction)
    : RigidBody(transform,
                std::make_unique<btBoxShape>(cast_vector(dimensions)),
                mass,
                restitution,
                friction)
{
}

//--------------------------------------------------------------------------
Capsule::Capsule(Transformable3D& transform,
                 float radius,
                 float height,
                 units::mass::kilogram_t mass,
                 float restitution,
                 float friction)
    : RigidBody(transform,
                std::make_unique<btCapsuleShape>(radius, height),
                mass,
                restitution,
                friction)
{
}

//--------------------------------------------------------------------------
Cylinder::Cylinder(Transformable3D& transform,
                   Vector3f const& dimensions,
                   kilogram_t mass,
                   float restitution,
                   float friction)
    : RigidBody(transform,
                std::make_unique<btCylinderShape>(cast_vector(dimensions)),
                mass,
                restitution,
                friction)
{
}

} // namespace rigidbody
