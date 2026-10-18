---
name: compages-gpu
description: Write or change Compages GPU-layer code (compages::gpu): Drawable and the shader naming contract, Buffer<Vertex>, Pipeline/Program, textures, framebuffers, RenderPass, compute shaders and barriers, render state, frame errors, the GL45 backend. Use for shader programs, "unknown attribute" frame errors, black frames, compute races, multi-pass rendering.
---

# Compages GPU layer

The shader is the API: GLSL declares names, C++ fills them by name, `draw()`
uploads the dirty ranges then draws. Header `<Compages/GPU/GPU.hpp>`.

```cpp
if (auto ready = compages::gpu::init(glfwGetProcAddress); !ready) { /* ready.error() */ }

compages::gpu::Drawable mesh;
COMPAGES_TRY(mesh.load(vertex_src, fragment_src));   // compile error -> Status
mesh["position"] = { { -0.8f, -0.6f }, { 0.8f, -0.6f }, { 0.0f, 0.8f } };   // attribute
mesh["tint"] = compages::core::Vector3f{ 1, 0.5f, 0.2f };                    // uniform
mesh["image"] = texture;                                                    // sampler (texture outlives mesh)
COMPAGES_TRY(mesh.prepare());                        // surface frame errors before the loop

compages::gpu::clear({ 0.1f, 0.1f, 0.15f });         // per frame
mesh.draw();
```

- Attributes given one by one are interleaved; assignments mark dirty
  ranges; `draw()` sends only those. A raw `Buffer` you manage: `upload()`.
- Typed vertices: struct field names = attribute names
  (`compages::core::Vector2f position; ... color;`), `mesh.vertices<Vertex>(span)`,
  `mesh.vertex<Vertex>(i).position.y = …` (dirty).
- Static data: `Buffer<Vertex>::from(data, { .usage = BufferUsage::Immutable, .cpu_mirror = false })`
  then `mesh.vertices(buffer)` (no copy).
- One mesh, several shaders: one `Buffer<Vertex>`, `Pipeline::create<Vertex>(program, state)`
  per shader (checks the layout; a dropped attribute is fine, a type mismatch fails). Demo `05a_MultiPassMesh`.
- Off-screen: `Framebuffer`, `RenderPass` (viewport, clear, target), `Texture` (`05b`, `05c`).
- Compute: `ComputeProgram::fromSource(src)` / `load`, `dispatchItems(n)`, then
  **`gpu::barrier(Barrier::VertexAttrib | ...)` yourself** (missing barrier =
  race, not compile error); `buffer.download()` needs a CPU mirror and the
  right barrier. Demos `00c_Compute`, `12_ComputeParticles`, `13_Galaxy`, `21_IndirectDraw`.
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
  `src/GPU/Backends/GL45/` (selected by `GPU_BACKEND` in `Makefile.common`).
- `gpu` includes only `Core`, never `World`/`Renderer`.
- Handles are pooled (index + generation); `Buffer<T>` is not copyable;
  `./build/Compages-examples --check` must report no leftover handle.
- Tests: `tests/GPU/` (need a GL 4.5 context; use the xvfb + Mesa command of
  AGENTS.md without screen).

Reference: doc/04-GPU-Layer.md, doc/10-Debugging.md.
