# 🟦 5. The World layer

The second layer answers one question: **where are things, and how do they move, without touching the graphics card?** This is headless simulation: unit tests, planners, servers, robots. In this page you learn the two mechanisms of a `World` (the scene graph and the ECS), routines, joints, and where the line with the Renderer is.

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
| Step | `world.update(frame)` | routines, joints, world matrices |
| Local logic | `Routine` | `update(dt)` on one entity |

The two mechanisms help each other:

- The **scene graph** keeps the parents and children, and so computes the world matrices. Every game engine has one (Godot, Unity, Unreal): when the parent moves, its children follow.
- The **ECS** ([EnTT](https://github.com/skypjack/entt)) decides which data a loop sees. An entity may use one mechanism, the other, or both.

---

## ✨ 2. A small program

```cpp
#include <Compages/World/Entity.hpp>

struct Velocity { compages::Vector3f value; };

compages::world::World world;
compages::world::Entity ship = world.entity("Ship").set(Velocity{ { 1, 0, 0 } });
ship.child("Gun").position(0.0f, 0.5f, 0.0f);

compages::Frame step;
step.elapsed = 1.0f / 60.0f;

world.each<Velocity>([&](compages::world::Entity e, Velocity& v) {
    e.position(e.position() + v.value * step.elapsed);
});
world.update(step);

compages::world::Entity gun = world.lookup("Ship/Gun");
```

Here `position()` is the **transform** of the graph, not a component of yours. `07a_HeadlessWorld` moves its entities with its own `Position` and `Velocity` structs instead, and `each` sees an entity only if it has both. Choose what fits: particle integration with your structs, mechanical links with the graph. Both can live together.

`Entity` is a handle: copying it does not copy the entity. An empty handle, or one whose entity was destroyed, is `false` in `if (entity)`. `entity.enable(false)` switches an entity and its descendants off.

---

## 🎬 3. Routines

A **routine** is code attached to one entity and run at each step of the world, like a `MonoBehaviour` in Unity. This section builds one step by step; each step adds one function. The headers: `World/Routines/Routine.hpp` and `World/Routines/RoutineSystem.hpp` (both come with `World/Entity.hpp`).

### Step 1: code at each frame

Derive from `compages::world::Routine`, override `update`, and add it to an entity like a component. The arguments of `add` go to the constructor.

```cpp
struct Spin: compages::world::Routine
{
    explicit Spin(float p_speed) : speed(p_speed) {}
    void update(float p_dt) override { transform().rotateY(speed * p_dt); }
    float speed;
};

ship.add<Spin>(2.0f);               // two radians a second
ship.get<Spin>().speed = 1.0f;      // find it again: get<T>(), find<T>(), has<T>()
ship.remove<Spin>();
```

Inside a routine you reach `entity()`, `transform()`, `world()`, `input()` (keys and mouse) and `frame()` (timing). An entity may hold several routines.

### Step 2: the life of a routine

| Function | Called |
|----------|--------|
| `onEnable()` | when the routine becomes active: added to an enabled entity, entity enabled again, `enabled(true)` |
| `start()` | once, at the first step where it is active |
| `onDisable()` | when it stops being active: entity disabled, `enabled(false)`, or before `onDestroy()` |
| `onDestroy()` | once, before it is removed, its entity destroyed, or its world destroyed; the entity is still alive |

`routine.enabled(false)` switches off one routine and leaves the others of the entity running; `entity.enable(false)` switches off the entity, its children and all their routines. A change takes effect at the next step.

### Step 3: a fixed pace with `fixedUpdate`

`update(dt)` receives the time of the frame, which changes from frame to frame. A controller, a physics engine or a behavior tree wants the same step each time, so that a run is the same on a slow and on a fast computer. `fixedUpdate(dt)` is called 0, 1 or more times per step, with a constant `dt`:

```cpp
world.routines().fixedStep(0.25f);   // fixedUpdate() at 4 Hz (1/60 s by default)
```

After a very slow frame, at most 8 `fixedUpdate()` run: the world drops the time it cannot catch up with instead of freezing.

### Step 4: after the matrices with `lateUpdate`

`update` runs before the joints and the world matrices are computed. `lateUpdate(dt)` runs after them: what a camera following an object, or a sensor reading a pose, needs. The world then updates the matrices of what `lateUpdate` moved (nothing to do if it moved nothing).

### Step 5: order with `priority`

Routines run by `priority()` (smaller first), then in the order they were added, whatever their entity. A run is the same each time.

```cpp
struct Walk: compages::world::Routine
{
    void update(float p_dt) override { transform().position.x += p_dt; }
    [[nodiscard]] int priority() const override { return -1; }   // before the others
};
```

### Step 6: later, with timers

```cpp
after(2.0f, [this] { entity().destroy(); });       // once, in two seconds
TimerId blink = every(0.5f, [this] { flip(); });   // every half second
cancel(blink);
```

Timers count the time of the steps where the routine is active: they wait while it is disabled.

### Step 7: talking with messages

Any value can be sent to the routines of an entity; they receive it in `onMessage`, right away, if they are active. `as<T>()` reads it back.

```cpp
struct Hit { float damage; };
world.routines().send(guard, Hit{ 10.0f });   // or broadcast(Hit{ ... }) to every entity

void onMessage(compages::world::Message const& p_message) override
{
    if (Hit const* hit = p_message.as<Hit>()) { life -= hit->damage; }
}
```

### All together

A routine with every function, a second one moving the entity first, two steps of 0.3 s at a fixed pace of 0.25 s, a message, then the entity destroyed:

```text
step 0
onEnable
start
  fixedUpdate 0.250
  update 0.300
  lateUpdate, x = 0.3
step 1
  fixedUpdate 0.250       0.05 s left from step 0 + 0.3 s: one fixed step
  update 0.300
  every 0.5 s             the timer set in start()
  lateUpdate, x = 0.6     Walk ran first (priority -1), the matrix is up to date
onMessage: hit 10
onDisable
onDestroy
```

The order of a step:

```text
world.update(frame)
  step       onEnable / onDisable / start, fixedUpdate x N, update, timers
  update     joints -> local places, world matrices
  endStep    lateUpdate, world matrices of what lateUpdate moved
```

### Collisions: no `onCollision`

Unity calls `OnCollisionEnter` because it has a physics engine. Compages has none ([section 5](#plug-in-a-physics-engine)), so it detects no contact. The outside engine finds the contacts and sends a `compages::world::Contact` (the other body, the point, the normal, the speed) to the routines of each body:

```cpp
world.routines().send(a, compages::world::Contact{ b, point, normal, speed });

void onMessage(compages::world::Message const& p_message) override
{
    if (auto const* contact = p_message.as<compages::world::Contact>()) { /* react */ }
}
```

`07e_ExternalPhysics` does exactly this: a `Physics` routine steps a small engine in `fixedUpdate` and sends the shocks; a `Squash` routine on each ball swells at a shock.

### A behavior tree is not a routine

A routine can **host** a behavior tree (BlackThorn, BehaviorTree.CPP…): it owns the tree and its blackboard, calls `tick()` in `fixedUpdate`, resets it in `onDisable` and frees it in `onDestroy`. The actions of the tree write commands (the angle of a joint, a speed), not poses: the joints and `PoseDriver` keep one writer per pose. Compages ships no behavior tree.

### Routines or systems?

Routines are flexible but pay the price of objects: virtual calls and scattered memory. For thousands of entities, prefer a system (`each`, [section 1](#-1-two-mechanisms-one-container)). For millions, keep the data on the GPU: see the table [Where does my data live?](01-Introduction.md#where-does-my-data-live).

`07b_Routines` puts three routines on three cubes (spin, float, "breathe" while Space is pressed). The camera controllers `Orbit`, `Fly` and `Rig` are routines too.

`Rig` is the camera of a strategy game or of a map: the left button drags the ground, the right button turns, the wheel zooms toward the point under the mouse (that point stays under the mouse). To jump to another place, do not move the target: call `flyTo`. The camera then goes up, across and down along the path of van Wijk and Nuij (the path of a map viewer). It starts and stops with no speed and no acceleration, so there is no jolt; the farther the place, the higher it goes. Any input of the user stops the flight where it is (`10e_Archipelago`):

```cpp
compages::world::RigController& rig = camera.get<compages::world::Rig>().controller;
rig.flight_pace = 0.8f;                                       // seconds per unit of path
rig.flyTo(compages::Vector3f(4200.0f, 0.0f, -3100.0f), 3200.0f);  // target, distance (m)
if (rig.flying()) { /* ... */ }
```

---

## 🦾 4. Joints

The same graph, used for robots: a child gets a joint instead of full freedom.

```cpp
using namespace units::literals;

compages::world::Entity arm = ship.child("Arm")
    .position(0, 0, 0.5f)
    .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
arm.joint().angle(30.0_deg);
```

`update` (through `update(Frame)`) runs `KinematicSystem`, which sets the local transform from the joint value, then `TransformSystem`, which propagates from parents to children. Turning a joint moves everything below it.

`entity.joint()` gives a `JointHandle` (`World/Kinematics/JointHandle.hpp`): it sets and reads the joint value. `.revolute()` and `.prismatic()` stay on `Entity`, to chain while building.

A joint keeps two values. The **commanded** value is the one you set; it may be outside the limits. The **applied** value is the commanded one kept inside the limits: it is the one that moves the link. `arm.joint().angle()` gives the applied angle, `arm.joint().commandedAngle()` the commanded one (`offset()` and `commandedOffset()` for a slider). In the component, `state.position.value` is the command and `state.position.applied()` the effect. A zero axis gives a joint that does not move.

| Joint | Set with | Example |
|-------|----------|---------|
| revolute (turns) | `.revolute(axis, min, max)`, then `.joint().angle(a)` | `wheel.revolute({ 1, 0, 0 })`: no limits, a continuous joint |
| prismatic (slides) | `.prismatic(axis, min, max)`, then `.joint().offset(o)` | `slider.prismatic({ 0, 0, 1 }, 0.0_m, 0.3_m).joint().offset(0.1_m)` |
| fixed | the local transform | — |

`07c_MovingRobot` builds three robots from boxes this way. `08d_RobotArm` loads a URDF (ABB IRB 2400): one entity per link, one revolute joint between each, and sliders to set the angles within their limits. This is forward kinematics, still independent of the rendering: the demo only uses a `Scene` to look at the arm.

A URDF describes a robot, not only its look, so the chain belongs to the World: `compages::world::loadUrdf(world, path)` (`World/Import/UrdfLoader.hpp`) builds it headless. `parseUrdf` reads the file without creating anything, and `buildUrdf` builds it with a callback for each link: the renderer uses that callback to add the visuals.

---

## 🎯 5. Who writes the pose: `PoseDriver`

Three writers can move an entity: the `KinematicSystem` (from its joint), the `AnimationSystem` (from a clip of the renderer) and your own code (a physics engine, a robot controller, the network). Without a rule, they write over each other.

The rule without `PoseDriver`: a joint wins over a clip, and your code may always write. With the component, only the named writer touches the pose:

| `PoseDriver::source` | Who writes the local pose |
|----------------------|---------------------------|
| `Kinematic` | the `KinematicSystem`, from the joint |
| `Animation` | the `AnimationSystem`, from the playing clip |
| `External` | your code only: no system of Compages writes it |

### Plug in a physics engine

Compages has **no physics engine**, and will not get one: MuJoCo, Bullet, Jolt or your own does this job better. The engine keeps its bodies; Compages keeps the poses and draws them. The best place to step the engine is the `fixedUpdate` of a routine ([section 3](#-3-routines)): a constant step, whatever the frame rate.

```cpp
#include <Compages/World/Components/PoseDriver.hpp>

// Once: the engine owns the pose of the ball.
ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });

struct Physics: compages::world::Routine
{
    void fixedUpdate(float p_dt) override
    {
        engine.step(p_dt);                            // 1. the engine moves its bodies
        auto const p = engine.position(body);         // 2. its result goes into the entity
        ball.position(p.x, p.y, p.z);
        for (auto const& c : engine.contacts())       // 3. its shocks go to the routines
        {
            world().routines().send(ball, compages::world::Contact{ wall, c.point, c.normal, c.speed });
        }
    }
    // ... the engine, the ball, the wall
};
```

`scene.draw(viewFrame)` (or `world.update(frame)` headless) then runs the routine, computes the matrices and draws. Without a routine, step the engine between `World::step()` and `World::update()`: the routines have run, the matrices are not computed yet.

`07e_ExternalPhysics` does this with a tiny engine of a few lines (gravity, bounces, balls against balls), written without any Compages type. RobotIK does the same with MuJoCo.

---

## 📦 6. What lives in the World

In `include/Compages/World/Components/`, simulation data:

- **Camera**: field of view, near and far planes. The view used for drawing is the world matrix of the entity.
- **Light**: color, intensity. The direction of the sun is the forward axis of its entity.
- **SkinInstance**: bone ids and pose matrices, for skinning by the renderer.
- **PoseDriver**: who writes the pose ([section 5](#-5-who-writes-the-pose-posedriver)).

And what lives elsewhere:

| Element | Where |
|---------|-------|
| `MeshRenderer`, `Animator` | `compages::renderer` |
| glTF files, textures, materials | `AssetManager` (Renderer) |
| draw calls | `gpu::`, called by the renderer |
| window, GLFW | your application, or `examples/Common/Window.cpp` |

Saving a scene (`08c_PrefabAndSave`) touches both: the world and the asset catalog. The JSON keeps asset **names**, not GPU ids.

---

## ⏰ 7. `update` and `draw`

```text
Headless:   world.update(frame);       step + update + endStep

With a picture:  scene.draw(viewFrame);
                    World::step(frame)       routines: fixedUpdate, update, timers
                    AnimationSystem          clips -> local transforms
                    World::update()          joints, matrices
                    World::endStep()         lateUpdate, matrices of what it moved
                    skinning                 pose -> SkinInstance
                    extraction + drawing
```

Animation clips belong to the **renderer**, because they live in the asset catalog: `Scene` applies them after the routines and before the single `World::update()`. A headless world has no clip until a `Scene` loads one.

| Type | Holds |
|------|-------|
| `compages::Frame` | `elapsed`, `total`, `width`, `height` — enough for a headless step |
| `compages::world::ViewFrame` | a `Frame` plus keyboard and mouse, for routines and `Scene::draw` |

You fill `elapsed` (and `total`) yourself in a headless loop.

---

## 🧭 8. Demos of this layer

The folder `07_World/` follows this chapter.

| Demo | Idea |
|------|------|
| `07a_HeadlessWorld` | ECS, routine, `lookup`, no GPU |
| `07b_Routines` | several routines, input |
| `07c_MovingRobot` | hierarchy and joints |
| `07d_AnimatedModel` | hips and shoulders as joints |
| `07e_ExternalPhysics` | an outside engine in `fixedUpdate` writes the poses (`PoseDriver::External`) and sends `Contact` messages |
| `06k_MiscLookAt` | `lookAt` on a thousand transforms |
| `08d_RobotArm` | URDF, forward kinematics |

Next: [06-Renderer-Layer.md](06-Renderer-Layer.md) — a picture of this world.
