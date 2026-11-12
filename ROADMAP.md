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
- [x] Volume rendering (`scene.show(Field<float, Grid3D>)`, ray marching a 3D
      texture) and slices (`compages::slice()`), demo `11f_Volume`
- [x] Isosurfaces (`compages::isosurface()`, marching tetrahedra;
      `renderer::meshOf()`), in `11f_Volume`
- [x] Vector fields drawn as arrows (`scene.show(Field<Vector2f>, ArrowLook)`)
      and streamlines (`compages::streamline()`), demo `11g_FlowAroundCylinder`
- [x] Inspection: `scene.probe(field, look, pixel)`, demo `11c_FieldProbe`
- [x] Inspection: follow a point over time (`Chart`, demo `11h_Thermometers`),
      slices (`compages::slice()`)
- [x] Time series on a `CanvasTexture` (`Chart`)
- [x] Axes with numbers on a `Chart`, a values axis that follows the
      values (`ChartScale::FollowValues`)
- [ ] Histograms on a `Chart`
- [x] Hooks in the shadow of the sun: a material whose `deform` is replaced
      draws its own shadow (`Material::casts_shadow`)
- [ ] Hooks in the shadows of the lamps (cube faces: the fragment writes
      the distance)
- [x] Shadows of what the camera does not see: the snapshot keeps the
      casters out of view (`RenderItem::in_view`), the sun stands beyond them
- [ ] Custom per-instance parameters of a material (typed block)
- [x] Hot reload: `gpu::ShaderWatch` watches the files listed by
      `ShaderSource` and calls the load of the user
- [ ] Topology for SimTaDyn maps (vertices, edges, polygons), fields on it
- [ ] Grids with more detail where it matters. In order:
  - [x] A C++20 concept of grid (`PlanarGrid`: `count`, `index`, `point`,
        `locate` → `GridCell`), so that `Field<T, Grid>::sample()`,
        `streamline()` and `scene.show()` take other grids; for a
        non-regular grid, finding the cell of a point is a search, not a
        division
  - [x] Curvilinear grid (`CurvilinearGrid`, body-fitted, an O-grid around a shape): same
        (i, j) neighbours, contiguous values and GPU upload, each point
        placed freely; `show()` keeps its mesh by `gl_VertexID`. Fixes the
        steps around the column of `11g_FlowAroundCylinder` (polar grid,
        tight at the wall, loose far away)
  - [ ] Stretched rectilinear grid (one array of coordinates per axis): a
        cheap middle step
  - [ ] Quadtree / adaptive refinement, as a domain of its own (like the
        SimTaDyn topology), for fine zones that move (a flame front, a
        shock): neighbours across levels, T-junctions, neighbour tables for
        compute shaders. This is adaptivity (the data have fine and coarse
        zones), not level of detail (choosing a resolution by the distance
        to the camera): the LOD of a far terrain belongs to the renderer
        (`Patch`), not to `Fields`
  - [ ] Sparse 3D grid (dense 8^3 blocks only where there are values, like
        OpenVDB): memory and empty space skipped by the ray marching of
        mostly empty volumes (3D smoke, lidar). A point with no block holds
        the background value: `zero<T>()` of `Core/Maths.hpp`, the neutral
        element of the algebra, which a type can redefine (minus infinity
        for (max, +)). The sparse grid numbers only the stored points
        (`index()` through a table of blocks), so `Field` keeps one
        contiguous array and no storage parameter
- [ ] Never three independent axes (kind of data, topology, storage): a
      quadtree is its own storage, a sparse grid is a topology; the grid
      numbers the points, the field holds one array. `Field<T, Grid>`, not
      `Field<T, Dim>`: two grids of the same dimension (regular,
      curvilinear) are different grids
- [x] Demo: a bimetal beam bending with heat (`11d_BimetalBeam`: heat
      equation on a `Field<float>`, Timoshenko curvature, the strip bent by a
      `deform` hook reading a storage buffer)
- [x] Demos of fields: `11a_HeatPlate`, `11b_RippleTank`, `11c_FieldProbe`
- [x] Demo: smoke (`11e_Smoke`: Stable Fluids in 2D, five compute kernels
      from one GLSL text chosen by `ShaderSource::define`)

## TODO: public techniques of games

Published techniques of game studios, rebuilt with code, pictures and
scenes of our own (credits in doc/11-Credits.md). From the simplest to the
most complex: each step reuses what the ones before it built.

- [x] Flow maps (Valve, Grimes GDC 2011, Vlachos SIGGRAPH 2010):
      `10f_FlowingWater`
- [x] Metallic-roughness shading of glTF (Karis, UE4 SIGGRAPH 2013):
      `shadePbr()`, normal and metallic-roughness maps
- [x] Clustered forward lighting (Olsson et al., HPG 2012; Doom 2016,
      SIGGRAPH 2016): `LightClusters`, `06m_ManyLights`
- [ ] Cel shading and outlines (Guilty Gear Xrd, Motomura GDC 2015): a
      hook `main` with light in steps, and an outline pass
- [ ] Interior mapping (van Dongen, CGI 2008): rooms behind windows faked
      by one hook `main`, no geometry
- [ ] Rain and wet surfaces (Lagarde, "Water drop" series, Remember Me
      2012-2013): darker and glossier when wet, puddles from a height mask,
      ripples of drops; reuses `shadePbr()` and the flow maps
- [ ] A sky in an environment map, read by the glint of `shadePbr()`
      (prefiltered by roughness): metals reflect the scene, not a flat
      ambient
- [ ] Shadows for more than four lamps: an atlas filled by importance
      (distance, intensity, on screen), the others through the clusters
- [ ] Ambient occlusion in screen space (GTAO, Jimenez et al. 2016): a
      pass over the depth and normals
- [ ] Temporal anti-aliasing (Karis, SIGGRAPH 2014): jittered camera,
      motion vectors, a history picture
- [ ] Procedural grass and wind (Ghost of Tsushima, Wohllaib GDC 2021):
      blades instanced by a compute shader, bent by a wind `Field`
- [ ] Octahedral impostors (Brucks, Fortnite 2018): far trees and props as
      pictures baked from many directions
- [ ] Geometry clipmaps (Losasso and Hoppe, SIGGRAPH 2004; GPU Gems 2,
      chapter 2): nested rings of terrain round the camera, for
      `10e_Archipelago`
- [ ] Ocean by FFT (Tessendorf, "Simulating Ocean Water", SIGGRAPH 2001):
      a spectrum of waves, FFT in compute shaders (ping-pong), in place of
      the Gerstner swells of `10e_Archipelago`
- [ ] Volumetric fog in froxels (Wronski, Assassin's Creed 4, SIGGRAPH
      2014): a `Field<float, Grid3D>` of light scattering, filled by compute
      over the slices of the clusters, ray marched in the picture
- [ ] Shadows and occlusion from distance fields (Wright, UE4, SIGGRAPH
      2015): signed distance fields of the meshes, on the sparse 3D grid of
      P3

## Earlier decisions (audit of 2026-11-06)

Accepted: offscreen rendering to an image, one-line helpers (grid, axes,
gizmo, bounds), tone mapping and colour management, sorted transparency,
Windows / macOS / Web ports. Later: big data demos (lidar, a galaxy of one
million stars, octree). Refused: CMake export, the Routine vs ECS vs GPU
demo. Python bindings: separate repository `pycompages`.
