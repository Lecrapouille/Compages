---
name: compages-gpu
description: Write or change Compages GPU-layer code (compages::gpu): Drawable and the shader naming contract, VertexBuffer<Vertex>, Pipeline/Program, textures, framebuffers, RenderPass, compute shaders and barriers, render state, frame errors, the GL45 backend. Use for shader programs, "unknown attribute" frame errors, black frames, compute races, multi-pass rendering.
---

# Compages GPU layer

The shader is the API: GLSL declares names, C++ fills them by name, `draw()`
uploads the dirty ranges then draws. Header `<Compages/GPU/GPU.hpp>`.

```cpp
if (auto ready = compages::gpu::init(glfwGetProcAddress); !ready) { /* ready.error() */ }

compages::gpu::Drawable mesh;
COMPAGES_TRY(mesh.loadSources(vertex_src, fragment_src));   // compile error -> Status
mesh["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };   // attribute
mesh["tint"] = compages::Vector3f{ 1, 0.5f, 0.2f };                    // uniform
mesh["image"] = texture;                                                    // sampler (texture outlives mesh)
COMPAGES_TRY(mesh.prepare());                        // surface frame errors before the loop

compages::gpu::clear({ 0.1f, 0.1f, 0.15f });         // per frame
mesh.draw();
```

- Attributes given one by one are interleaved; assignments mark dirty
  ranges; `draw()` sends only those. A raw `Buffer` you manage: `upload()`.
- Typed vertices: struct field names = attribute names
  (`compages::Vector2f position; ... color;`),
  `mesh.setVertices<Vertex>(span, BufferUsage::Static | BufferUsage::Dynamic)` (no
  default; Static frees the CPU copy after the first draw, editing it then is an
  error), `mesh.editVertex<Vertex>(i).position.y = …` (marked),
  `editVertices<Vertex>(first, count)`, `editVertices<Vertex>()`, `vertexCount()`.
- Rule: the drawable owns its vertices; `mesh.borrowVertices(buffer)` (no copy)
  only for GPU-made (compute) or shared vertices. Table in doc/04 §3.
- Buffers: `VertexBuffer<T>`, `IndexBuffer<T>`, `StorageBuffer<T>` (the kind is in
  the type); `::from(data, usage)` / `::create(count, usage)`, no default usage:
  `Static` (no CPU copy, never written; a compute pass may write a StorageBuffer),
  `Stream` (no CPU copy, `write()`), `Dynamic` (CPU copy, `[]`/`emplace_back`, `upload()`).
- One mesh, several shaders: one `VertexBuffer<Vertex>`, `d.borrowVertices(buffer)` in
  each `Drawable` (demos `01d_BorrowedTriangle`, `05a_MultiPassMesh`). One level
  lower: `Pipeline::create<Vertex>(program, state)` per shader (checks the
  layout; a dropped attribute is fine, a type mismatch fails), `54_SweatAndDecals`.
- All the vertex syntaxes side by side: `02b_VertexSyntax`.
- Off-screen: `Framebuffer`, `RenderPass` (viewport, clear, target), `Texture` (`05b`, `05c`).
- Compute: `ComputeProgram` `loadSource(src)` / `loadFile(path)`, `dispatchItems(n)`, then
  **`gpu::barrier(Barrier::VertexAttrib | ...)` yourself** (missing barrier =
  race, not compile error); `buffer.download()` needs a CPU mirror and the
  right barrier. Demos `00c_Compute`, `12_ComputeParticles`, `13_Galaxy`, `21_IndirectDraw`.
- Ping-pong (`GPU/PingPong.hpp`): `PingPong<T>` + `gpu::dispatch(program, state, { .read = "In", .write = "Out" })`
  (bind, dispatch, barrier Storage|VertexAttrib, swap); `TexturePingPong` for fragment-shader steps:
  read `current()`, `RenderPass(next())`, `swap()`.
- State: `mesh.state().depth_test = true` (blending, culling, primitive);
  `mesh.indices({...})`. After ImGui or any foreign GL code:
  `gpu::forgetRenderState()`.

## Errors

Load time → `compages::Status`/`Result<T>` (`COMPAGES_TRY`,
`COMPAGES_TRY_ASSIGN`). Frame time (unknown name, wrong component count,
empty draw) → recorded, first message kept: `gpu::takeFrameError()`,
`frameErrorCount()`, `setBreakOnError(true)`; library code reports with
`gpu::reportError(...)`, never throws. Driver: `[gpu] warning:/error:` on
stderr (KHR_debug).

## Rules

- No `GLenum`/`gl*` in public headers; GL calls only in
  `src/GPU/Backends/GL45/` (selected by `GPU_BACKEND` in the `Makefile`).
- `gpu` includes only `Core`, never `World`/`Renderer`.
- Handles are pooled (index + generation); `Buffer<T, Kind>` is not copyable;
  `./build/linux-gcc-debug/bin/Compages-examples --check` must report no leftover handle.
- Tests: `tests/GPU/` (need a GL 4.5 context; use the xvfb + Mesa command of
  AGENTS.md without screen).

Reference: doc/04-GPU-Layer.md, doc/10-Debugging.md.
