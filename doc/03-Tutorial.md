# 🎓 3. Your first programs

In this page you write three programs, one per layer: a triangle drawn from a shader, a world that moves without any screen, and a 3D scene in ten lines. Go only as far as the first layer that does your job.

The first and third programs are demos of the gallery: open them with `./build/Compages-examples 01b_Triangle` and `./build/Compages-examples 50_ThreeJsLike`, and read their source in the **Source** panel. The second one runs in a terminal; its output below is real.

---

## 🔌 0. Before any drawing: the GPU device

Layers 1 and 3 need an OpenGL 4.5 core context, then a call to `compages::gpu::init`. **Compages does not open a window for you**: your application (or the gallery) does.

```cpp
#include <Compages/GPU/GPU.hpp>

if (auto ready = compages::gpu::init(glfwGetProcAddress); !ready)
{
    std::cerr << ready.error() << '\n';
    return 1;
}

// ... frames ...

compages::gpu::shutdown();
```

`glfwGetProcAddress` is one example; SDL and Qt give their own function. Layer 2 needs none of this.

---

## 🔺 1. A triangle named by the shader

📁 `examples/00_GettingStarted/01b_Triangle.cpp` · more in [chapter 4](04-GPU-Layer.md)

The vertex shader declares two attributes, `position` and `color`. These names are all the C++ side needs to know:

```cpp
constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec2 position;
in vec3 color;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";
```

Setup, once:

```cpp
compages::gpu::Drawable m_triangle;

// The one step that can fail for a reason outside the program: a shader
// that does not compile. COMPAGES_TRY hands the compiler log to the caller.
COMPAGES_TRY(m_triangle.load(VERTEX_SHADER, FRAGMENT_SHADER));

// One value per vertex, attribute by attribute.
m_triangle["position"] = { { -0.8f, -0.6f },
                           { 0.8f, -0.6f },
                           { 0.0f, 0.8f } };
m_triangle["color"] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

// Optional: checks everything now rather than at the first frame.
return m_triangle.prepare();
```

Every frame:

```cpp
compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
m_triangle.draw();      // sends what changed, then draws
```

![Triangle](images/01b_Triangle.png)

What happened:

- `load` compiled the shaders and read the names they declare.
- `m_triangle["position"] = …` filled the **CPU mirror**. The values of the two attributes are stored **interleaved** (position, color, position, color…), the layout the GPU reads best.
- `draw()` sent the changed bytes to the GPU, then drew. No `glBindBuffer`, no attribute location.

A wrong name, or a `vec2` given three numbers, does not crash: the message is kept and lists what the shader really declares. `prepare()` returns it before your loop; during the loop, read it with `compages::gpu::takeFrameError()` ([chapter 10](10-Debugging.md)).

When your vertices are already a struct, the field names must match the shader. `01c_InterleavedTriangle` sends them once, in a buffer that never changes and has no CPU copy:

```cpp
struct Vertex
{
    compages::core::Vector2f position;
    compages::core::Vector3f color;
};

COMPAGES_TRY_ASSIGN(m_vertices,
                    compages::gpu::Buffer<Vertex>::from(
                        corners,
                        { .usage = compages::gpu::BufferUsage::Immutable,
                          .cpu_mirror = false }));
m_triangle.vertices(m_vertices);
```

📌 Remember: **the shader names, C++ fills, `draw()` sends.** Next steps in the gallery: `02_DynamicGeometry` (only the moved vertex is sent), then `10_ScientificAndCompute/` (Game of Life, particles, galaxy).

---

## 🌍 2. A world without a screen

📁 `examples/30_WorldAndAssets/30_HeadlessWorld.cpp` · more in [chapter 5](05-World-Layer.md)

No `gpu::init` here, no window, no OpenGL. The container is `compages::world::World`. An `Entity` is a handle that you chain, like a Three.js object.

### Step 1 — components and a system

Create `world.cpp`:

```cpp
#include <Compages/World/Entity.hpp>
#include <iostream>

struct Position { compages::core::Vector3f value{ 0, 0, 0 }; };
struct Velocity { compages::core::Vector3f value{ 0, 0, 0 }; };

int main()
{
    compages::world::World world;

    // The rock has a position but no velocity: the loop below skips it.
    world.entity("player").set(Position{}).set(Velocity{ { 1, 0, 0 } });
    world.entity("rock").set(Position{ { 0, 5, 0 } });

    compages::core::Frame step;
    step.elapsed = 1.0f / 60.0f;

    for (int i = 0; i < 60; ++i)
    {
        world.each<Position, Velocity>(
            [&](compages::world::Entity, Position& p, Velocity& v)
            { p.value += v.value * step.elapsed; });
        world.update(step);
    }

    compages::world::Entity player = world.lookup("player");
    compages::core::Vector3f const& x = player.get<Position>().value;
    std::cout << "player after 1 s: " << x.x << " " << x.y << " " << x.z << "\n";

    // The scene graph: a child follows its parent.
    compages::world::Entity ship = world.entity("Ship").position(2, 0, 0);
    compages::world::Entity gun = ship.child("Gun").position(0, 0.5f, 0);
    world.update(step);
    std::cout << "Ship/Gun found: " << (world.lookup("Ship/Gun") ? "yes" : "no") << "\n";
    return 0;
}
```

