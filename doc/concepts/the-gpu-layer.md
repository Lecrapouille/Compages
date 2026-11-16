# 🟧 The GPU layer

[⏫ Concepts](README.md)

![05d_ComputeParticles](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05d_ComputeParticles.png)

In raw OpenGL, a shader and the C++ that feeds it are glued by numbers. The shader says `in vec2 position;`; the C++ asks the driver for the location of `position`, binds a buffer, describes the bytes with `glVertexAttribPointer(0, 2, GL_FLOAT, …)`, and hopes the two sides agree. When they do not, the screen stays black and nothing says why. Most of a first OpenGL program is this glue, and most of its bugs are in it.

## 🔖 The name is the contract

The shader already knows its inputs and their types. So Compages reads them from the compiled program, and the C++ side uses the same names:

```cpp
compages::gpu::Drawable mesh;
COMPAGES_TRY(mesh.loadSources(vertex_src, fragment_src));

mesh["position"] = positions;                           // attribute: one value per vertex
mesh["tint"]     = compages::Vector3f{ 1, 0.5f, 0.2f }; // uniform
mesh["image"]    = texture;                             // sampler
mesh["time"]     = elapsed;                             // uniform

mesh.draw();
```

`operator[]` does not guess: the program already classified each name as an attribute, a uniform or a sampler. A wrong name or a wrong type is an error message that lists what the shader really declares, not a black screen ([errors](errors.md)).

**The CPU writes, `draw()` sends.** Assigning changes a copy kept in normal memory, the **CPU mirror**, and marks the changed range dirty. `draw()` sends only that range, then draws. Moving one corner of a triangle sends one vertex, not three (`02b_DynamicGeometry`). Attributes given one by one are stored **interleaved** (position, colour, position, colour…), the layout the GPU reads best.

The idea comes from [Glumpy](https://glumpy.github.io/) (Python). Glumpy alone would copy a mesh again for each shader that reads it (the picture, the shadow, the outline). Compages adds a vertex layout on the C++ side (`VertexBuffer<Vertex>`, `Pipeline`), checked against each program: one buffer, several shaders.

## 🧬 Who owns the vertices

**The drawable owns its vertices. Borrow a buffer when they are made on the GPU, shared by several drawables, or large and never changed.**

| The vertices… | Call | Who keeps them | Demo |
|---------------|------|----------------|------|
| are given or edited from C++ | `d.setVertices(data)`, `d.editVertex<V>(i)`, `d.emplace_back(v)` | the drawable, on the CPU and the GPU | `01d_InterleavedTriangle`, `02b_DynamicGeometry` |
| are given one attribute at a time | `d["position"] = { … }` | the drawable | `01c_Triangle` |
| are made on the GPU, shared, or large and fixed | `d.borrowVertices(buffer)` | your `Buffer`, which must outlive the drawable | `01e_BorrowedTriangle`, `05d_ComputeParticles` |

What a buffer is used for is part of its type, so drawing indices from a buffer of vertices does not compile: `VertexBuffer<T>`, `IndexBuffer<T>`, `StorageBuffer<T>`. How it lives is chosen at creation, with the OpenGL words, and has no default:

| `BufferUsage` | CPU copy | Written by | For |
|---------------|----------|------------|-----|
| `Static` | no | nobody after creation (a compute pass may write a `StorageBuffer`) | a mesh read from a file |
| `Stream` | no | `write()`, often whole | skinned vertices, debug lines |
| `Dynamic` | yes | `buffer[i] = …`, `emplace_back()`, then `upload()` sends what changed | a trail, numbers the CPU and the GPU share |

## 🧮 Compute, and the barrier you write

A compute shader draws nothing: it runs once per item of a grid. The same `StorageBuffer` can be what it writes and the vertices of the next draw, with no trip through the CPU:

```cpp
COMPAGES_TRY(step.dispatchItems(count));
compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);
points.draw();
```

The GPU does not wait for a dispatch to finish before the next command reads its result. The `barrier` says "the next reader waits", and the flag names the next reader: `VertexAttrib` for a draw, `Storage` for another shader, both with `|`. **Compages does not add it for you**: a missing barrier is a race, not a compile error. `gpu::dispatch(program, pingpong, …)`, which runs a whole ping-pong step, does add it, since it knows the next reader.

## 🎫 Handles

Driver objects live in pools: an index plus a generation. Destroying twice does nothing. A `Buffer` cannot be copied. The gallery's `--check` closes each demo and verifies that every counter is back at zero: destroying your C++ objects is enough, there is no `tearDown()` to write.

Before any drawing, a `RenderPass` says where to draw; it ends at the closing brace of its scope. Compages never opens a window: `gpu::init(loader)` receives the function loader of a context your program made, and `gpu::shutdown()` must come after every GPU object is destroyed and before the context is.

## 📏 What names cost

- Names are checked when the program runs, not when it compiles. A typo in `mesh["colour"]` is found by `prepare()` or at the first frame, not by the C++ compiler.
- **The dirty range is one range**, from the first change to the last: changing elements 1 and 5000 sends everything between them, in one copy.
- Compute needs OpenGL 4.3. The macOS backend (`GL41`) and the web backend (`GLES3`) have no compute shaders, storage blocks or images: check `gpu::device().compute`. The rest of this page works on the three backends ([architecture](architecture.md#-the-backend-boundary)).

## 🔗 See also

- [The GPU alone](../tutorials/02-the-gpu.md): the triangle and the compute shader, step by step.
- [Compute on the GPU](../guides/compute-on-the-gpu.md): ping-pong, work groups, reading back.
- [Draw in several passes](../guides/draw-in-several-passes.md): textures as targets, post-processing, shaders made of pieces.
