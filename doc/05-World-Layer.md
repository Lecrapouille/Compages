# 🟦 5. The World layer

The second layer answers one question: **where are things, and how do they move, without touching the graphics card?** This is headless simulation: unit tests, planners, servers, robots. In this page you learn the two mechanisms of a `World` (the scene graph and the ECS), behaviors, joints, and where the line with the Renderer is.

| | |
|---|---|
| **Entry points** | `compages::world::World`, `compages::world::Entity` |
| **Headers** | `<Compages/World/Entity.hpp>` (chaining), `<Compages/World/World.hpp>` (loops, `update`) |
| **Code** | `include/Compages/World/`, `src/World/` |
| **Not in this layer** | any `gpu::` type, meshes, windows |

The commented programs are in [chapter 3 §2](03-Tutorial.md#-2-a-world-without-a-screen).

---

## 🧩 1. Two mechanisms, one container

| Idea | API | For |
|------|-----|-----|
| Container | `World` | the source of truth |
| Handle | `Entity` | chained calls, like a Three.js `Object3D` |
| Identity | `EntityId` | a 32-bit generational number, safe to store anywhere |
| Parenting | `child()`, `lookup("A/B")` | the scene graph |
| Placement | `.position()`, `.rotate()`, `.scale()`, `.lookAt()` | the transform in the graph |
| Data | `.set(T{})`, `.add<T>(...)`, `.get<T>()`, `.has<T>()`, `.remove<T>()` | your structs: the ECS |
| System | `world.each<T...>(fn)` | a loop over the entities that have these components |
| Step | `world.update(frame)` | behaviors, joints, world matrices |
| Local logic | `Behavior` | `update(dt)` on one entity |

The two mechanisms help each other:

- The **scene graph** keeps the parents and children, and so computes the world matrices. Every game engine has one (Godot, Unity, Unreal): when the parent moves, its children follow.
- The **ECS** ([EnTT](https://github.com/skypjack/entt)) decides which data a loop sees. An entity may use one mechanism, the other, or both.

---

## ✨ 2. A small program

```cpp
#include <Compages/World/Entity.hpp>

struct Velocity { compages::core::Vector3f value; };

compages::world::World world;
compages::world::Entity ship = world.entity("Ship").set(Velocity{ { 1, 0, 0 } });
ship.child("Gun").position(0.0f, 0.5f, 0.0f);

compages::core::Frame step;
step.elapsed = 1.0f / 60.0f;

world.each<Velocity>([&](compages::world::Entity e, Velocity& v) {
    e.position(e.position() + v.value * step.elapsed);
});
world.update(step);

compages::world::Entity gun = world.lookup("Ship/Gun");
```

Here `position()` is the **transform** of the graph, not a component of yours. `30_HeadlessWorld` moves its entities with its own `Position` and `Velocity` structs instead, and `each` sees an entity only if it has both. Choose what fits: particle integration with your structs, mechanical links with the graph. Both can live together.

`Entity` is a handle: copying it does not copy the entity. An empty handle, or one whose entity was destroyed, is `false` in `if (entity)`. `entity.enable(false)` switches an entity and its descendants off.

---

## 🎬 3. Behaviors

```cpp
struct Spin: compages::world::Behavior
{
    explicit Spin(float p_speed) : speed(p_speed) {}
    void update(float p_dt) override { transform().rotateY(speed * p_dt); }
    float speed;
};

ship.add<Spin>(2.0f);    // built with Spin(2.0f)
```

Like Unity's `MonoBehaviour`: `World::update(Frame)` calls `start()` once on new behaviors, then `update(dt)` at every step, in the order they were added. An entity may hold several behaviors. Inside, you reach `entity()`, `transform()`, `world()`, `input()` and `frame()`. Disabled entities and their descendants are skipped.

Behaviors are flexible but pay the price of objects: virtual calls and scattered memory. For thousands of entities, prefer a system (`each`).

`51_Behaviors` puts three behaviors on three cubes (spin, float, "breathe" while Space is pressed). The camera controller `Orbit` is a behavior too.

---

## 🦾 4. Joints

The same graph, used for robots: a child gets a joint instead of full freedom.

```cpp
using namespace units::literals;

compages::world::Entity arm = ship.child("Arm")
    .position(0, 0, 0.5f)
    .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
arm.angle(30.0_deg);
```

`update` (through `update(Frame)`) runs `KinematicSystem`, which sets the local transform from the joint value, then `TransformSystem`, which propagates from parents to children. Turning a joint moves everything below it. The value stays inside the limits (see the tutorial output in [chapter 3](03-Tutorial.md#step-2--a-joint-and-a-behavior)).

| Joint | Set with | Example |
|-------|----------|---------|
| revolute (turns) | `.revolute(axis, min, max)`, then `.angle(a)` | `wheel.revolute({ 1, 0, 0 })`: no limits, a continuous joint |
| prismatic (slides) | `.prismatic(axis, min, max)`, then `.offset(o)` | `slider.prismatic({ 0, 0, 1 }, 0.0_m, 0.3_m).offset(0.1_m)` |
| fixed | the local transform | — |

`31_MovingRobot` builds three robots from boxes this way. `38_RobotArm` loads a URDF (ABB IRB 2400): one entity per link, one revolute joint between each, and sliders to set the angles within their limits. This is forward kinematics, still independent of the rendering: the demo only uses a `Scene` to look at the arm.

---

## 📦 5. What lives in the World

In `include/Compages/World/Components/`, simulation data:

- **Camera**: field of view, near and far planes. The view used for drawing is the world matrix of the entity.
- **Light**: color, intensity. The direction of the sun is the forward axis of its entity.
- **SkinInstance**: bone ids and pose matrices, for skinning by the renderer.

And what lives elsewhere:

| Element | Where |
|---------|-------|
| `MeshRenderer`, `Animator` | `compages::renderer` |
| glTF files, textures, materials | `AssetManager` (Renderer) |
| draw calls | `gpu::`, called by the renderer |
| window, GLFW | your application, or `examples/Common/Window.cpp` |

Saving a scene (`35_PrefabAndSave`) touches both: the world and the asset catalog. The JSON keeps asset **names**, not GPU ids.

---

## ⏰ 6. `update` and `draw`

```text
Headless:   world.update(frame);

With a picture:  scene.draw(viewFrame);
                    World::update(frame)     behaviors
                    AnimationSystem          clips -> local transforms
                    World::update()          joints, matrices
                    skinning                 pose -> SkinInstance
                    extraction + drawing
```

Animation clips belong to the **renderer**, because they live in the asset catalog: `Scene` applies them between the two passes of `World::update`. A headless world has no clip until a `Scene` loads one.

| Type | Holds |
|------|-------|
| `compages::core::Frame` | `elapsed`, `total`, `width`, `height` — enough for a headless step |
| `compages::world::ViewFrame` | a `Frame` plus keyboard and mouse, for behaviors and `Scene::draw` |

You fill `elapsed` (and `total`) yourself in a headless loop.

---

## 🧭 7. Demos of this layer

| Demo | Idea |
|------|------|
| `30_HeadlessWorld` | ECS, behavior, `lookup`, no GPU |
| `31_MovingRobot` | hierarchy and joints |
| `32c_MiscLookAt` | `lookAt` on a thousand transforms |
| `38_RobotArm` | URDF, forward kinematics |
| `51_Behaviors` | several behaviors, input |

Next: [06-Renderer-Layer.md](06-Renderer-Layer.md) — a picture of this world.
