# 🧮 Compute on the GPU

[⏫ Guides](README.md)

Run a simulation on the GPU (particles, cells, stars, a fluid) and draw it where it is, without bringing it back to the CPU.

![05f_Galaxy](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05f_Galaxy.png)

The smallest compute shader is in [the GPU alone](../tutorials/02-the-gpu.md). This guide goes on from there.

## 🔁 Compute writes, draw reads, a barrier between

A compute shader runs once per item of a grid. The same `StorageBuffer` can be what it writes and the vertices of the next draw, with no copy through the CPU. From `05d_ComputeParticles`:

```cpp
COMPAGES_TRY(step.dispatchItems(count));
compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);
points.draw();
```

The barrier makes the writes visible to the next reader. Choose the flag for **that reader**: `VertexAttrib` for a draw that reads vertices, `Storage` for another shader that reads a storage buffer, both with `|` (`Command`, `All`… are in `GPU/Enums.hpp`). **The barrier is your job**: a missing one is a race, not a compile error; the particles flicker or lag a frame.

## 🏓 Ping-pong: read one copy, write the other

When a step reads neighbouring items, keep its input state separate from its output: one cell could otherwise read a neighbour that another cell has already changed. The GPU gives no error for this; the result changes from one run to the next. So keep two copies: `current()` to read (and to draw), `next()` to write, then `swap()` the roles. Nothing is copied.

```text
step 1:   A -> B
step 2:   B -> A
step 3:   A -> B
```

**Buffers** (a compute shader): `gpu::PingPong<T>` holds two storage buffers, and `gpu::dispatch` runs one step. The names of the two blocks go with their role, so they cannot be swapped by mistake. From `05f_Galaxy`:

```cpp
// setUp: the first state, in both buffers.
COMPAGES_TRY_ASSIGN(m_stars, compages::gpu::PingPong<Star>::from(std::span<const Star>(stars)));

// each frame: current() to Input, next() to Output, one thread per star, barrier, swap.
COMPAGES_TRY(compages::gpu::dispatch(m_step, m_stars, { .read = "Input", .write = "Output" }));
m_points.borrowVertices(m_stars.current());  // draw the latest state, no copy
m_points.draw();
```

`dispatch` ends with `barrier(Storage | VertexAttrib)`; when it fails, it does not swap. For a step that needs more (two dispatches, other blocks), do it by hand: `bind("Input", current())`, `bind("Output", next())`, `dispatchItems(count())`, `barrier(…)`, `swap()`.

**Textures** (a fragment shader): `gpu::TexturePingPong` holds two textures and the framebuffer that draws into each. From `05b_GameOfLife`:

```cpp
// setUp
COMPAGES_TRY_ASSIGN(m_field, compages::gpu::TexturePingPong::create(desc));
COMPAGES_TRY(m_field.current().write(std::as_bytes(std::span(cells))));  // generation 0

// each frame
m_step["previous"] = m_field.current();                  // read the current one
{
    compages::gpu::RenderPass into(m_field.next(), { .clear_color = false });
    m_step.draw(3u);                                     // write the other one
}
m_field.swap();                                          // it becomes current
m_show["image"] = m_field.current();
```

`draw(3u)` with no vertices: the vertex shader makes three corners from `gl_VertexID`, one triangle covers the texture, and each pixel is one cell. A render pass makes its writes visible to the next pass by itself; only a compute dispatch needs the barrier. `05c_GrayScott` and `10c_SweatAndDecals` work this way. [Texture ping-pong](../techniques/texture-ping-pong.md) explains the feedback hazard and the Game of Life update in detail.

## 🧱 Work groups and more

| Need | Types | Demo |
|------|-------|------|
| a draw reads what compute wrote | `program.bind("Block", buffer.handle())`, `barrier(Barrier::Storage)` | `11b_RippleTank` |
| the CPU never counts the survivors | compute writes an indirect draw command | `09c_IndirectDraw` |
| several kernels in a row | one dispatch each, a barrier between | `05g_NeuralNetwork`, `11e_Smoke` |
| a formula of the time is enough | no compute: the vertex shader computes the position | `05e_SpiralGalaxy` |

