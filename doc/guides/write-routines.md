# 🎬 Write routines

[⏫ Guides](README.md)

Attach behaviour to one entity (a door, a robot arm, a camera) with code that runs at each step: at a fixed pace, after the matrices, later, or when a message comes.

![07b_Routines](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07b_Routines.png)

A **routine** is code attached to one entity and run at each step of the World, like a `MonoBehaviour` in Unity. The workshop arm has one ([make it move](../tutorials/03-the-world.md)). Headers: `World/Routines/Routine.hpp` and `World/Routines/RoutineSystem.hpp`, both included by `World/Entity.hpp`. Times are `compages::si::Seconds`; the suffixes `0.5_s`, `1.2_rad_per_s` need `using namespace compages::si::literals;` ([units](../tutorials/00-glossary.md#-units)).

## 🔄 1. Code at each frame

```cpp
struct Spin: compages::world::Routine
{
    explicit Spin(compages::si::AngularVelocity p_speed) : speed(p_speed) {}
    void update(compages::si::Seconds p_dt) override { transform().rotateY(speed * p_dt); }
    compages::si::AngularVelocity speed;   // 1.2_rad_per_s: speed * p_dt is an angle
};

ship.add<Spin>(2.0f);               // two radians a second; the arguments go to the constructor
ship.get<Spin>().speed = 1.0f;      // find it again: get<T>(), find<T>(), has<T>()
ship.remove<Spin>();
```

Inside a routine: `entity()`, `transform()`, `world()`, `input()` (keys and mouse), `frame()` (time). An entity may hold several routines.

## 🌱 2. Its life

| Function | Called |
|----------|--------|
| `onEnable()` | when it becomes active: added to an enabled entity, entity enabled again, `enabled(true)` |
| `start()` | once, at the first step where it is active |
| `onDisable()` | when it stops being active, or before `onDestroy()` |
| `onDestroy()` | once, before it is removed or its entity or world destroyed; the entity is still alive |

`routine.enabled(false)` switches off one routine; `entity.enable(false)` switches off the entity, its children and all their routines. A change takes effect at the next step.

## ⏲ 3. A fixed pace: fixedUpdate

`update(dt)` receives the time of the frame, which changes. A controller, a physics engine or a behaviour tree wants the same step each time. `fixedUpdate(dt)` is called 0, 1 or more times per step, with a constant `dt`:

```cpp
world.routines().fixedStep(0.25_s);   // fixedUpdate() at 4 Hz (1/60 s by default)
```

After a very slow frame, at most 8 run: the World drops the time it cannot catch up with instead of freezing.

## 🧭 4. After the matrices: lateUpdate

`update` runs before the joints and the world matrices are computed; `lateUpdate(dt)` after them. A camera that follows an object, a sensor that reads a pose, want it. The World then updates the matrices of what `lateUpdate` moved.

## 🔢 5. Order: priority

Routines run by `priority()` (smaller first), then in the order they were added, whatever their entity. A run is the same each time.

```cpp
[[nodiscard]] int priority() const override { return -1; }   // before the others
```

## ⏱ 6. Later: timers

```cpp
after(2.0_s, [this] { entity().destroy(); });       // once, in two seconds
TimerId blink = every(0.5_s, [this] { flip(); });   // every half second
cancel(blink);
```

Timers count the time of the steps where the routine is active: they wait while it is disabled.

## ✉ 7. Messages

Any value can be sent to the routines of an entity; they receive it in `onMessage`, at once, if they are active:

```cpp
struct Hit { float damage; };
world.routines().send(guard, Hit{ 10.0f });   // or broadcast(Hit{ … }) to every entity

void onMessage(compages::world::Message const& p_message) override
{
    if (Hit const* hit = p_message.as<Hit>()) { life -= hit->damage; }
}
```

## ⏱ All together

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

## 🧯 What a routine is not

- **No `onCollision`.** Compages has no physics engine, so it detects no contact. The outside engine sends a `compages::world::Contact` message ([plug a physics engine](plug-a-physics-engine.md)).
- Not a behaviour tree. A routine can host one (BehaviorTree.CPP, BlackThorn…): it owns the tree, ticks it in `fixedUpdate`, resets it in `onDisable`. The actions of the tree write commands (a joint angle, a speed), not poses.
- Not for a million things. A routine is an object with virtual calls. For thousands of entities, use components and `world.each`; for millions, keep the data on the GPU ([where does my data live?](../concepts/three-layers.md#-where-does-my-data-live)).

`07b_Routines` puts three routines on three cubes. The camera controls `Orbit`, `Fly` and `Rig` are routines too ([move the camera](move-the-camera.md)).

## 🔗 See also

[◀ The World](../concepts/the-world.md) · [One frame](../concepts/one-frame.md)
