# 🏗 Architecture

[⏫ Concepts](README.md)

A change in one layer should not force every application to depend on the others. The include boundaries below preserve that separation. [Three layers](three-layers.md) explains the application design; the [code map](../reference/code-map.md) lists folders and headers for contributors.

## 🧭 Overview

```mermaid
flowchart TB
  subgraph app [Application]
    WIN[Window and GL context]
  end

  subgraph L3 [renderer]
    SC[Scene]
    AM[AssetManager]
    EX[SceneExtractor]
    RS[RenderSnapshot]
    REN[Renderer]
  end

  subgraph L2 [world]
    W[World]
    SG[SpatialGraph]
    TS[TransformStore]
    ENT[Entity]
    ECS[EnTT components]
  end

  subgraph L1 [gpu]
    DR[Drawable]
    BUF[Buffer Pipeline Texture Compute]
  end

  subgraph BE [private]
    GL[Backends/GL45 or GL41]
  end

  WIN --> SC
  SC --> W
  SC --> AM
  SC --> EX
  EX --> RS
  REN --> RS
  REN --> AM
  REN --> DR
  W --> SG
  W --> TS
  W --> ECS
  ENT --> W
  DR --> BUF
  BUF --> GL
```

`Core/` (vectors, matrices, `Result`, `Frame`, files) is the shared vocabulary, in the `compages` namespace; it is not a fourth layer. `Fields/` (grids and fields) is plain data, also in `compages`, usable by every layer ([fields and grids](fields-and-grids.md)).

## 🚧 Who may include whom

| Part | Entry point | May include | Never includes |
|------|-------------|-------------|----------------|
| `Fields` (in `compages`) | `Field<T>` | `Core` | `GPU`, `World`, `Renderer` |
| `compages::gpu` | `Drawable` | `Core`, `Fields` | `World`, `Renderer`, any `gl*` in a public header |
| `compages::world` | `World`, `Entity` | `Core`, `Fields` | `GPU`, `Renderer` |
| `compages::renderer` | `Scene` | `World`, `GPU`, `Fields`, `Core` | window code |

Public header entry points are listed in the [code map](../reference/code-map.md#-public-headers).

Only `src/GPU/Backends/` includes glad and GL headers. A world component never holds a GPU pointer, only asset ids; the Renderer draws from a `RenderSnapshot`, never from the World. The library never calls GLFW: GLFW lives in `examples/Common/Window.cpp`.

## 🎭 Four actors per frame

```text
application    context, input, ViewFrame
world          entities, routines, joints, matrices
renderer       extraction, drawing, asset ids resolved
gpu            sends the CPU changes to the backend, draws and dispatches
```

[One frame](one-frame.md) gives the order in detail.

## 📦 Public contracts and private helpers

`include/Compages/` is the installed contract. Implementations in `src/` can change without exposing backend details to applications. A helper used by one implementation stays beside that file. This keeps private dependencies out of public headers.

Only `Scene` uses PIMPL. Its implementation brings together assets, extraction and drawing, while its public header keeps those details private. The smaller classes expose their own focused interfaces. This avoids an allocation and an extra indirection for every small object.

## 🎮 The backend boundary

Every driver call goes through `src/GPU/Backends/Backend.hpp`: a list of free functions, with no virtual call. One backend implements them, chosen at build time by `GPU_BACKEND`. The compiler then lists what a new backend still lacks. Keeping these calls behind the GPU interface also lets World code compile without a graphics context.

Three backends exist:

| `GPU_BACKEND` | Context | Where | Folder |
|---------------|---------|-------|--------|
| `GL45` (default) | OpenGL 4.5 core | Linux, Windows | `GL45/` |
| `GL41` | OpenGL 4.1 core | macOS | `GL41/` |
| `GLES3` | OpenGL ES 3.0 | WebGL 2 (Emscripten), or a native ES context | `GL41/` with `COMPAGES_GLES3` |

`GL45` configures objects with Direct State Access: it never binds an object to change it. OpenGL 4.1 and ES 3.0 have no such calls, so `GL41` binds each object on a target that no draw depends on, and puts the old binding back. They also tie the vertex format to the vertex buffer: `GL41` keeps the fields of each shared vertex array and points them at the buffer again when the buffer changes.

The shaders stay written once, for `#version 450 core`. `GL41` translates them before compiling (`src/GPU/Internal/GlslDialect.cpp`): it lowers the version, adds the default precisions of ES, and moves `layout(binding = N)` (GLSL 4.20) out of the text, to apply it after linking. It also defines `COMPAGES_NO_STORAGE`, so that a shader keeps a path without storage blocks.

What has no equivalent is refused, with a sentence that says why: compute shaders, storage blocks and images (OpenGL 4.3 / ES 3.1). `DeviceInfo::compute` tells the program before it tries. The Renderer then draws without clustered lamps (only the four lamps with shadows) and without volumetric fog. A reversed depth buffer still draws right, but without `glClipControl` it keeps the precision of an ordinary one.

World-only applications need none of this: the include boundary keeps them free of any driver.
