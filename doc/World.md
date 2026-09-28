# Second Layer — World Layer

The second layer answers a simple question: **Where do things exist in the world, and how do they move, without interacting with the graphics card?** This is the "headless" simulation—unit tests, planners, servers—using a chained API style similar to Three.js and data iteration reminiscent of an Entity Component System (ECS).

**Public interfaces:** `compages::world::World` and `compages::world::Entity`
**Headers:** `<Compages/World/Entity.hpp>` (chaining), `<Compages/World/World.hpp>` (loops, `update`)
**Source code:** `include/Compages/World/`, `src/World/`
**Not present in this layer:** any `gpu::` types, meshes, or windows.

The commented program can be found in [Tutorial.md](Tutorial.md). Below, we describe the two key mechanisms, followed by examples (behaviors, joints, and what lies outside this layer).

## Two mechanisms, one container

| Concept   | API                        | What it's for                                   |
|-----------|----------------------------|-------------------------------------------------|
| Container | `World`                    | the source of truth                             |
| Handle    | `Entity`                   | for chained calls, like an `Object3D`           |
| Identity  | `EntityId`                 | 32-bit generational int, can be stored anywhere |
| Parenting | `child()`, `lookup("A/B")` | the spatial graph of entities in the world      |
| Placement | `.position()`, `.rotate()`, `.scale()` | the transform of entities in the graph   |
| Data      | `.set(T{})`, `.add<T>(...)`, `.get<T>()` | your structs, the ECS                 |
| System    | `world.each<T...>()`       | loop over entities with those components        |
| Step      | `world.update(frame)`      | behaviors, joints, matrix updates               |
| Local logic | `Behavior`               | `update(dt)` running on a given entity          |

The graph and the ECS are complementary:

- The graph maintains parent-child relationships, and thus computes world matrices. This spatial structure is used across all game engines (Godot, Unity, Unreal Engine, etc.)—when the parent node moves, so do its children.
- The ECS defines what data a loop can access. An entity may use just one or both approaches.

## Minimal program

```cpp
#include <Compages/World/Entity.hpp>

struct Velocity { Vector3f value; };

compages::world::World world;
compages::world::Entity ship = world.entity("Ship").set(Velocity{ { 1, 0, 0 } });
ship.child("Gun").position(0.0f, 0.5f, 0.0f);

compages::world::Frame step;
step.elapsed = 1.0f / 60.0f;

world.each<Velocity>([&](compages::world::Entity e, Velocity& v) {
    e.position(e.position() + v.value * step.elapsed);
});
world.update(step);

compages::world::Entity gun = world.lookup("Ship/Gun");
```

Here, `position()` refers to the **graph transform**, not to a user-defined component. In `30_HeadlessWorld` the movement instead uses custom `Position` and `Velocity` structs, and `each` only "sees" an entity if it has both. A rock, with no velocity, is ignored. Choose the mechanism that fits your scenario: particle integration via your structs, mechanical linkage via the graph. Both can coexist.

`Entity` is a handle. Copying it does not duplicate the entity. An empty handle, or one referring to a destroyed entity, evaluates false in `if (entity)`.

## Behaviors

```cpp
struct Spin : compages::world::Behavior
{
    explicit Spin(float speed) : speed(speed) {}
    void update(float dt) override { transform().rotateY(speed * dt); }
    float speed;
};

ship.add<Spin>(2.0f);
```

Inspired by Unity's MonoBehavior: `World::update(Frame)` calls `start()` once on new behaviors, then calls `update(dt)` every frame, in the order they were added. An entity may hold several behaviors. The simulation API (`entity()`, `transform()`, `world()`, `input()`, `frame()`) is directly accessible. Disabled entities, and their descendants, are skipped. This structure brings flexibility compared to ECS, but presents typical OOP issues: vtables and cache misses.

`51_Behaviors` demonstrates attaching three simple behaviors to three cubes (spin, float, "breathe" while space is pressed). The camera controller `Orbit` is also a behavior.

## Joints (Articulations)

Same hierarchical graph, but used for robotics (articulated chains). Here, a child has a joint rather than full freedom:

```cpp
using namespace units::literals;

compages::world::Entity arm = ship.child("Arm")
    .position(0, 0, 0.5f)
    .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
arm.angle(30.0_deg);
```

The argument-less `update` (as called by `update(Frame)`) runs: `KinematicSystem` sets the local transform from the joint angle, then `TransformSystem` propagates parent → children. Rotating a joint moves everything below it. `31_MovingRobot` builds three robots from boxes in this manner; `38_RobotArm` loads a URDF (IRB 2400): one entity per link, a `revolute` joint between each, and gallery sliders to set joint angles within their limits. This is straightforward forward kinematics, still independent of rendering—the demo just uses a `Scene` to visualize the arm.

`prismatic` is the linear (translation) joint; `offset()` is its driver. A fixed joint is simply the local transform.

## What lives in World

In `include/Compages/World/Components/`: simulation data.

- **Camera** — fov, near/far planes. The view at render time is the entity's world matrix.
- **Light** — color, intensity. The sun's direction matches the entity's forward axis.
- **SkinInstance** — bone ids and pose matrices, for renderer skinning.

## What lives elsewhere

| Element                 | Where                             |
|-------------------------|-----------------------------------|
| `MeshRenderer`, `Animator` | `compages::renderer`           |
| glTF files, textures, materials | `AssetManager`             |
| Draw calls              | in `gpu::`, invoked by the renderer|
| Window, GLFW            | the application, or `examples/Common/Window.cpp` |

Saving a scene (`35_PrefabAndSave`) bridges both: the world and the asset catalog. The JSON stores asset **names** (not GPU ids).

## `update` and `draw`

```text
Headless:   world.update(frame);

Displayed:  scene.draw(viewFrame);
               World::update(frame)     behaviors
               AnimationSystem          clips → local transforms
               World::update()          joints, matrices
               skinning                 pose to SkinInstance
               extraction + drawing
```

Animation clips are a concern of the **renderer**, because the clips are stored in the asset catalog. `Scene` applies them between the two passes of `World::update`. A headless world has no clips until a `Scene` loads them in.

`Frame` contains `elapsed`, `total`, and viewport size. `ViewFrame` adds keyboard and mouse input, used by behaviors and `Scene::draw`. You can create a `Frame` from a `ViewFrame` when input is not needed for the simulation.

## Demo List

| Demo                | Idea                                       |
|---------------------|--------------------------------------------|
| `30_HeadlessWorld`  | ECS, behavior, `lookup`, no GPU            |
| `31_MovingRobot`    | hierarchy and joints                       |
| `32c_MiscLookAt`    | `lookAt` on a thousand transforms          |
| `38_RobotArm`       | URDF, kinematics                           |
| `51_Behaviors`      | multiple behaviors, input                  |

## Next Steps

- [Renderer.md](Renderer.md) — visualizing this world.
- [Philosophy.md](Philosophy.md) — why headless simulation is the primary approach.
