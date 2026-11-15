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

`05f_Galaxy` keeps tiles of stars in work-group memory to share reads between threads. `05e_SpiralGalaxy`, a port of the [galaxy example of Glumpy](https://github.com/glumpy/glumpy/blob/master/examples/galaxy.py) by Nicolas P. Rougier, needs no compute shader at all: each star rides a fixed ellipse, its position is a formula of the time, and only one uniform changes per frame. When the next state depends on the last (gravity, a Game of Life), the state must be stored: that is the compute shader and the ping-pong.

## 📤 Reading the numbers back

`m_stars.current().read()` returns a `std::vector<T>` (`05f_Galaxy` writes it to `galaxy.csv`, and `05f_Galaxy_plot_xy.py` plots it); `buffer.download()` refreshes the CPU copy of a buffer that keeps one (`05a_Compute`). Both make the CPU wait for the GPU: once, not every frame.

## 🔗 See also

[◀ The GPU layer](../concepts/the-gpu-layer.md) · [Draw in several passes](draw-in-several-passes.md) · [The ripple tank](../case-studies/ripple-tank.md), [Smoke](../case-studies/smoke.md)
