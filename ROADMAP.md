# Compages roadmap

What comes after `10e_Archipelago`, and how far it has gone. One line per
step: tick it in the commit that does it, so that work can stop and start
again at any step.

## Why

`10e_Archipelago` is the test of the API at full size. It shows three gaps:

1. **Shaders cannot be composed.** `Sea.cpp` cuts the text of the PBR shader
   (`find("void main()")`, `replace("vWorldPos = world.xyz;", …)`). A change in
   `ShaderLib.cpp` breaks the demo, and no C++ compiler tells.
2. **A custom material takes many steps**: a program, a pipeline of
   `MeshVertex`, sampler units set by hand, textures bound by hand each frame.
3. **Fields of cells are plain vectors** indexed by hand (`cellAt(i, j)`,
   `(j + 1) * (cells + 3) + i + 1`), and the ground under the mouse is found by
   a loop written in the demo.

## Decisions

- Keep `Drawable`, the buffers and `PingPong` as they are.
- One mechanism for shaders: **includes**. A file says `#include "x.glsl"` or
  `#include <name>`. C++ can give the text of a name (`include(name, text)`),
  which also covers GLSL generated from C++ data. A **hook** is an include
  with a default text, that a shader of the library declares; `hook(name,
  text)` replaces it, and an unknown hook name is an error, not a silence.
- No multi-stage `loadFiles({ … })` for now.
- `Material::create()` builds the program and the pipelines; the layout is
  always `MeshVertex`, so it is not a parameter.
- Typed uniform blocks already exist (`gpu::TypedUniformBlock<T>`).
- Structured data lives in `include/Compages/Fields/`, beside `Core/`: it
  includes only `Core`, and every layer may include it. Types are in the
  namespace `compages`. One `Field<T>` for scalars and vectors: no
  `ScalarField` / `VectorField` aliases.
- Compages gives data structures, not algorithms (option A): no island
  generator, no geology, no compute graph, no Forth. A field of a million
  cells is never a million entities.

## Critical notes on the audit

- Agreed: the diagnosis of `Sea.cpp`, the order shaders → materials → data.
- Too many shader concepts were proposed (raw GLSL, ShaderSource,
  ShaderLibrary, generated sources, hooks). Includes alone do it all.
- GLSL `#line` takes a source *number*, not a file name: the error log of
  the driver must be rewritten with a table number → file.
- A hook that moves vertices must also reach the shadow pass, or the shadow
  is wrong. The sea casts no shadow, so this waits (see TODO).
- The "cells" of Archipelago are one site per square of a regular grid, so
  `Grid2D` fits for real. Unstructured topologies (SimTaDyn maps, FEM meshes)
  are another domain: not now, but `Field<T>` must not prevent them.
- 2D plots and labels should be built on `CanvasTexture` and the
  distance-field letters that already exist.
- A job service stays in the demo (`Job.hpp`).

## Progress

### P1: shaders

- [x] `gpu::ShaderSource`: main text or file, include directories, virtual
      includes, hooks, `#define` after `#version`, include once, cycles
      reported, `#line` table, list of files read
- [x] `Program::load(ShaderSource, ShaderSource)`,
      `ComputeProgram::load(ShaderSource)`; compiler logs name the file and
      line; `loadFiles()` understands `#include`
- [x] PBR, lit, unlit and shadow shaders cut into shared includes
      (`compages/skinning.glsl`, `compages/lights.glsl`, `compages/fog.glsl`,
      `compages/present.glsl`)
- [x] Hooks `deform` (PBR vertex) and `main` (PBR fragment);
      `renderer::shaders::pbr()` returns the two sources (`ShaderPair`)
- [x] `Sea.cpp` and `10c_SweatAndDecals` use them: no more `find` / `substr`
      / `replace`

### P2: materials

- [x] `Material::create(name, MaterialDesc)`
- [x] Textures of a material by sampler name, bound by the Renderer
      (`Material::textures`, units 2 to 6)
- [x] `replaceMesh` and `wear` of Archipelago moved into the Renderer:
      `Scene::reshape(entity, mesh)`, `Scene::look(entity, MaterialId)`
- [x] The Renderer sets the uniforms of a family only when the shader reads
      them, so a `main` hook may leave the lights out
- [ ] Shared uniforms of Sea in a `TypedUniformBlock` (only if it is shorter)

### P3: fields

- [ ] `Grid2D`: columns, rows, origin, spacing, index, neighbours
- [ ] `Field<T>`: values on a grid, bilinear sampling
- [ ] `HeightField::intersect(Ray)`
- [ ] Unit tests
- [ ] Archipelago uses them (cells of `Land` and `Making`, `Patch`,
      `placeUnder`)

### Documentation

- [ ] doc/04 (ShaderSource), doc/06 (hooks, materials), doc/07 (Fields
      layer), doc/08 (API), skills

## TODO: scientific computing

In rough order. Each item should make a demo shorter or possible.

- [ ] `scene.show(field)` with a colour map and a range (heat map, coloured
      terrain)
- [ ] A field on the GPU: upload, partial update (`DirtyRange` exists), drawn
      without coming back to the CPU
- [ ] Compute shaders reading and writing fields (`PingPong` of fields)
- [ ] `Grid3D` and `Field3D`, then volume rendering, slices, isosurfaces
- [ ] Vector fields drawn as arrows; streamlines
- [ ] Inspection: probe a value under the mouse, follow a point over time
- [ ] 2D plots (time series, histograms) on a `CanvasTexture`
- [ ] Hooks in the shadow and depth passes
- [ ] Custom per-instance parameters of a material (typed block)
- [ ] Hot reload: watch the files listed by `ShaderSource`
- [ ] Topology for SimTaDyn maps (vertices, edges, polygons), fields on it
- [ ] Demos: a bimetal beam bending with heat (heat equation on a
      `Field<float>`, the beam bent by a `deform` hook); smoke (stable fluids
      in 2D with `PingPong`)

## Earlier decisions (audit of 2026-11-06)

Accepted: offscreen rendering to an image, one-line helpers (grid, axes,
gizmo, bounds), tone mapping and colour management, sorted transparency,
Windows / macOS / Web ports. Later: big data demos (lidar, a galaxy of one
million stars, octree). Refused: CMake export, the Routine vs ECS vs GPU
demo. Python bindings: separate repository `pycompages`.
