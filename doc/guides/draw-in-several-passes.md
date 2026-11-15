# Draw in several passes

[Guides](README.md)

Draw into a texture and use it in the next draw (post-processing, a mirror, a simulation), share one mesh between several shaders, write shaders in pieces, and reload them while the program runs.

![04e_PostProcess](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04e_PostProcess.png)

For filters of the picture of a Scene, [screen effects](screen-effects.md) do it all; this guide is for passes of your own, with the GPU layer.

## When one Drawable is not enough

| Need | Types | Demo |
|------|-------|------|
| one mesh, several shaders | one `VertexBuffer<Vertex>`, borrowed by several `Drawable` | `04c_MultiPassMesh` |
| draw into a picture | `Framebuffer`, `RenderPass`, `Texture` | `04d_RenderToTexture`, `04e_PostProcess` |
| write a picture into a file | `texture.saveFile("out.png")` | `Compages-headless` |
| a cellular automaton on a picture | `TexturePingPong` ([compute on the GPU](compute-on-the-gpu.md)) | `05b_GameOfLife` |
| a height field computed on the CPU | dynamic vertices, fixed indices | `02c_HeightMap`, `03e_Terrain3D` |

`04c_MultiPassMesh` is the milestone: one `VertexBuffer<Vertex>`, three drawables (lit, normals, wireframe) that borrow it. Each shader reads only the fields it needs; a field it does not declare is skipped, a field of the wrong type is an error. The layout is declared on the C++ side, so the shader does not guess it. Under the `Drawable`, the same is a `Program` and `Pipeline::create<Vertex>(program, state)`, which checks the struct against the shader, then `gpu::drawIndexed(pipeline, vertices, indices)` (`10c_SweatAndDecals`).

## Passes and state

- A `RenderPass` chooses the target (a framebuffer, or the window), sets the viewport and clears. It ends at the closing brace of its scope; passes nest. `01b_ClearScreen` opens two passes, one per half of the window.
- `gpu::clear(color)` clears the current target.
- `drawable.state().depth_test = true` (blending, culling, primitive too) belongs to the drawable; the pipeline is rebuilt only when the state changes. `03c_DepthAndTransforms` uses the depth test and back-face culling.
- `gpu::forgetRenderState()`: after another library (ImGui) touched the context, the backend stops trusting its cached state.

A pass that draws **over a picture of a Scene** and tests its depth must use the Scene's reversed depth ([very large scenes](very-large-scenes.md#reversed-depth)).

## Shaders made of pieces: ShaderSource

A long shader is easier to read and to share in pieces. A `ShaderSource` is the GLSL of one stage: a main text that says `#include "name"` or `#include <name>`. A name is looked for in the texts given with `include(name, text)` (GLSL in a string, or **made from C++ data**: the constants of a simulation written once for the CPU and the GPU), then, for `"name"`, beside the including file, then in each folder given with `directory(path)`. Each piece goes in once; a circle of includes is an error that shows it (`a -> b -> a`). `define(name, value)` adds a `#define` after the `#version`.

```cpp
auto source = compages::gpu::ShaderSource::fromText("water.frag", R"(#version 450 core
#include <waves.glsl>
out vec4 oColor;
void main() { oColor = vec4(wave(0.5), 1.0); }
)");
source.include("waves.glsl", "float wave(float x) { return sin(x * PI); }\n")
      .define("PI", "3.14159");
COMPAGES_TRY(program.load(vertex, source));
```

The driver numbers the pieces (`#line`), and Compages rewrites its log with their names: `1:1(28): error` becomes `waves.glsl:1(28): error`.

A **hook** is a named place a shader of a library leaves open, with a default text: `#include <hook/deform>`. The library declares it with `declareHook(name, text)`; you replace it with `hook(name, text)` ([write a material](write-a-material.md)).

## Hot reload

`gpu::ShaderWatch` watches the files of two stages (and their includes) and calls your load again when one is saved. A load that fails keeps what was there and `poll()` returns the log, so a typo while editing does not break the picture:

```cpp
auto load = [this](ShaderSource const& p_vertex, ShaderSource const& p_fragment) -> compages::Status
{
    COMPAGES_TRY(m_program.load(p_vertex, p_fragment));
    COMPAGES_TRY_ASSIGN(m_pipeline, compages::gpu::Pipeline::create<Vertex>(m_program, m_state));
    return compages::success();
};
COMPAGES_TRY(m_watch.watch(ShaderSource::fromFile("water.vert"), ShaderSource::fromFile("water.frag"), load));
// each frame:
if (compages::Status reloaded = m_watch.poll(); !reloaded) { std::cerr << reloaded.error(); }
```

## See also

[The GPU layer](../concepts/the-gpu-layer.md) · [Compute on the GPU](compute-on-the-gpu.md) · [Screen effects](screen-effects.md)
