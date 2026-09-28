# Philosophy

Compages exists to let you write **the minimum code** at the right level. A particle simulation does not need a scene graph. A robot planner does not need a window. A game needs all three — but in order, and with types that remain clearly separated.

The project targets **scientific simulations** (fields, automata, n-body, curves) first, then **games** that reuse the same objects in space. Both have the same constraint: the public surface of each layer should remain small. The rest of the API exists for special cases encountered in the demo gallery, but the goal is to reduce what a new program must name.

## Three Layers

```text
                    your application
                    window, input, time
                              │
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
   renderer::Scene      world::World        gpu::Drawable
   show                 simulate           compute / draw
```

| Question you’re asking         | Layer     | What this layer leaves to others               |
|-------------------------------|-----------|-----------------------------------------------|
| I have GLSL and arrays        | GPU       | entities, glTF files, window                  |
| I have bodies that move, tests included | World     | draw calls, mesh catalog                      |
| I want an image of this world | Renderer  | the simulation itself — it stays in `World`   |

`World::update` is the simulation step. `Scene::draw` calls it, then photographs the world for the GPU. Headless mode is not just an afterthought: it's simply layer 2, used by itself.

## Layer 1 — The Shader is the API

[Glumpy](https://glumpy.github.io/) showed that you can write, in Python:

```python
program['position'] = array
program.draw()
```

Compages adopts this approach ergonomically in C++20, with `gpu::Drawable`:

- The shader declares `in vec2 position`. The C++ code writes `drawable["position"] = …`. The name is the contract.
- Assignment affects the **CPU copy**. The backend is only involved on `draw()` (or via explicit `upload()` if you manage the buffer yourself).
- Public headers contain no `GLenum`. The OpenGL 4.5 backend (Direct State Access) is replaceable: the build variable `GPU_BACKEND` selects the implementation. Currently only `GL45` exists.

Glumpy alone would duplicate vertices every time a mesh was sent through multiple shaders (shadow, depth, outline). Compages keeps the Glumpy style for "one-shader" cases, but adds an explicit **vertex layout on the C++ side** (`Buffer<Vertex>`, `Pipeline`) checked by each program. An attribute ignored by a shader is fine; a name or type mismatch is an error. The demo `05a_MultiPassMesh` demonstrates: a single buffer, three pipelines.

## Layer 2 — The World Exists Without an Image

Previous project model: a node tree with a virtual `onDraw()` method, buffers attached to nodes, and reparenting that became a lifetime headache.

Current model: two mechanisms within a single `World`:

- **A spatial graph.** Parent, children, local transform, world matrices recalculated from parent to children. The fluent interface (`entity("Ship").child("Gun").position(...)`) is reminiscent of Three.js.
- **An ECS** ([EnTT](https://github.com/skypjack/entt)). Data is stored as structs. `world.each<Velocity>(...)` only sees entities with that component. The identity inside a component is a generational `EntityId`, like a GPU handle.

A `Behavior` is a small piece of code attached to an entity (`start()` once, `update(dt)` repeatedly). Joints (`revolute`, `prismatic`) live on children: `World::update` resolves them, then propagates transforms.

Camera, lights, and skin pose are **simulation data**. The view matrix is not a hidden field of `Camera`: it’s taken from the world matrix of the entity, read when the renderer needs it.

## Layer 3 — Rendering Without Owning the Simulation

`renderer::Scene` is a façade: `box`, `sphere`, `camera`, `sun`, `load`, `draw`. You can express things in a single line, just like a Three.js tutorial.

Under the hood, there are three jobs:

1. **The catalog** (`AssetManager`) — meshes, textures, materials, animation clips. Identifiers are generational. A pure world component never holds a `gpu::` pointer.
2. **Render components** — `MeshRenderer`, `Animator`. These live in `compages::renderer`, attached to `World` entities.
3. **The snapshot** — `SceneExtractor` copies cameras, lights, transforms, and asset IDs into a `RenderSnapshot` that no longer references the world. Drawing uses the snapshot. The world can advance while an earlier image is still being processed (the data contract already allows this).

Multiple `Scene` objects can share a single `World` and `AssetManager`: for split screen, minimaps (`32a_SplitViews`), etc.

## Two Times for Errors

| When      | What happens         | Where to read it         |
|-----------|---------------------|--------------------------|
| Load (shader, file) | the call fails immediately | `compages::Result`, macro `COMPAGES_TRY` |
| Frame (unknown name, wrong type) | first error is kept, others counted | `gpu::takeFrameError()`, `Scene::prepare()` |

Checking every assignment on every frame would swamp a simulation. `prepare()`, at the end of setup, surfaces these errors before your loop runs. The gallery shows the error message as an overlay.

## OpenGL 4.5, and What Lies Beneath

- **4.3** brings compute and storage buffers: particles, n-body, indirect draw.
- **4.5** brings Direct State Access: the backend configures objects without binding them first.
- **macOS** tops out at OpenGL 4.1. The 4.5 backend isn’t honest there; CPU tests pass, the graphics gallery is limited. The `Backends/` folder exists to allow future backends.
- The **Web** would need a WebGPU backend for compute; WebGL 2 alone doesn’t cover this layer.

The gallery shows three kinds of computation: texture ping-pong (`11a_GameOfLife`), compute and SSBO (`12_ComputeParticles`, `13_Galaxy`), indirect draw powered by compute (`21_IndirectDraw`).

## What the Library Leaves Outside

Compages is a library. The window (GLFW, SDL, Qt) is the application’s concern: `gpu::init` receives a function loader once the context is ready. The editor, asset store, and the production pipeline for a large game are left outside. The legacy physics (ReactPhysics3D) is in `attic/Physics/`, not in the main build, waiting to come back as components and a `world::` system.

## Further Reading

- [Tutorial.md](Tutorial.md) — write all three kinds of program.
- [Architecture.md](Architecture.md) — folders, includes, a frame from start to finish.
- [Credits.md](Credits.md) — Glumpy, SimTaDyn, Three.js, Bevy.
