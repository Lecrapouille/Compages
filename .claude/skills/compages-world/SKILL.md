---
name: compages-world
description: Write or change Compages World-layer code (compages::world): World/Entity/EntityId, scene graph (child, lookup, transforms, world matrices), ECS components and each<> systems, Routine, revolute/prismatic joints, Frame/ViewFrame, controllers (Orbit, Fly), headless simulation and tests. Use for anything that moves without needing the GPU.
---

# Compages World layer

Headless simulation: no `gpu::init`, no window. Headers
`<Compages/World/Entity.hpp>` (chaining) and `<Compages/World/World.hpp>`.
Vectors and `Frame` are in `compages` (not global).

```cpp
struct Position { compages::Vector3f value{ 0, 0, 0 }; };   // your structs = components
struct Velocity { compages::Vector3f value{ 0, 0, 0 }; };

compages::world::World world;
world.entity("player").set(Position{}).set(Velocity{ { 1, 0, 0 } });
compages::world::Entity ship = world.entity("Ship").position(2, 0, 0);
ship.child("Gun").position(0, 0.5f, 0);                  // graph: child follows parent

compages::Frame step;  step.elapsed = 1.0f / 60.0f;  // you fill elapsed/total
world.each<Position, Velocity>([&](compages::world::Entity, Position& p, Velocity& v)
    { p.value += v.value * step.elapsed; });             // system: entities with BOTH
world.update(step);                                      // routines, joints, matrices
world.lookup("Ship/Gun");                                // path lookup; Entity is a handle, if(e) tests it
```

- Components: `.set(T{})`, `.add<T>(args)`, `.get<T>()`, `.has<T>()`, `.remove<T>()`.
  Transform (graph) ≠ your `Position` component: `.position()`, `.rotate(rad, axis)`,
  `.scale()`, `.lookAt()`, `.worldMatrix()`, `.worldPosition()`.
- Routine (`World/Routines/`, ex-Behavior; never call it Behavior: BlackThorn
  behavior trees): `struct Spin: compages::world::Routine { void update(float dt) override; }`,
  `entity.add<Spin>(2.0f)`. Life: `onEnable`, `start` (once), `fixedUpdate`
  (`world.routines().fixedStep(s)`, max 8 per step), `update`, timers
  (`after/every/cancel`), `lateUpdate` (after the matrices), `onDisable`,
  `onDestroy` (entity still alive). Order: `priority()` then order of addition.
  `enabled(bool)` per routine. Messages: `world.routines().send(e, value)` →
  `onMessage(m)`, `m.as<T>()`; `Contact` = shock from an outside engine (no
  `onCollision`). `RoutineSystem` holds them; `World` is not movable. Access
  `entity()`, `transform()`, `world()`, `input()`, `frame()`. Prefer `each`
  for many entities. Tutorial: doc/05-World-Layer.md §3.
- Joints (`using namespace units::literals;`):
  `child.revolute({0,0,1}, -90.0_deg, 90.0_deg)` then `child.joint().angle(30.0_deg)`
  (`World/Kinematics/JointHandle.hpp`; no limits = continuous),
  `prismatic(axis, 0.0_m, 0.3_m)` then `.joint().offset(0.1_m)`. `joint().angle()`
  is the applied value (inside the limits), `commandedAngle()` the one set.
  `update()` runs `KinematicSystem` then `TransformSystem` (loops, no recursion).
- Who writes a pose: `PoseDriver { Kinematic, Animation, External }`
  (`World/Components/PoseDriver.hpp`). A physics engine is outside Compages: it
  marks its bodies `External` and writes their pose each step (`07e_ExternalPhysics`).
  `world.step(frame)` runs the routines, `world.update()` the matrices,
  `world.endStep()` lateUpdate; `world.update(frame)` does the three.
- URDF headless: `compages::world::loadUrdf(world, path)` (`World/Import/`).
- `ViewFrame` = `Frame` + keyboard/mouse, for routines such as `Orbit`, `Fly`
  (`World/Controllers/Controls.hpp`); `FPSController` is applied, not a routine.
- Camera, Light, SkinInstance components are simulation data
  (`World/Components/`); the view matrix is the camera entity world matrix.

## Rules

- `world` includes only `Core`: no `gpu::` type, no mesh, no window here.
  Meshes/materials/clips belong to `compages::renderer`.
- Store `EntityId` (generational) rather than raw pointers.
- A headless test checks numbers, not pixels (`07a_HeadlessWorld` reports with
  `gpu::reportError` inside the gallery only).

Quick check without the gallery (verified):
`g++ -std=c++20 x.cpp $(PKG_CONFIG_PATH=build/linux-gcc-debug/lib/pkgconfig pkg-config --cflags Compages) build/linux-gcc-debug/lib/libCompages.a`.

Reference: doc/05-World-Layer.md, doc/03-Tutorial.md §2.
