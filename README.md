<p align="center"><img src="doc/logos/compages-layers.png" alt="Compages" width="420"></p>

# 🧱 Compages

**A C++20 3D library you can take one layer at a time: draw with the GPU in a few lines, simulate a world without any screen, or build a full 3D scene like in Three.js. Each layer works without the ones above it.**

*Compages* is a Latin word: an assembly of parts that lock together into one whole. Here, three layers: GPU, world and renderer.

---

## 🤔 What is it for?

A 3D program does two different jobs:

- **Simulate**: things exist and move. A robot arm turns its joints, particles fall, a character walks. This is numbers in memory: a unit test or a planner needs it, without any window.
- **Show**: a camera looks at this world and the graphics card draws it, sixty times per second.

Most 3D engines mix the two and give you everything at once: a window, an editor, a scene format, a physics engine. You wanted to draw a curve, and you got a game engine. Compages does the opposite. It is three stacked layers, and **you stop at the first one that does your job**:

| Layer | When you have… | You write… | Inspired by |
|-------|----------------|------------|------------|
| 🟧 **GPU** | a shader and arrays of numbers | `drawable["position"] = list_of_points; drawable.draw();` | [Glumpy](https://glumpy.github.io/) |
| 🟦 **World** | things that move, even with no screen | `world.entity("Ship").child("Gun")`, your structs as components | [Three.js](https://threejs.org/) + an ECS |
| 🟩 **Renderer** | a world you want to see | `scene.box("Cube", color(1, 0, 0)); scene.draw(frame);` | [Three.js](https://threejs.org/) |

- 🌌 A galaxy simulated on the GPU needs only the GPU layer.
- 🤖 A robot planner, a server or a unit test needs only the World layer: no OpenGL context at all.
- 🎮 A game or a robot simulator uses the three, and the types stay separate.

---

## 📖 The story

It started with [SimTaDyn](https://github.com/Lecrapouille/SimTaDyn), as student project, where I wanted to draw **maps**. I needed an OpenGL layer.

With the old OpenGL (the "legacy" one, `glBegin` / `glEnd`), drawing was simple. Then OpenGL 3.3 *core* arrived: buffers, vertex arrays, attribute locations, shaders to compile and link, uniforms to look up by hand… dozens of calls before the first triangle, and one wrong call gives a black window with no message. **I never managed to get it right.**

Then I found [Glumpy](https://glumpy.github.io/), in Python, and it was a revelation: the **shader names its data**, you fill `program["position"]`, you call `draw()`, and the copies between the CPU and the GPU are hidden. A whole OpenGL application in a few lines. I wrote a C++ version of this idea: **OpenGLCppWrapper**. But it hit walls:

- 🔁 **No ping-pong.** GPU simulations draw into a texture, then read it in the next pass while writing into another one, and swap (the Game of Life, reaction-diffusion, particles). My copy of the Glumpy model could not express it.
- 🌳 **No real scene graph.** I wanted to build scenes like in Three.js, `ship.child("Gun").position(0, 0.5, 0)`, and my API made it painful with materials paradoxically because of the abstraction layer.
- 🧟 **Unity-style scripts, without Unity's inheritance.** I liked Unity's `MonoBehaviour` (a small script attached to an object, `Start()` once, `Update()` every frame), but not the deep class hierarchies that come with it, with virtual calls everywhere and data scattered in memory.

The project stopped for several years. With the help of LLMs, I rebuilt it from scratch as **Compages**, and fixed what had blocked me:

- ⚡ the GPU layer sends everything the CPU changed **in one pass** at `draw()`, with no virtual call and no `gl*` in your code;
- 🧬 the world is an **ECS** ([EnTT](https://github.com/skypjack/entt)): an entity is an id, your data are plain structs packed in memory, and loops over thousands of entities stay **cache friendly**;
- 🌳 the **scene graph** sits on the same entities, with the chained style of Three.js;
- 🎬 **behaviors** give the comfort of `MonoBehaviour`, but they are optional small scripts attached to an entity, not a class tree you must inherit from;
- 🔁 **ping-pong**, render-to-texture and compute shaders are first-class: the gallery has a Game of Life, a reaction-diffusion and an eight-thousand-star galaxy.

---

## ✨ Why Compages?

| | 🧱 Compages | Raw OpenGL | Glumpy | Three.js | Unity / Godot | bgfx / sokol |
|---|---|---|---|---|---|---|
| Language | C++20 | C | Python | JavaScript | C# / GDScript | C / C++ |
| Triangle in a few lines | ✅ the shader names the data | ❌ dozens of calls | ✅ | ✅ | ✅ (in the editor) | ❌ you describe every buffer |
| CPU → GPU copies hidden | ✅ one flush at `draw()` | ❌ | ✅ | ✅ | ✅ | ❌ |
| Compute shaders, ping-pong | ✅ | ✅ (by hand) | partly | WebGL limits | ✅ | ✅ (by hand) |
| Scene graph in chained calls | ✅ | ❌ | ❌ | ✅ | ✅ (editor) | ❌ |
| ECS, cache-friendly data | ✅ | — | ❌ | ❌ | Unity DOTS (separate) | — |
| Simulation without any screen | ✅ a whole layer for it | — | ❌ | ❌ | partly | — |
| Use only the part you need | ✅ three independent layers | — | ✅ | ❌ | ❌ | ✅ |
| A library, not an editor | ✅ your window, your loop | ✅ | ✅ | ✅ | ❌ | ✅ |

I know of no other C++ library that puts these together: the **Glumpy** way of talking to the GPU, the **Three.js** way of building scenes, an **ECS** world that runs **headless**, and **Unity**-like behaviors, as three layers you can use one at a time.

What this gives you:

- 🔬 **Scientists** draw and compute on the GPU without learning the OpenGL state machine.
- 🤖 **Robotics** gets a world that runs in unit tests and CI, and the same world drawn when you want to see it. [RobotIK](https://github.com/Lecrapouille/RobotIK) uses Compages for its world and its simulator.
- 🎮 **Games** get Three.js-short scene code, glTF models, animations and cameras, in C++.
- 🧑‍🏫 **Learners** get a gallery of more than forty small demos, each adding one idea, with its source shown next to the picture.

---

## 👀 What it looks like

### 🟧 GPU — a triangle from a shader

Here is a hello-world example drawing a single triangle. The shader declares `position` and `color`; C++ fills them by name; `draw()` sends what changed.

```cpp
compages::gpu::Drawable triangle;

// At setup
triangle.load(VERTEX_SHADER, FRAGMENT_SHADER);   // GLSL: in vec2 position; in vec3 color;
triangle["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };
triangle["color"]    = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

// Every frame
compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
triangle.draw();
```

<img src="doc/images/01b_Triangle.png" alt="Triangle" width="360"> <img src="doc/images/13_Galaxy.png" alt="Galaxy" width="360">

The same door leads to simulations: in `13_Galaxy`, 8000 stars attract each other in a compute shader, and the drawable reads the buffer the computation just wrote. This screenshot on the right shows initial step of the galaxy simulation.

### 🟦 World — things that move, without any screen

```cpp
struct Velocity { compages::core::Vector3f value; };      // your data: a plain struct

compages::world::World world;
compages::world::Entity ship = world.entity("Ship").set(Velocity{ { 1, 0, 0 } });
ship.child("Gun").position(0.0f, 0.5f, 0.0f);             // the gun follows the ship

world.each<Velocity>([&](compages::world::Entity e, Velocity& v) {   // a system: a plain loop
    e.position(e.position() + v.value * step.elapsed);
});
world.update(step);                                       // behaviors, joints, matrices
```

This runs in a unit test, with no OpenGL. Joints too: `.revolute(axis, -90.0_deg, 90.0_deg).angle(30.0_deg)`, and the children follow.

### 🟩 Renderer — a picture of that world

```cpp
compages::renderer::Scene scene(world);

// At setup
scene.camera().position(0.0f, 1.0f, 3.0f).add<compages::world::Orbit>();
scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
compages::world::Entity cube = scene.box("Cube", compages::renderer::color(0.9f, 0.18f, 0.12f));

// Every frame
cube.rotate(frame.elapsed, { 0.4f, 1.0f, 0.0f });
scene.draw(frame);
```

<img src="doc/images/50_ThreeJsLike.png" alt="Cube" width="360"> <img src="doc/images/53_DoomLike.png" alt="Doom-like" width="360">

`scene.load(...)` reads some glTF files used for games but also files such as STL and URDF used in robotic (with their joints). The gallery goes up to a small Doom-like game or basic robot forward kinematic.

---

## 🚀 Quick start

Details, Fedora, CI without screen, system install: [doc/02-Getting-Started.md](doc/02-Getting-Started.md).

You need Linux, a C++20 compiler, Make, CMake, Git, `pkg-config`, GLFW and an **OpenGL 4.5** driver. **Note:** macOS stops at OpenGL 4.1 I will see how to manage this.

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git

cd Compages

make download-external-libs
make compile-external-libs
make -j"$(nproc)"

./build/Compages-examples   # the gallery: every demo, its source, GPU counters
```

Keyboard keys: `PgUp` / `PgDn` change demo, `F1` hides the panels.

---

## 📚 Documentation

Start with chapters 1 to 3, then open the chapter of the layer you need.

| # | Chapter | Contents |
|---|---------|----------|
| | [doc/README.md](doc/README.md) | 🧭 Reading order and quick paths |
| 1 | [Introduction](doc/01-Introduction.md) | 🌱 3D words, the three layers and why they are separate |
| 2 | [Getting started](doc/02-Getting-Started.md) | 🚀 Install, build, gallery, headless CI, system install |
| 3 | [Your first programs](doc/03-Tutorial.md) | 🎓 One program per layer, line by line |
| 4 | [The GPU layer](doc/04-GPU-Layer.md) | 🟧 Drawable, buffers, compute, several passes |
| 5 | [The World layer](doc/05-World-Layer.md) | 🟦 Scene graph, ECS, behaviors, joints |
| 6 | [The Renderer layer](doc/06-Renderer-Layer.md) | 🟩 Scene, assets, frame, live textures |
| 7 | [Architecture](doc/07-Architecture.md) | 🏗 Folders, includes, one frame |
| 8 | [API reference](doc/08-API-Reference.md) | 📖 The calls of the three layers |
| 9 | [Examples](doc/09-Examples.md) | 🖼 The gallery and its keys |
| 10 | [Debugging](doc/10-Debugging.md) | 🐞 Errors, driver messages, apitrace, RenderDoc |
| 11 | [Credits](doc/11-Credits.md) | 🙏 Inspirations, third parties |

Coding agents: [AGENTS.md](AGENTS.md) and the skills of `.claude/skills/`.

---

## 📝 License

Compages is dual-licensed: GNU GPL v3 or later, or a commercial license for those who cannot use the GPL. See [LICENSING.md](LICENSING.md).
