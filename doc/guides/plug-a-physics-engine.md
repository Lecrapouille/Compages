# ⚙ Plug a physics engine

[⏫ Guides](README.md)

Let MuJoCo, Bullet, Jolt or your own engine move the entities, and Compages draw them.

![07e_ExternalPhysics](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07e_ExternalPhysics.png)

Compages has **no physics engine**, and will not get one: a real engine does this job better. The engine keeps its bodies; Compages keeps the poses and draws them. Three steps.

### 📄 1. Tell Compages who moves the entity

A `PoseDriver` set to `External` means: no system of Compages writes this pose, only your code.

```cpp
#include <Compages/World/Components/PoseDriver.hpp>

ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });
```

**2. Step the engine at a fixed pace**, in the `fixedUpdate` of a routine, and copy its result into the entities:

```cpp
struct Physics: compages::world::Routine
{
    void fixedUpdate(float p_dt) override
    {
        engine.step(p_dt);                            // the engine moves its bodies
        auto const p = engine.position(body);         // its result goes into the entity
        ball.position(p.x, p.y, p.z);
        for (auto const& c : engine.contacts())       // its shocks go to the routines
        {
            world().routines().send(ball, compages::world::Contact{ wall, c.point, c.normal, c.speed });
        }
    }
    // ... the engine, the ball, the wall
};
```

**3. React to contacts** in the routines of each body:

```cpp
void onMessage(compages::world::Message const& p_message) override
{
    if (auto const* contact = p_message.as<compages::world::Contact>()) { /* react */ }
}
```

`scene.draw(frame)` (or `world.update(frame)` headless) then runs the routine, computes the matrices and draws. Without a routine, step the engine between `World::step()` and `World::update()`: the routines have run, the matrices are not computed yet.

`07e_ExternalPhysics` does this with a tiny engine of a few lines (gravity, bounces, balls against balls), written without any Compages type; a `Squash` routine on each ball swells at a shock. RobotIK does the same with MuJoCo.

### 🟦 Very large worlds

Keep the simulation in `double`, outside Compages, and write into the World the positions relative to a moving origin ([very large scenes](very-large-scenes.md#-keep-the-simulation-precise)).

## 🔗 See also

[The World: who may move an entity](../concepts/the-world.md#-who-may-move-an-entity-posedriver) · [Write routines](write-routines.md)
