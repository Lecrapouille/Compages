# AGENTS.md — context for coding agents

Compages: C++20 3D library in three stacked layers, each usable without the
one above: **GPU** (`compages::gpu`, shader-named data, `Drawable`, compute),
**World** (`compages::world`, scene graph + EnTT ECS, routines, joints,
headless), **Renderer** (`compages::renderer`, `Scene`, assets, snapshot,
drawing). Shared vocabulary directly in `compages` (vectors, matrices,
`Frame`, `Result`), from `Core/`; structured data (`Grid2D`, `CurvilinearGrid`, `Field<T>`,
`intersect` with a height field) directly in `compages`, from `Fields/`. OpenGL 4.5 backend `GL45`. Linux, plain `make` (MyMakefile in
`.makefile/`, no CMake). Dual license GPL-3.0-or-later / commercial.

Read only what the task needs. Load a skill instead of reading the docs:

| Task | Skill (`.claude/skills/`) | Long doc |
|------|---------------------------|----------|
| Shaders, `Drawable`, buffers, compute, passes, backend | `compages-gpu` | doc/concepts/the-gpu-layer.md |
| Entities, components, systems, routines, joints, headless | `compages-world` | doc/concepts/the-world.md |
| `Scene`, primitives, assets (glTF/STL/URDF), cameras, `CanvasTexture` | `compages-renderer` | doc/guides/README.md |
| Add or change a gallery demo | `compages-example` | doc/reference/gallery.md |
| Edit the documentation | `compages-docs` | README.md (Documentation) |
| Find a call | — | doc/reference/api.md |
| Change the core structure | — | doc/concepts/architecture.md |

## Layout

```text
include/Compages/   public, installed: Core/ Fields/ GPU/ World/ Renderer/ + Compages.hpp
                    World/: Components/ Spatial/ Kinematics/ Import/ Controllers/
                    Renderer/: Scene/ Assets/ Import/ Procedural/ Animation/ Render/ Components/ Serialization/
src/                same folders as include/; src/GPU/Backends/GL45/ is the only place with glad/gl*
examples/           the gallery (one binary build/linux-gcc-debug/bin/Compages-examples);
                    01_FirstSteps/ … 13_Workshop/ = chapters, read in order;
                    13_Workshop/ = the robot workshop of doc/tutorials/ (Workshop/Arm.cpp is World only);
                    Common/ = window (only GLFW user), Gallery, ExampleManifest.hpp;
                    Headless/ = Compages-headless (EGL, no window, Scene::renderToFile)
tests/              GoogleTest: Core/ Fields/ GPU/ World/ Renderer/ (build/linux-gcc-debug/bin/Compages-UnitTest)
external/           external.mk declares the third parties, versions.lock their commits;
                    external/Compages-data/ = demo assets
```

## Commands

```bash
make                                      # lib + gallery (fetches the third parties)
make check                                # unit tests
./build/linux-gcc-debug/bin/Compages-examples [name]          # gallery; --check [name] = run all (or one), fail on GPU leaks/errors
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 ./build/linux-gcc-debug/bin/Compages-examples --check   # no screen
```

## Rules that are easy to break

- Include direction: `Fields` includes only `Core`; `gpu` and `world`
  include only `Core` and `Fields`; `renderer` includes `world`, `gpu`,
  `Fields`, `Core`. `world` never sees a `gpu::` type; public
  headers never contain `GLenum`/`gl*`; only `src/GPU/Backends/GL45/` includes
  glad.
- The library never opens a window or calls GLFW: `gpu::init(loader)`
  receives a ready context. GLFW lives in `examples/Common/Window.cpp`.
- A world component never holds a GPU pointer, only asset ids; the renderer
  draws from a `RenderSnapshot`, never from the `World` directly.
- No physics engine (and no audio) in Compages: an outside engine writes the
  poses of entities marked `PoseDriver::External` (`07e_ExternalPhysics`).
- Classes stay small and focused. PIMPL only for `Scene` (`Scene::Impl` in
  `Scene.cpp`). A helper used only by one `.cpp` lives in `src/` next to it.
- Errors: load-time failures return `compages::Status` / `Result<T>`
  (`COMPAGES_TRY`, `COMPAGES_TRY_ASSIGN`); per-frame errors are recorded
  (`gpu::reportError`, read by `takeFrameError`), never thrown.
- After a compute dispatch, a `gpu::barrier(...)` is the caller's job.
- GPU handles are pooled (index + generation); `--check` must end with every
  counter at zero.
- RobotIK uses this repository as `external/Compages` (World + Renderer for
  its simulator, `CanvasTexture` for painted canvases): keep the public API stable
  or update RobotIK with it.

## Style

- `.clang-format` (braces on their own line, 4 spaces).
- Parameters `p_name`, members `m_name`, constants `kName` or `UPPER_CASE`.
- Doxygen `//!` comments; class banners `// ****...`, method banners `// ----...`.
  See "Doxygen comments" below.
- Every source file starts with the SPDX header
  `GPL-3.0-or-later OR LicenseRef-Compages-Commercial` (copy it from a neighbour).
- Commit subject: imperative sentence, no prefix. Docs: simple English (short
  sentences, common words) for readers whose English is basic; see the
  `compages-docs` skill.

## Doxygen comments

A comment says what the code cannot say. Repeating the name of the function
("Flushes pending changes to the GPU" on `flush()`) is not a comment. A
public class or function answers the questions that matter for it among:

| Question | The comment says |
|----------|------------------|
| Why? | which problem it solves |
| When? | in which situation to use it (and when not to) |
| How? | its observable behaviour, or the mechanism worth knowing |
| Guarantees? | invariants, side effects, contracts, who owns what |
| Limits? | what it does not do, special cases |
| Why this choice? | why this API exists rather than another one |

Every public **class** has a `\code … \endcode` example of its common use,
taken from a demo or a test that compiles. Example of the expected level:

```cpp
// ------------------------------------------------------------------------
//! \brief Synchronizes pending CPU-side changes with GPU resources.
//!
//! Changes to CPU-side data are accumulated to avoid unnecessary GPU
//! transfers. Call this method when the updated data must become
//! available for rendering.
//!
//! Only modified regions are uploaded, reducing transfer overhead when
//! working with large datasets.
//!
//! \note This operation does not wait for the GPU to finish executing
//! rendering commands.
// ------------------------------------------------------------------------
void flush();
```

Short getters and obvious members keep a one-line `\brief`. The same plain
English as the documentation.

## At the end of each request

Before the final answer, review what you changed against what was asked, then
end the answer with a table, one row per point of the request:

| | Point asked | What was done | What remains |
|---|-------------|---------------|--------------|
| ✅ | … | done and checked (how) | — |
| ⚠️ | … | partly done, or done but not checked | … |
| ❌ | … | not done, or done wrong | why, what to do |

Judge whether each change goes the way the user asked, not only whether it
builds. Say what was not checked (a locked screen, no test) instead of
marking it ✅.
