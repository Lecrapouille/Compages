# 🟦 The World

[⏫ Concepts](README.md)

![07c_MovingRobot](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07c_MovingRobot.png)

A simulation needs two different kinds of organisation. A robot arm is a **tree**: the forearm hangs on the elbow, which hangs on the shoulder; turning the shoulder must move everything after it. A swarm of a thousand drones is a **table**: the same data (position, speed, battery) for each, and one loop over all of them. A tree is slow to loop over; a table does not know parents and children. Engines usually pick one and fake the other.

## ✌ One container, two mechanisms

A `compages::world::World` holds both:

- **A scene graph**: each entity has a parent, children and a transform (position, rotation, scale) relative to its parent. `world.update()` computes the world matrix of every entity, from the root down. This is what every game engine has (Godot, Unity, Unreal).
- **An ECS** ([EnTT](https://github.com/skypjack/entt)): any struct of yours can be attached to an entity as a component, and `world.each<A, B>(fn)` visits only the entities that have both, with the data packed in memory.

An entity may use one, the other, or both. The workshop arm uses the graph only; `07a_HeadlessWorld` moves its entities with its own `Position` and `Velocity` structs.

| Idea | Call | For |
|------|------|-----|
| Container | `World` | the source of truth |
| Handle | `Entity` | chained calls: `world.entity("Ship").child("Gun").position(…)` |
| Identity | `EntityId` | a 32-bit number with a generation, safe to store anywhere |
| Parenting | `child()`, `parent()`, `lookup("Ship/Gun")` | the scene graph |
| Placement | `.position()`, `.rotation()`, `.scale()`, `.lookAt()` | the transform in the graph |
| Data | `.set(T{})`, `.add<T>(…)`, `.get<T>()`, `.has<T>()`, `.remove<T>()` | your structs |
| System | `world.each<T…>(fn)` | a loop over the entities that have these components |
| Step | `world.update(frame)` | routines, joints, world matrices |
| Local logic | `Routine` | `update(dt)` on one entity |

`Entity` is a handle: copying it does not copy the entity. An empty handle, or one whose entity was destroyed, is `false`. `entity.enable(false)` switches an entity and its children off.

A small headless program, compiled and run against the library:

```cpp
#include <Compages/World/Entity.hpp>
#include <iostream>

struct Position { compages::Vector3f value{ 0, 0, 0 }; };
struct Velocity { compages::Vector3f value{ 0, 0, 0 }; };

int main()
{
    compages::world::World world;
    world.entity("player").set(Position{}).set(Velocity{ { 1, 0, 0 } });
    world.entity("rock").set(Position{ { 0, 5, 0 } });   // no Velocity: the loop skips it

    compages::Frame step;
    step.elapsed = 1.0f / 60.0f;
    for (int i = 0; i < 60; ++i)
    {
        world.each<Position, Velocity>([&](compages::world::Entity, Position& p, Velocity& v)
                                       { p.value += v.value * step.elapsed; });
        world.update(step);
    }
    compages::Vector3f const& x = world.lookup("player").get<Position>().value;
    std::cout << "player after 1 s: " << x.x << " " << x.y << " " << x.z << "\n";
    return 0;
}
```

```text
player after 1 s: 1 0 0
```

`each` runs only when you call it: `update()` does not discover your systems. `Position` here is your struct; the place of the entity in the graph, which `.position(x, y, z)` changes, is another thing. Choose what fits; both can live together.

## 🦾 Joints

A joint is a child with one freedom instead of six: `.revolute(axis, min, max)` turns round an axis, `.prismatic(axis, min, max)` slides along it. `entity.joint().angle(a)` (or `.offset(o)`) sets the value; `update()` turns it into the local transform, then the matrices follow down the chain. This is forward kinematics, with no screen.

A joint keeps two values: the **commanded** one, which you set and may be outside the limits, and the **applied** one, kept inside the limits, which moves the link. `angle()` reads the applied angle, `commandedAngle()` the commanded one.

A URDF file describes a robot as such a chain: `compages::world::loadUrdf(world, path)` builds it headless; the Renderer adds the visuals when it loads the same file (`08d_RobotArm`).

## ✍ Who may move an entity: PoseDriver

Three writers can move an entity: the kinematics (from its joint), an animation clip (from the Renderer), and your code (a physics engine, a controller, the network). Without a rule they would write over each other. The rule without a `PoseDriver`: a joint wins over a clip, and your code may always write. With one, only the named writer touches the pose:

| `PoseDriver::source` | Who writes the local pose |
|----------------------|---------------------------|
| `Kinematic` | the kinematics, from the joint |
| `Animation` | the animation, from the playing clip |
| `External` | your code only: no system of Compages writes it |

This is how a physics engine plugs in without Compages having one: [the guide](../guides/plug-a-physics-engine.md).

## 📦 What lives in the World, and what does not

In `include/Compages/World/Components/`: **Camera** (lens, viewport, `sees_helpers`; the view comes from the inverse of the entity's world matrix), **Light** (colour, intensity; the sun shines along the forward axis of its entity), **SkinInstance** (the pose of a skeleton), **PoseDriver**.

Not in the World: meshes and materials (`MeshRenderer`, in the Renderer, holding asset ids), the files (the `AssetManager`), draw calls (`gpu::`), the window. A world component never holds a GPU pointer. This is why `world.update()` runs in a unit test with no context at all.

## 📏 Two ways to place a thing, and the cost of a routine

- Two ways to place a thing (your component, or the transform) can confuse at first: a `Position` struct alone does not move the graph. Copy it into the transform if you want both.
- A `Routine` is one virtual call per entity per frame: fine for hundreds, not for a million. For a million, use components and `each`, or keep the data on the GPU ([where does my data live?](three-layers.md#-where-does-my-data-live)).

## 🔗 See also

- [Make it move](../tutorials/03-the-world.md): the arm of the workshop, joint by joint.
- [Write routines](../guides/write-routines.md): fixed steps, timers, messages, order.
- [One frame](one-frame.md): when the routines, the joints and the matrices run.
