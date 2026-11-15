# 🧩 Share and reload shader source

[⏫ Guides](README.md)

Share GLSL between several shaders and reload edited files without restarting the application.

![04e_PostProcess](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04e_PostProcess.png)

Start with a working GPU program from [the GPU tutorial](../tutorials/02-the-gpu.md). This guide changes how its source is assembled and loaded; pass scheduling stays in [draw in several passes](draw-in-several-passes.md).

## 🧩 Shaders made of pieces: ShaderSource

A long shader is easier to read and to share in pieces. A `ShaderSource` is the GLSL of one stage: a main text that says `#include "name"` or `#include <name>`. A name is looked for in the texts given with `include(name, text)` (GLSL in a string, or **made from C++ data**: the constants of a simulation written once for the CPU and the GPU), then, for `"name"`, beside the including file, then in each folder given with `directory(path)`. Each piece goes in once; a circle of includes is an error that shows it (`a -> b -> a`). `define(name, value)` adds a `#define` after the `#version`.

```cpp
auto source = compages::gpu::ShaderSource::fromText("water.frag", R"(#version 450 core
#include <waves.glsl>
out vec4 oColor;
void main() { oColor = vec4(vec3(wave(0.5)), 1.0); }
)");
source.include("waves.glsl", "float wave(float x) { return sin(x * PI); }\n")
      .define("PI", "3.14159");
COMPAGES_TRY(program.load(vertex, source));
```

Here `program` is an existing `compages::gpu::Program`, and `vertex` is its vertex-stage `compages::gpu::ShaderSource`. Call this code in a function returning `compages::Status`.

The driver numbers the pieces (`#line`), and Compages rewrites its log with their names: `1:1(28): error` becomes `waves.glsl:1(28): error`.

A **hook** is a named place a shader of a library leaves open, with a default text: `#include <hook/deform>`. The library declares it with `declareHook(name, text)`; you replace it with `hook(name, text)` ([write a material](write-a-material.md)).

## 🔄 Hot reload

The callback below belongs to a class holding `m_program`, `m_pipeline`, `m_state` and `m_watch`. Its `Vertex` type describes the mesh attributes.

`gpu::ShaderWatch` watches the files of two stages (and their includes) and calls your load again when one is saved. A load that fails keeps what was there and `poll()` returns the log, so a typo while editing does not break the picture:

```cpp
auto load = [this](compages::gpu::ShaderSource const& p_vertex, compages::gpu::ShaderSource const& p_fragment) -> compages::Status
{
    COMPAGES_TRY(m_program.load(p_vertex, p_fragment));
    COMPAGES_TRY_ASSIGN(m_pipeline, compages::gpu::Pipeline::create<Vertex>(m_program, m_state));
    return compages::success();
};
COMPAGES_TRY(m_watch.watch(compages::gpu::ShaderSource::fromFile("water.vert"), compages::gpu::ShaderSource::fromFile("water.frag"), load));
// each frame:
if (compages::Status reloaded = m_watch.poll(); !reloaded) { std::cerr << reloaded.error(); }
```

## 🧯 Common problems

- Include names must match the names registered with `include()` or the files found through `directory()`.
- Keep `#version` in the main source. Put shared declarations and functions in the included pieces.
- After replacing a `Program`, rebuild pipelines that refer to it. The reload callback above does both operations.
- Read the `Status` from `poll()`: it reports edits that failed to compile while the previous program remains usable.

## 🔗 See also

[Shader library](../reference/shaders.md) · [Write a material](write-a-material.md) · [The GPU layer](../concepts/the-gpu-layer.md)
