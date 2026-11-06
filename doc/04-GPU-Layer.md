# 🟧 4. The GPU layer

The first layer speaks **shader**. You write GLSL, fill the data with the names the shader declares, and call `draw()`. In this page you learn the naming contract, typed vertices, errors, compute shaders, and the tools for the cases where one `Drawable` is not enough.

| | |
|---|---|
| **Entry point** | `compages::gpu::Drawable` |
| **Header** | `<Compages/GPU/GPU.hpp>` |
| **Code** | `include/Compages/GPU/`, implementation in `src/GPU/` |
| **Not in this layer** | the window, entities, model files |

The commented triangle is in [chapter 3 §1](03-Tutorial.md#-1-a-triangle-named-by-the-shader).

![Particles: a compute shader writes the buffer, the draw reads it](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05d_ComputeParticles.png)

---

## 🔌 1. Create the device

```cpp
#include <Compages/GPU/GPU.hpp>

if (auto ready = compages::gpu::init(glfwGetProcAddress); !ready)
    return fail(ready.error());

// ... frames ...

compages::gpu::shutdown();
```

`init` loads the OpenGL functions. The OpenGL 4.5 core context must already be current.

---

## 🔖 2. The naming contract

```cpp
compages::gpu::Drawable mesh;
COMPAGES_TRY(mesh.loadSources(vertex_src, fragment_src));

mesh["position"] = positions;                                // attribute: one value per vertex
mesh["tint"]     = compages::Vector3f{ 1, 0.5f, 0.2f }; // uniform
mesh["image"]    = texture;                                  // sampler: the texture must outlive the drawable
mesh["time"]     = elapsed;                                  // uniform

compages::gpu::clear({ 0.08f, 0.09f, 0.12f });
mesh.draw();
```

`operator[]` does not guess: the shader already classified each name as an attribute, a uniform or a sampler. The same syntax works for the three.

- Attributes given one by one are **stored interleaved** (position, color, position, color…): the layout the GPU reads best.
- Assigning an attribute marks a **dirty range**; `draw()` sends only what was marked. In `02b_DynamicGeometry`, moving the top corner sends one vertex out of three. `02a_CpuGpuSync` shows the same on a raw `Buffer`, with orange bars for the bytes the GPU does not have yet (there you call `upload()` yourself).
- The dirty range is **one range**, from the first change to the last: changing element 1 and element 5000 sends everything between them, in one copy.
- Uniforms are written at each draw. A texture gets a unit and is bound only for the draw.

---

## 🧬 3. Typed vertices

When your data is already a struct, the field names must match the attribute names. The drawable keeps the vertices on the CPU and sends only what changed:

```cpp
struct Vertex
{
    compages::Vector2f position;
    compages::Vector3f color;
};

mesh.setVertices<Vertex>({
    { { -0.8f, -0.6f }, { 1, 0, 0 } },
    { {  0.8f, -0.6f }, { 0, 1, 0 } },
    { {  0.0f,  0.8f }, { 0, 0, 1 } },
});
mesh.setVertices(corners);                          // or any std::vector, std::array, span

mesh.editVertex<Vertex>(2u).position.y = 0.5f;      // marked, sent at the next draw
```

`editVertex(i)` (one), `editVertices(first, count)` (a run), `editVertices()` (all) and `emplace_back(v)` mark what changed; the next draw sends only that (`02b_DynamicGeometry`). `01f_VertexSyntax` shows every way side by side.

### One rule: who owns the vertices

**The drawable owns its vertices. Borrow a `Buffer` when the vertices are made on the GPU (a compute pass), shared by several drawables, or large and never changed (a `Static` buffer keeps no CPU copy).**

| The vertices... | Call | Who keeps them | Demo |
|-----------------|------|----------------|------|
| are given or edited from C++ | `d.setVertices(data)`, `d.editVertex<V>(i)`, `d.emplace_back(v)` | the drawable, on the CPU and the GPU | `01c`, `02a`, `02b`, `10a`, `15` |
| are given one attribute at a time | `d["position"] = { ... }` | the drawable | `01b` |
| are made on the GPU, shared, or large and fixed | `d.borrowVertices(buffer)` | your `Buffer`, which must outlive the drawable | `01d`, `05a`, `12`, `13`, `16` |

### Buffers

What the GPU uses a buffer for is part of its type, so a mistake such as drawing indices from a buffer of vertices does not compile:

| Type | Holds |
|------|-------|
| `VertexBuffer<T>` | corners of shapes |
| `IndexBuffer<T>` | the order to join them (`std::uint16_t` or `std::uint32_t`) |
| `StorageBuffer<T>` | data a compute pass reads and writes; can also be drawn as vertices |

How it lives is chosen at creation, with the OpenGL names, and never changes. There is no default:

| `BufferUsage` | CPU copy | Written by | For |
|---------------|----------|------------|-----|
| `Static` | no | nobody after creation (a compute pass may write a `StorageBuffer`) | a mesh read from a file |
| `Stream` | no | `write()`, often whole; the CPU keeps its own data | skinned vertices, debug lines |
| `Dynamic` | yes | `buffer[i] = …`, `emplace_back()`, then `upload()` sends only what changed | a trail, numbers the CPU and the GPU share |

```cpp
COMPAGES_TRY_ASSIGN(m_particles,
    compages::gpu::StorageBuffer<Particle>::from(start, compages::gpu::BufferUsage::Static));
m_points.borrowVertices(m_particles);   // read where it is: no copy

compages::gpu::VertexBuffer<Vertex> trail;   // empty and Dynamic: grows as needed
trail.emplace_back(Vertex{ p, red });
COMPAGES_TRY(trail.upload());
```

---

## 🚨 4. Errors

| When | Mechanism |
|------|-----------|
| a shader does not compile | the `Status` of `load`, often through `COMPAGES_TRY` |
| unknown name, wrong type, empty draw | a frame message |

```cpp
const std::string err = compages::gpu::takeFrameError();   // "" when nothing happened
```

Only the first message is kept; `frameErrorCount()` says how many came after. `prepare()` at the end of the setup runs these checks before the main loop. `setBreakOnError(true)` stops the debugger on the faulty line. More in [chapter 10](10-Debugging.md).

---

## 🧮 5. Compute

A compute shader does not draw pixels: you give it a grid. The same `Buffer` can be the storage block it writes and the vertices of the next draw, with no copy through the CPU. This is `05d_ComputeParticles`:

```cpp
COMPAGES_TRY(step.dispatchItems(count));
compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);
points.draw();
```

> ⚡ `barrier` is **not** added after a dispatch for you: the GPU may run ahead. A missing barrier gives a race, not a compile error. Combine the flags with `|` (`VertexAttrib`, `Storage`, `Command`, `All`… in `GPU/Enums.hpp`).

- `05a_Compute` is the smallest one: an array of numbers, then `download()` brings the result back to the CPU (orange bars while the CPU copy is late).
- `05f_Galaxy`: two ping-pong buffers, eight thousand stars, tiles in work group memory (see below).
- `05e_SpiralGalaxy`, inspired by Glumpy, needs **no compute shader**: each body rides a fixed ellipse, so its position is a formula of the time. The vertex shader computes it; the vertices are sent once and each frame writes one uniform, `years`. When the next state depends on the previous one (gravity, a Game of Life), the state must be stored: that is the compute shader and the ping-pong below.

### 🏓 Ping-pong: read one copy, write the other

A parallel simulation must not read and write the same state at the same time: one cell (a star, a pixel) could read a neighbour that another cell has already changed. The GPU gives no error for this race; the result just changes from one run to the next.

So a ping-pong keeps two copies of the state:

- `current()`: the state to read. It is also the latest state, the one to draw.
- `next()`: where the step writes the new state.

After the step, `swap()` exchanges the two roles. Nothing is copied: only the roles change.

```text
step 1:   A -> B
step 2:   B -> A
step 3:   A -> B
```

**Buffers** (a compute shader). `gpu::PingPong<T>` holds two storage buffers, and `gpu::dispatch` runs one step in one call. The names of the two storage blocks are given with their role, so they cannot be swapped by mistake. This is the loop of `05f_Galaxy`:

```cpp
// setUp: the first state, in both buffers.
COMPAGES_TRY_ASSIGN(m_stars, compages::gpu::PingPong<Star>::from(std::span<const Star>(stars)));

// Each frame: bind current() to Input and next() to Output, dispatch one
// thread per star, barrier, swap.
COMPAGES_TRY(compages::gpu::dispatch(m_step, m_stars, { .read = "Input", .write = "Output" }));
m_points.borrowVertices(m_stars.current());  // draw the latest state, no copy
m_points.draw();
```

`dispatch` ends with `barrier(Storage | VertexAttrib)`, so the next step and a draw both see the new state; when it fails, it does not swap. For a step that needs something else (two dispatches, other blocks), do it by hand: `bind("Input", current())`, `bind("Output", next())`, `dispatchItems(count())`, `barrier(...)`, `swap()`.

**Textures** (a fragment shader). `gpu::TexturePingPong` holds two textures and the framebuffer that draws into each one. A fragment shader must not sample the texture it draws into, for the same reason. The render pass stays in your code: it says where the draw goes. This is `05b_GameOfLife` (also `05c_GrayScott`, `10c_SweatAndDecals`):

```cpp
// setUp
COMPAGES_TRY_ASSIGN(m_field, compages::gpu::TexturePingPong::create(desc));
COMPAGES_TRY(m_field.current().write(std::as_bytes(std::span(cells))));  // generation 0

// Each frame
m_step["previous"] = m_field.current();                  // read the current one
{
    compages::gpu::RenderPass into(m_field.next(), { .clear_color = false });
    m_step.draw(3u);                                     // write the other one
}
m_field.swap();                                          // it becomes current
m_show["image"] = m_field.current();
```

`currentTarget()` draws over the current state in place, without a step: `10c_SweatAndDecals` paints a decal there. A render pass makes its writes visible to the next pass by itself; a compute dispatch needs the `barrier`.

To see the numbers on the CPU, read the current copy back: `m_stars.current().read()` returns a `std::vector<T>` (`05f_Galaxy` writes it to `galaxy.csv`); `buffer.download()` refreshes the CPU copy of a buffer that keeps one (`05a_Compute`). Both make the CPU wait for the GPU: do it once, not every frame. The helper script `examples/05_Simulation/05f_Galaxy_plot_xy.py` displays the star positions from that CSV with matplotlib.

---

## 🧰 6. When one Drawable is not enough

| Need | Types | Demo |
|------|-------|------|
| one mesh, several shaders | one `VertexBuffer<Vertex>`, borrowed by several `Drawable` | `04c_MultiPassMesh` |
| draw into an image | `Framebuffer`, `RenderPass`, `Texture` | `04d_RenderToTexture`, `04e_PostProcess` |
| cellular automaton on an image | two `Texture`, swapped | `05b_GameOfLife`, `05c_GrayScott` |
| compute writes the vertices | `ComputeProgram`, `Buffer`, `barrier` | `05d_ComputeParticles`, `05f_Galaxy` |
| the CPU never counts the survivors | compute → indirect draw command | `09c_IndirectDraw` |
| height field computed on the CPU | dynamic vertices, fixed indices | `02c_HeightMap`, `03e_Terrain3D` |

`04c_MultiPassMesh` is the milestone: one `VertexBuffer<Vertex>`, three drawables (lit, normals, wireframe) that borrow it. Each shader reads only the fields it needs; a field it does not declare is skipped, a field of the wrong type is an error. So the layout is declared **on the C++ side**: the shader does not guess it, and you never write `glVertexAttribPointer`.

Beneath the `Drawable`, the same is written with a `Program` and `Pipeline::create<Vertex>(program, state)`, which checks the struct against the shader, then `gpu::drawIndexed(pipeline, vertices, indices)` (`10c_SweatAndDecals`).

---

## 🎚 7. Passes and state

- `gpu::clear(color)` clears the current target.
- A `RenderPass` sets the viewport, clears, and chooses the target (framebuffer or window). `01b_ClearScreen` opens two passes, one per half of the window, without shader.
- `drawable.state().depth_test = true` (also blending, culling, primitive) belongs to the drawable. The pipeline is rebuilt only when the state changes. `03c_DepthAndTransforms` uses depth test and back-face culling.
- `gpu::forgetRenderState()`: call it after another library (ImGui in the gallery) touched the context, so that the backend stops trusting its cached state.

Driver messages (`KHR_debug`) go to stderr as `[gpu] warning:` and `[gpu] error:`.

---

## 🎫 8. Handles

Driver objects live in pools: an index plus a generation. Destroying twice does nothing. A `Buffer<T, Kind>` cannot be copied. The gallery `--check` verifies that closing a demo brings every counter back to zero: destroying your objects is enough, no `tearDown()` to write.

---

## 🧭 9. Gallery path for this layer

The gallery folders `01` to `05` follow this chapter. Read them in order; each demo adds one idea:

1. `01_FirstSteps/` — `01c_Triangle` (by name), `01d_InterleavedTriangle` (by struct), `01e_BorrowedTriangle` (a `Buffer` we own, borrowed by two drawables), `01f_VertexSyntax` (every way side by side).
2. `02_ChangingData/` — `02a_CpuGpuSync` (CPU copy, pending range), `02b_DynamicGeometry` (one edited vertex), `02c_HeightMap` (all sent again), `02d_Lorenz` (only the new part sent).
3. `03_Images/` — textures, then `03c_DepthAndTransforms` (indices, depth, matrices).
4. `04_Passes/` — `04a_Mandelbrot` (no vertex buffer), `04c_MultiPassMesh` (one buffer, three drawables), render to texture, post-process.
5. `05_Simulation/` — `05a_Compute` (one computation, one read-back), `05b_GameOfLife` (`TexturePingPong`), `05d_ComputeParticles`, `05e_SpiralGalaxy` (a formula: the vertex shader is enough), `05f_Galaxy` (a state: `PingPong`), `05g_NeuralNetwork` (three compute passes in a row).

All demos: [chapter 9](09-Examples.md).

Next: [05-World-Layer.md](05-World-Layer.md) — placing things without the GPU.