`05e_SpiralGalaxy`, a port of the [galaxy example of Glumpy](https://github.com/glumpy/glumpy/blob/master/examples/galaxy.py) by Nicolas P. Rougier, needs no compute shader at all: each star rides a fixed ellipse, its position is a formula of the time, and only one uniform changes per frame. When the next state depends on the last (gravity, a Game of Life), the state must be stored: that is the compute shader and the ping-pong.

## 🤝 Every item reads every other: `shared` memory

In `05f_Galaxy`, each star is pulled by the 8192 others, so each invocation reads 8192 positions. The GPU can do this faster than one read per star and per position. To see how, you need two facts about a compute shader.

**Work groups.** The invocations do not run one by one. They run in groups whose size the shader sets: `layout(local_size_x = 256) in;` makes groups of 256 invocations, so 8192 stars make 32 groups.

**Two kinds of memory.** The storage buffers are in the global memory of the card: large and seen by every invocation, but slow (hundreds of cycles per read that misses the cache). A variable declared `shared` is in a small memory inside the unit that runs the group (tens of kilobytes): almost as fast as a register, but seen only by the invocations of that group, and gone when the group ends.

So the group works as a team. The positions are cut into tiles of 256. For each tile, each invocation copies one position from the buffer into a `shared` array. `barrier()` then waits until all 256 have copied. After it, each invocation reads the 256 positions from the fast memory, and a second `barrier()` waits before the array is overwritten by the next tile. Each position is read from the slow memory once per group instead of 256 times. The result is the same as a plain loop; only the time changes.

Writing this by hand is error prone (the last tile is often partial, and a `barrier()` that some invocations skip hangs or corrupts the group). It also does not exist on macOS and in the browser (see below). Write the loop once with `<compages/tiles.glsl>`, an include that every shader has without declaring it. From `05f_Galaxy`:

```glsl
#version 450 core
layout(local_size_x = 256) in;
#include <compages/tiles.glsl>
// ...
COMPAGES_TILE(vec2, positions);   // where a tile waits: shared memory, or nothing

void main()
{
    uint i = gl_GlobalInvocationID.x;
    Star me = incoming[min(i, count - 1u)];
    vec2 acc = vec2(0.0);

    // for each j < count: other = incoming[j].position
    COMPAGES_FOR_EACH(positions, j, count, vec2 other, incoming[j].position)
    {
        vec2 r = other - me.position;
        float d2 = dot(r, r) + softening;
        acc += r * (G / (d2 * sqrt(d2)));
    }
    COMPAGES_END_FOR_EACH
    if (i >= count)
    {
        return;
    }
    // ...
}
```

The program must be loaded from a `ShaderSource`, which resolves the include: `m_step.load(gpu::ShaderSource::fromText("galaxy.comp", STEP))`. With compute shaders, the macros expand to the tiled loop with `shared` and `barrier()`. On this machine (RTX 3050) the step of 8192 stars takes 0.40 ms, as the loop written by hand; the plain loop takes 0.62 ms. Where compute is emulated, they expand to the plain loop.

Three rules, all checked by `05f_Galaxy`:

- `COMPAGES_TILE` comes after `layout(local_size_x = …)`: the size of the tile is the size of the group.
- Every invocation of the group must reach the loop, as for any `barrier()`. Do not `return` before it: clamp the index (`min(i, count - 1u)`), and test it after the loop.
- The last argument is copied into the loop by the preprocessor: put an expression that holds a comma between brackets.

## 🍎 On macOS and in the browser

OpenGL 4.1 (macOS) and WebGL 2 have no compute shader and no storage buffer. The `GL41` and `GLES3` backends run your compute shader anyway: `ComputeProgram`, `PingPong`, `dispatch()` and your GLSL do not change. The backend rewrites the shader into a vertex shader that runs once per invocation, with nothing drawn. Before each dispatch, it copies each storage buffer into a texture, where the shader reads it. The outputs of each invocation are written into the buffer by the card (transform feedback). `gpu::device().compute_emulated` is true there, and `COMPAGES_EMULATED_COMPUTE` is defined in the shader.

This has a cost and rules. The cost, for the step of `05f_Galaxy` on the same card:

| Stars | GL45 (compute, `shared`) | GL41 (emulated) |
|------:|------:|------:|
| 8 192 | 0.40 ms | 2.5 ms |
| 32 768 | 6.4 ms | 34 ms |

Most of it is the reads: a texture read for each 32 bit word, where a storage buffer reads a whole struct. Plan on a few times slower; a Mac has its own card, so measure there.

The rules come from what a vertex shader can do:

- **An invocation writes only its own element**: the one of its index in the dispatch (`gl_GlobalInvocationID.x` for a dispatch in one dimension). The shader checks the index of each write: a write to another element is lost. A block written at several different indices in one shader is refused when it loads.
- **It reads any element**, as the buffer was when the dispatch started, and its own element as it has written it.
- At most four buffers are written by one dispatch. When more than one is written, each holds numbers or `vec2` / `vec4` (no struct).
- A storage block holds one array of unknown size (`buffer Stars { Star stars[]; };`), of `float`, `int`, `uint`, their vectors, or structs of them.
- `shared` and `barrier()` outside `<compages/tiles.glsl>`, atomics (`atomicAdd`), and images (`imageStore`) are refused when the program loads, with a message that says why.

In the gallery, `05a`, `05d`, `05f`, `11b` and `11e` run this way. The vertex and fragment shaders that read what compute wrote (`program.bind("Block", buffer.handle())`) work too: they read a copy of the buffer, made again after each dispatch that wrote it. `05g_NeuralNetwork` (one invocation writes all the gradients of its sample, so it is refused) and `09c_IndirectDraw` (`atomicAdd`) need real compute: on macOS and the web they say so and stop.

## 📤 Reading the numbers back

`m_stars.current().read()` returns a `std::vector<T>` (`05f_Galaxy` writes it to `galaxy.csv`, and `05f_Galaxy_plot_xy.py` plots it); `buffer.download()` refreshes the CPU copy of a buffer that keeps one (`05a_Compute`). Both make the CPU wait for the GPU: once, not every frame.

## 🔗 See also

[◀ The GPU layer](../concepts/the-gpu-layer.md) · [Draw in several passes](draw-in-several-passes.md) · [Build for macOS and the web](build-for-macos-and-the-web.md) · [The ripple tank](../case-studies/ripple-tank.md), [Smoke](../case-studies/smoke.md)