Build it from the root of your clone, against the static library, and run it:

```sh
g++ -std=c++20 world.cpp -I include \
    -isystem external/units/include -isystem external/entt/src \
    build/libCompages.a -o world
./world
```

```text
player after 1 s: 1 0 0
Ship/Gun found: yes
```

- `each<Position, Velocity>` is a **system**: a loop over the entities that have **both** components. The rock is never visited.
- `update(step)` runs the behaviors, the joints, then computes the world matrices.
- `Position` is **your** struct. The place of an entity in the scene graph (what `.position(x, y, z)` changes) is its **transform**. A simulation may use one, the other, or both.

### Step 2 — a joint and a behavior

A revolute joint turns a child around an axis, within limits. A behavior is code attached to one entity, called by `update`:

```cpp
#include <Compages/World/Entity.hpp>
#include <iostream>

using namespace units::literals;

//! A behavior: a small piece of code attached to one entity.
struct Spin: compages::world::Behavior
{
    explicit Spin(float p_speed) : speed(p_speed) {}
    void update(float p_dt) override { transform().rotateY(speed * p_dt); }
    float speed;
};

int main()
{
    compages::world::World world;

    // A base, a shoulder joint turning around x, and a hand 1 m above it.
    compages::world::Entity base = world.entity("Robot");
    compages::world::Entity shoulder = base.child("Shoulder")
        .position(0.0f, 0.0f, 0.5f)
        .revolute({ 1, 0, 0 }, -90.0_deg, 90.0_deg);
    compages::world::Entity hand = shoulder.child("Hand").position(0.0f, 0.0f, 1.0f);
    world.entity("Fan").add<Spin>(2.0f);

    compages::core::Frame step;
    step.elapsed = 1.0f / 60.0f;

    for (auto angle : { 0.0_deg, 90.0_deg, 120.0_deg })
    {
        shoulder.angle(angle);
        world.update(step);
        compages::core::Vector3f p = hand.worldPosition();
        std::cout << "angle " << angle.value() << " deg -> hand at ("
                  << p.x << ", " << p.y << ", " << p.z << ")\n";
    }
    return 0;
}
```

```text
angle 0 deg -> hand at (0, 0, 1.5)
angle 90 deg -> hand at (0, -1, 0.5)
angle 120 deg -> hand at (0, -1, 0.5)
```

- You set the angle; `update` computes the local transform of the shoulder, and the hand follows: this is **forward kinematics**, still without any screen.
- 120° is outside the limits: the joint stays at 90°.

📌 This is how the layer is tested: `30_HeadlessWorld` fails if the player did not move, with zero pixels to look at.

---

## 🧊 3. A picture of the world

📁 `examples/50_Complete/50_ThreeJsLike.cpp` · more in [chapter 6](06-Renderer-Layer.md)

The gallery gives the demo a `compages::world::World m_world` and a `compages::renderer::Scene m_scene{ m_world }`. Setup:

```cpp
// Background, camera, light, shape: the whole scene.
m_scene.background(0.04f, 0.05f, 0.08f);
// Orbit starts from wherever the camera is: place it first.
m_scene.camera().position(0.0f, 1.0f, 3.0f).add<compages::world::Orbit>();
m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
m_cube = m_scene.box("Cube", compages::renderer::color(0.9f, 0.18f, 0.12f));
return m_scene.prepare();
```

Every frame:

```cpp
// The cube turns; the orbit behavior turns the camera from the mouse.
m_cube.rotate(p_frame.elapsed, { 0.4f, 1.0f, 0.0f });
m_scene.draw(p_frame);
```

![Cube](images/50_ThreeJsLike.png)

- `box` returns an entity of the **world**: you move it like any other entity. The mesh and the material are renderer components.
- `Orbit` is a behavior that reads the mouse from the `ViewFrame` (a `Frame` plus keyboard and mouse). Right drag turns, the wheel zooms.
- `draw` updates the world, then draws it.

Other looks, same call:

```cpp
m_scene.sphere("Ball", compages::renderer::color(1.0f, 0.3f, 0.2f));
m_scene.box("Crate", compages::renderer::texture("wooden-crate.jpg"));
```

Texture files are searched in `external/Compages-data/`, or in `COMPAGES_DATA_PATH`.

---

## 📎 4. Which header to include

| Program | Include |
|---------|---------|
| shaders or compute only | `<Compages/GPU/GPU.hpp>` |
| simulation: `World`, `each`, `update` | `<Compages/World/World.hpp>` |
| simulation with chaining `entity().child()` | `<Compages/World/Entity.hpp>` |
| scene and pictures | `<Compages/Compages.hpp>` |

Next: the chapter of your layer — [4. GPU](04-GPU-Layer.md), [5. World](05-World-Layer.md) or [6. Renderer](06-Renderer-Layer.md).
