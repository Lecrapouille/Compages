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
- [-] Shared uniforms of Sea in a `TypedUniformBlock`: not done, it is
      longer (a struct, a std140 block in two shaders, a bind) than three
      `program.set()`

### P3: fields

- [x] `Grid2D`: columns, rows, origin, spacing, index, neighbours
- [x] `Field<T>`: values on a grid, bilinear sampling
- [x] `intersect(Ray, Field<float>)` and `intersectHeight(Ray, height, …)`
- [x] Unit tests
- [x] Archipelago uses them: the cells of `Land` and `Making` are
      `Field<T>` on `CELL_GRID`, `Patch::heights` a `Field<float>`, `placeUnder`
      calls `intersectHeight`

### Documentation

- [x] doc/04 (ShaderSource), doc/06 (hooks, materials), doc/07 (Fields
      layer), doc/08 (API), skills

## TODO: scientific computing

In rough order. Each item should make a demo shorter or possible.

- [x] `scene.show(field, look)` with a colour map and a range (heat map,
      coloured terrain): values in a float texture, colours and relief made
      by the PBR hooks; demo `11a_HeatPlate`
- [x] A field on the GPU, drawn without coming back to the CPU:
      `scene.show(grid, StorageBuffer<float>, look)`; a `Field` shown from
      the CPU goes through a storage buffer too (partial update: later, with
      `DirtyRange`)
- [x] Compute shaders reading and writing fields: `11b_RippleTank` (three
      turning buffers; a `PingPong` of a `Field` type is not needed so far)
- [x] `Grid3D` and `Field<T, Grid3D>` (one `Field` template on its grid; same
      order as the texels of a 3D texture: `gpu::Texture::fromField()`)
- [ ] Volume rendering, slices, isosurfaces
- [ ] Vector fields drawn as arrows; streamlines
- [x] Inspection: `scene.probe(field, look, pixel)`, demo `11c_FieldProbe`
- [ ] Inspection: follow a point over time (a curve), slices
- [ ] 2D plots (time series, histograms) on a `CanvasTexture`
- [ ] Hooks in the shadow and depth passes
- [ ] Custom per-instance parameters of a material (typed block)
- [ ] Hot reload: watch the files listed by `ShaderSource`
- [ ] Topology for SimTaDyn maps (vertices, edges, polygons), fields on it
- [x] Demo: a bimetal beam bending with heat (`11d_BimetalBeam`: heat
      equation on a `Field<float>`, Timoshenko curvature, the strip bent by a
      `deform` hook reading a storage buffer)
- [x] Demos of fields: `11a_HeatPlate`, `11b_RippleTank`, `11c_FieldProbe`
- [ ] Demo: smoke (stable fluids in 2D with `PingPong`)

## Earlier decisions (audit of 2026-11-06)

Accepted: offscreen rendering to an image, one-line helpers (grid, axes,
gizmo, bounds), tone mapping and colour management, sorted transparency,
Windows / macOS / Web ports. Later: big data demos (lidar, a galaxy of one
million stars, octree). Refused: CMake export, the Routine vs ECS vs GPU
demo. Python bindings: separate repository `pycompages`.
