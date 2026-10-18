---
name: compages-world
description: Write or change Compages World-layer code (compages::world): World/Entity/EntityId, scene graph (child, lookup, transforms, world matrices), ECS components and each<> systems, Behavior, revolute/prismatic joints, Frame/ViewFrame, controllers (Orbit, Fly), headless simulation and tests. Use for anything that moves without needing the GPU.
---

# Compages World layer

Headless simulation: no `gpu::init`, no window. Headers
`<Compages/World/Entity.hpp>` (chaining) and `<Compages/World/World.hpp>`.
Vectors and `Frame` are in `compages::core` (not global).

```cpp
struct Position { compages::core::Vector3f value{ 0, 0, 0 }; };   // your structs = components
struct Velocity { compages::core::Vector3f value{ 0, 0, 0 }; };

compages::world::World world;
world.entity("player").set(Position{}).set(Velocity{ { 1, 0, 0 } });
compages::world::Entity ship = world.entity("Ship").position(2, 0, 0);
ship.child("Gun").position(0, 0.5f, 0);                  // graph: child follows parent

compages::core::Frame step;  step.elapsed = 1.0f / 60.0f;  // you fill elapsed/total
world.each<Position, Velocity>([&](compages::world::Entity, Position& p, Velocity& v)
    { p.value += v.value * step.elapsed; });             // system: entities with BOTH
world.update(step);                                      // behaviors, joints, matrices
world.lookup("Ship/Gun");                                // path lookup; Entity is a handle, if(e) tests it
```

- Components: `.set(T{})`, `.add<T>(args)`, `.get<T>()`, `.has<T>()`, `.remove<T>()`.
  Transform (graph) ≠ your `Position` component: `.position()`, `.rotate(rad, axis)`,
  `.scale()`, `.lookAt()`, `.worldMatrix()`, `.worldPosition()`.
- Behavior: `struct Spin: compages::world::Behavior { void update(float dt) override; }`,
  `entity.add<Spin>(2.0f)`; `start()` once; access `entity()`, `transform()`,
  `world()`, `input()`, `frame()`; skipped when `enable(false)`. Prefer `each`
  for many entities (virtual calls, cache misses).
- Joints (`using namespace units::literals;`):
  `child.revolute({0,0,1}, -90.0_deg, 90.0_deg).angle(30.0_deg)` (clamped to
  limits; no limits = continuous), `prismatic(axis, 0.0_m, 0.3_m).offset(0.1_m)`.
  `update()` runs `KinematicSystem` then `TransformSystem`.
- `ViewFrame` = `Frame` + keyboard/mouse, for behaviors such as `Orbit`, `Fly`
  (`World/Controllers/Controls.hpp`); `FPSController` is applied, not a behavior.
- Camera, Light, SkinInstance components are simulation data
  (`World/Components/`); the view matrix is the camera entity world matrix.

## Rules

- `world` includes only `Core`: no `gpu::` type, no mesh, no window here.
  Meshes/materials/clips belong to `compages::renderer`.
- Store `EntityId` (generational) rather than raw pointers.
- A headless test checks numbers, not pixels (`30_HeadlessWorld` reports with
  `gpu::reportError` inside the gallery only).

Quick check without the gallery (verified):
`g++ -std=c++20 x.cpp -I include -isystem external/units/include -isystem external/entt/src build/libCompages.a`.

Reference: doc/05-World-Layer.md, doc/03-Tutorial.md §2.
