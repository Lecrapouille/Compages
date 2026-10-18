# AGENTS.md — context for coding agents

Compages: C++20 3D library in three stacked layers, each usable without the
one above: **GPU** (`compages::gpu`, shader-named data, `Drawable`, compute),
**World** (`compages::world`, scene graph + EnTT ECS, behaviors, joints,
headless), **Renderer** (`compages::renderer`, `Scene`, assets, snapshot,
drawing). Shared vocabulary in `compages::core` (vectors, matrices, `Frame`,
`Result`). OpenGL 4.5 backend `GL45`. Linux, plain `make` (MyMakefile in
`.makefile/`, no CMake). Dual license GPL-3.0-or-later / commercial.

Read only what the task needs. Load a skill instead of reading the docs:

| Task | Skill (`.claude/skills/`) | Long doc |
|------|---------------------------|----------|
| Shaders, `Drawable`, buffers, compute, passes, backend | `compages-gpu` | doc/04-GPU-Layer.md |
| Entities, components, systems, behaviors, joints, headless | `compages-world` | doc/05-World-Layer.md |
| `Scene`, primitives, assets (glTF/STL/URDF), cameras, `CanvasTexture` | `compages-renderer` | doc/06-Renderer-Layer.md |
| Add or change a gallery demo | `compages-example` | doc/09-Examples.md |
| Edit the documentation | `compages-docs` | doc/README.md |
| Find a call | — | doc/08-API-Reference.md |
| Change the core structure | — | doc/07-Architecture.md |

## Layout

```text
include/Compages/   public, installed: Core/ GPU/ World/ Renderer/ + Compages.hpp
src/                implementation; src/GPU/Backends/GL45/ is the only place with glad/gl*
examples/           the gallery (one binary build/Compages-examples);
                    Common/ = window (only GLFW user), Gallery, ExampleManifest.hpp
tests/              GoogleTest: Core/ GPU/ World/ Renderer/ (build/Compages-UnitTest)
attic/Physics/      ReactPhysics3D adapter, out of the build
external/           third parties from external/manifest (do not edit);
                    external/Compages-data/ = demo assets
```

## Commands

```bash
make download-external-libs && make compile-external-libs   # once
make -j"$(nproc)"                         # lib + gallery
make tests -j"$(nproc)"                   # unit tests
./build/Compages-examples [name]          # gallery; --check = run all, fail on GPU leaks/errors
xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 ./build/Compages-examples --check   # no screen
```

## Rules that are easy to break

- Include direction: `gpu` and `world` include only `Core`; `renderer`
  includes `world`, `gpu`, `Core`. `world` never sees a `gpu::` type; public
  headers never contain `GLenum`/`gl*`; only `src/GPU/Backends/GL45/` includes
  glad.
- The library never opens a window or calls GLFW: `gpu::init(loader)`
  receives a ready context. GLFW lives in `examples/Common/Window.cpp`.
- A world component never holds a GPU pointer, only asset ids; the renderer
  draws from a `RenderSnapshot`, never from the `World` directly.
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
- Every source file starts with the SPDX header
  `GPL-3.0-or-later OR LicenseRef-Compages-Commercial` (copy it from a neighbour).
- Commit subject: imperative sentence, no prefix. Docs: simple English for
  French readers.
