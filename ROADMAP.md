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
scenes of our own (credits in doc/reference/credits.md). From the simplest to the
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
- [x] Volumetric fog in froxels (Wronski, Assassin's Creed 4, SIGGRAPH
      2014): `Environment::volumetric`, `FogVolume` (128 x 72 x 64, inject
      then integrate by compute; sun and lamp shadows, clustered lamps),
      `06n_VolumetricFog`. Later: reprojection over frames (less noise),
      lights that only light the fog
- [ ] Shadows and occlusion from distance fields (Wright, UE4, SIGGRAPH
      2015): signed distance fields of the meshes, on the sparse 3D grid of
      P3

## After the audit of 2026-11-15

From the simplest to the most complex. Each step must make a demo shorter
or possible. Refused: a `GpuField` type (a `Grid` and a `StorageBuffer`
already do it), a compute graph, a `gpu::run` beside `dispatch`, clusters
built on the GPU, a benchmark tree compared with other engines, one C++
class per screen effect.

- [x] Grids with a size of zero or less hold no point (`count()` 0,
      `valid()`): a `Field` on a wrong grid is empty, not huge
- [x] `LightClusters::build()` keeps its memory from frame to frame
- [x] Fog sampler always on its unit (Mesa refused the draws: glTF duck
      test, first frame of every `11_Science` demo)
- [x] Helps for the operator: `RenderFlags::Helper`,
      `world::Camera::sees_helpers` (a robot camera does not see them)
- [x] Hook `surface` of the PBR shader (`Surface`: albedo, normal,
      metallic, roughness, emission, opacity); `10f` concrete, `11d` metal
- [x] `AssetManager::createMaterial(name, desc)`, `.textures` and
      `.buffers` in `MaterialDesc`
- [x] CPU and GPU time of a frame (`gpu::beginGpuTimer()`,
      `gpuMilliseconds()`), shown by the gallery
- [x] RobotIK: its teach markers marked `Helper`, no more hide/show in
      `RenderedCamera::capture()`; `Texture::create` / `fromFile` in place of
      the removed `allocate` / `loadFile` (RobotIK builds and passes its tests
      on this Compages)
- [x] Hook `light` of the PBR shader (`vec3 light(Surface s)`, default
      `sceneLight(s)`; `toScreen(s, lit)` after it), checked by
      `06o_CelShading` (flat tones, an outline from a shell inside out)
- [x] A render target of the `Scene` (`RenderTarget`: colour, depth,
      `resize`, `readColor` top row first), used by `renderToFile()`
- [x] RobotIK: its own `RenderTarget` struct replaced by Compages' one (3D
      view, model preview, robot cameras, `--screenshot` by `saveColor`);
      the camera frames written from the CPU keep a plain texture
      (`ShownPicture`); `Screenshot.cpp` and its stb copy are gone
- [x] Colour in linear light until a last pass, for a view that has
      screen effects (`linearTarget`: `present()` keeps the light linear;
      `screen()` turns a screen colour back into light, so unlit looks,
      sky, lines and background keep their colour)
- [x] Screen filters per view: `ScreenEffects` on a camera, a filter is
      `vec3 effect(vec2 uv)` reading `screenColor`, `screenDepth`,
      `screenSize`; bloom; chapter `12_Effects` (`12a_ScreenFilters`,
      `12b_Bloom`)
- [x] Metric depth read back (`RenderTarget::readDepth(lens)`)
- [ ] Entity ids in the main pass (segmentation for RobotIK, `pick()` to
      the pixel)
- [x] Diagnostic primitives in `DebugDraw`: `polyline`, `arrow`, `frame`
      (a pose), `frustum` (what a camera sees, in `06j_SplitViews`)
- [ ] Normals written by the main pass, SSAO; then history per view, TAA
- [ ] Vector field on the GPU and instanced arrows; automatic instancing

### What blocks RobotIK

What RobotIK still cannot do, or cannot do simply, with this Compages:

- [ ] **Its lock points to an old Compages.** `external/versions.lock` names
      2c39088 and `external/Compages` is an older clone (cd14d14): a plain
      `make` fails (`Vector3g`, `Quatd`, `Posed`, `yaw` missing). It builds
      only with `make CompagesProject.PATH=$HOME/MyGithub/Compages`. To do:
      commit Compages, move the lock to that commit, fetch again.
- [x] **Depth of a robot camera in metres.** `RenderTarget::readDepth(lens)`
      gives the distance along the axis, top row first (+infinity where
      nothing was drawn). RobotIK fills `CameraFrame::depth` with it for an
      `rgbd` camera: until now the simulator left it empty, so its
      `DepthEstimator` had nothing to read. No scenario has an `rgbd`
      camera yet to show it.
- [ ] **Segmentation.** No id of entity per pixel: a perception test cannot
      know which object a pixel shows. Wanted: an id target in the main pass,
      read back like the colour; `pick()` to the pixel comes with it.
- [ ] **A robot camera reads its picture by stopping the GPU.**
      `readColor()` waits for the frame (fine at a few images per second,
      not at 30 Hz with several cameras). Wanted: an asynchronous read
      (pixel buffer, read one frame later).
- [ ] **The noise of a real camera.** Blur, distortion of the lens, grain:
      now possible (a `ScreenEffects` on the entity of the robot camera, read
      back by `readColor()`), but no filter is written yet and the docs of
      RobotIK have no recipe.
- [ ] **Skinned meshes stay in their rest pose in RobotIK**: it calls
      `World::update` and `Scene::render`, never `Scene::update`, so the
      animation clips never run (no humanoid skin moves).
- [ ] **`Robotik-LineFollower` does not link** on this machine: the system
      `libgdal` needs `expat` (not Compages; environment).

Fixed on the way: a package's `background` was ignored (the colour went to
the render pass, and `Scene::render` clears with `scene.background()`); it
is now given to the Scene.

## Earlier decisions (audit of 2026-11-06)

Accepted: offscreen rendering to an image, one-line helpers (grid, axes,
gizmo, bounds), tone mapping and colour management, sorted transparency,
Windows / macOS / Web ports. Later: big data demos (lidar, a galaxy of one
million stars, octree). Refused: CMake export, the Routine vs ECS vs GPU
demo. Python bindings: separate repository `pycompages`.
