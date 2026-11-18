# Compages roadmap

What is left to do. One line per step: tick it, or remove it, in the commit
that does it. Done work is in the git history and the documentation, not
here.

The rule for every item: it makes a demo shorter, or it makes a demo
possible. The API keeps its contract: the shader names its data, and C++
never declares the same thing twice.

## Renderer

From the simplest to the most complex.

- [ ] Hooks in the shadows of the lamps (cube faces: the fragment writes the
      distance), as `Material::casts_shadow` does for the sun
- [ ] Custom per-instance parameters of a material, in a typed block (no
      table of strings read at each draw)
- [ ] Normals written by the main pass (same `deform`, skinning, normal maps
      and `discard` as the colour), then SSAO; a material that replaces
      `main` does not write them: document it
- [ ] History per view, then TAA: reprojection, previous matrices, motion
      vectors, reset on a camera cut or a resize
- [ ] Vector field on the GPU: `scene.show(grid, StorageBuffer<Vector2f>,
      look)`, arrows instanced and read from the buffer (the velocity of
      `11e_Smoke` cannot be shown today without the CPU)
- [ ] Automatic instancing: the queue grouped by mesh and material
      instance, the matrices in a storage buffer; measured on
      `09b_ManyCubes`. Needed by grass, impostors and the trees of
      `10e_Archipelago`
- [ ] A regular `Grid2D` drawn from `gl_VertexID`, without a buffer of
      positions (internal, measure first)

## Science and fields

- [ ] Histograms on a `Chart`
- [ ] Stretched rectilinear grid (one array of coordinates per axis): a cheap
      middle step before adaptive grids
- [ ] Quadtree, adaptive refinement, as a domain of its own: fine zones that
      move (a flame front, a shock), neighbours across levels, T-junctions,
      neighbour tables for compute shaders. This is adaptivity, not level of
      detail: the LOD of a far terrain belongs to the renderer
- [ ] Sparse 3D grid (dense 8^3 blocks only where there are values, like
      OpenVDB): memory, and empty space skipped by ray marching (3D smoke,
      lidar). A point with no block holds `zero<T>()`. The grid numbers only
      the stored points, so `Field` keeps one contiguous array
- [ ] Topology for SimTaDyn maps (vertices, edges, polygons), fields on it
- [ ] `CurvilinearGrid` cheap to copy (its points shared and immutable
      inside), so that `Field<T, Grid>` stays by value
- [ ] Partial upload of a field (`DirtyRange`, rows j0 to j1): only when a
      demo paints into a large field
- [ ] `11b_RippleTank` and `11e_Smoke` on `PingPong` where it fits
- [ ] Later, big data demos: lidar, a galaxy of one million stars, an octree

Keep one rule: never three independent axes (kind of data, topology,
storage). The grid numbers the points, the field holds one array:
`Field<T, Grid>`, not `Field<T, Dim>`.

## RobotIK

What RobotIK still cannot do, or cannot do simply, with this Compages.
Who owns what: RobotIK owns the clock of the simulation, the physics
(MuJoCo, Pinocchio) and the models of its sensors (noise, range, rate).
Compages owns the pictures: what a camera sees, its depth, its entity ids,
and the helps drawn for the operator. RobotIK keeps `compages::world` as its
ECS; it does not build a second model of the robot.

### Headless, checked by the linker

`Compages.mk` makes one library per layer (`Compages-World`,
`Compages-GPU`, `Compages`), and `Compages-WorldTest` links
`Compages-World` alone. `Compages-Headless` gives an OpenGL context with no
window (`HeadlessContext`, EGL) for camera tests in CI.

- [ ] RobotIK: `robotik-core` and `Robotik-Headless` depend on
      `Compages-World` only, the simulator on `Compages` (RobotIK side)

### Clocks

Today one `Frame` carries one time. RobotIK gives the time of the wall to
`world->update(viewFrame(...))` at each picture, while its simulation has
its own time (`simulation->time()`, steps of `SIMULATOR_DT_S`). So a routine
of Compages runs on the time of the wall even when the simulation is paused,
and the animation clips never run (`Scene::update()` would also run the
routines).

- [ ] The clips without the routines: a call that samples the animation
      clips and computes the world matrices, for a host that steps the world
      itself. A clip sampled at an absolute time, not by adding steps, so
      that the history slider of RobotIK shows the same pose every time.
      This also fixes the skinned meshes of RobotIK left in their rest pose
- [ ] Each picture knows the time of the world it shows: a `RenderTarget`
      keeps the `total` of the last update before its render. Needed as soon
      as the read-back is one frame late (below): an observation is stamped
      with the time of the simulation, never with the time it was read

### Cameras: RGB-D first, then segmentation

`RenderTarget` already gives the colour (`readColor`) and the depth in metres
(`readDepth`), and RobotIK fills its `CameraFrame` from them. Both stop the
GPU, and the lens is only a vertical field of view.

- [ ] A lens from the intrinsics of a calibrated camera (`fx`, `fy`, `cx`,
      `cy`, in pixels): an off-centre principal point. RobotIK turns them
      into a field of view today and loses `cx`, `cy`. Lens distortion stays
      a model of RobotIK
- [ ] Asynchronous read-back: ask the read after the render, take it one
      frame later (pixel buffers and a fence, which GL41 and GLES3 also
      have). `readColor()` and `readDepth()` stay for tests and files. Needed
      by several robot cameras at 30 Hz
- [ ] Entity ids written by the main pass, in an integer picture of the
      target (asked when the target is made): one `EntityId` per pixel.
      RobotIK maps an entity to its class for semantic segmentation. Also
      gives `pick()` to the pixel (the `Picker` tests boxes on the CPU today)
- [ ] From depth to points: each pixel of the depth as a point in the frame
      of the camera or of the world, so that RobotIK never sees the reversed
      depth or the projection
- [ ] Draw a point cloud that changes each frame (`DebugDraw` has lines,
      arrows, frames and frustums, no points): the output of an RGB-D camera
      or of a LiDAR
- [ ] An `rgbd` camera in a RobotIK scenario, to show the depth it already
      fills (RobotIK side)
- [ ] The noise of a real camera (blur, grain): possible with a
      `ScreenEffects` on the robot camera; a recipe in the RobotIK docs

### LiDAR, prepared but not written

The LiDAR stays in RobotIK: its `RangeScanner` casts its rays in the physics
backend, and noise, range and scan rate are its models. Compages only makes
a second way possible, on the GPU, for many beams:

- [ ] A depth picture drawn by several cameras of one rig (four views of
      90°, through `Camera::viewport` in one target), read in metres in one
      read-back, each part with its own lens. RobotIK picks its beams in it.
      A recipe first; a call only if the recipe is long

### Other points

- [ ] Its lock points to an old Compages (2c39088): a plain `make` fails. It
      builds only with `make CompagesProject.PATH=$HOME/MyGithub/Compages`.
      Commit Compages, move the lock, fetch again
- [ ] `Robotik-LineFollower` does not link: the system `libgdal` needs
      `expat` (environment, not Compages)
- [ ] Occupancy and cost maps shown with `scene.show(field, look)`: the
      floor of LineFollower or the room of MicroDuck (nothing to add to
      Compages)
- [ ] Demonstrator: PickAndPlace with a wrist camera, after the items
      above. It runs headless (tests, `Compages-World` only), debug (frames,
      paths, skills) and perception (camera pictures)

## AAA techniques

Published techniques of game studios, rebuilt with our own code, pictures
and scenes (credits in `doc/reference/credits.md`). From the simplest to the
most complex: each step reuses what the ones before it built. Each one gets
a page in `doc/techniques/`.

Done: flow maps (`10f_FlowingWater`), metallic-roughness PBR, clustered
forward lighting (`06m_ManyLights`), cel shading and outlines
(`06o_CelShading`), volumetric fog in froxels (`06n_VolumetricFog`), screen
filters and bloom (`12a`, `12b`).

- [ ] Interior mapping (van Dongen, CGI 2008): rooms behind windows faked by
      one hook, no geometry
- [ ] Rain and wet surfaces (Lagarde, "Water drop" series, Remember Me
      2012-2013): darker and glossier when wet, puddles from a height mask,
      ripples of drops; reuses `shadePbr()` and the flow maps
- [ ] A sky in an environment map, prefiltered by roughness, read by the
      glint of `shadePbr()`: metals reflect the scene, not a flat ambient
- [ ] Shadows for more than four lamps: an atlas filled by importance
      (distance, intensity, on screen), the others through the clusters
- [ ] Ambient occlusion in screen space (GTAO, Jimenez et al. 2016): needs
      the normals of the main pass
- [ ] Temporal anti-aliasing (Karis, SIGGRAPH 2014): jittered camera, motion
      vectors, a history picture
- [ ] Screen-space reflections: a room with a wet floor
- [ ] More screen filters (chapter `12_Effects`): depth of field, motion
      blur, refraction, film grain, colour table (LUT). A last demo lists
      every filter with a check box. The list of effects follows Lettier's
      *3D Game Shaders For Beginners*; never copy its text or pictures
- [ ] Procedural grass and wind (Ghost of Tsushima, Wohllaib GDC 2021):
      blades instanced by a compute shader, bent by a wind `Field`
- [ ] Octahedral impostors (Brucks, Fortnite 2018): far trees and props as
      pictures baked from many directions
- [ ] Geometry clipmaps (Losasso and Hoppe, SIGGRAPH 2004; GPU Gems 2,
      chapter 2): nested rings of terrain round the camera, for
      `10e_Archipelago`
- [ ] Ocean by FFT (Tessendorf, SIGGRAPH 2001): a spectrum of waves, FFT in
      compute shaders (ping-pong), in place of the Gerstner swells of
      `10e_Archipelago`
- [ ] Volumetric fog, next: reprojection over frames (less noise), lights
      that only light the fog
- [ ] Shadows and occlusion from distance fields (Wright, UE4, SIGGRAPH
      2015): signed distance fields of the meshes, on the sparse 3D grid
- [ ] Last: a simple dynamic global illumination (emissive surfaces,
      indirect light, moving objects)

Technique pages still missing for work already done: normal mapping, vertex
deformation (`deform` hook).

## Later: one WebGPU backend (decided 2026-11-17, not started)

**Why.** macOS stops at OpenGL 4.1 and the web gives WebGL 2: neither has
compute shaders, `shared` memory, atomics or images. GL41/GLES3 emulate part
of compute (transform feedback, `<compages/tiles.glsl>`) and the reads of
storage blocks (copies in textures); the rest stays GL45 only: `05g` (one
invocation writes 337 values), `09c` (`atomicAdd`, indirect draw), the
volumetric fog (images). WebGPU gives full compute everywhere, with one C API
(`webgpu.h`): Dawn or wgpu-native natively (Vulkan, Metal, D3D12), Emscripten
in the browser.

**What stays.** Users write GLSL and fill names from C++; the World and the
Renderer do not see the backend. GLSL is translated at run time (glslang →
SPIR-V → Tint or naga → WGSL), the reflection taken from the result.

**What it costs.**

- Translation at run time, also in the page (a few MB of WebAssembly):
  shaders are composed while running (hooks, defines, hot reload).
- Loose uniforms (213 in the repository) do not exist in WebGPU: gather them
  into a generated block, written per draw into a ring buffer.
- `sampler2D` is one object in GLSL, a texture and a sampler in WGSL.
- Missing in WebGPU: geometry and tessellation shaders, `LineLoop`,
  `TriangleFan`, wireframe, lines wider than 1 px, `gl_PointSize`, RGB8 and
  1D textures, doubles in shaders.
- Immutable pipelines, asynchronous read-back, recorded commands.
- Firefox is not WebGPU everywhere yet: GLES3 stays as the fallback.
- RobotIK must follow the API.

**First step: a prototype of the risk**, before any backend. A Compages GLSL
shader → SPIR-V → WGSL at run time, natively and in the browser, with the
loose uniforms gathered, `sampler2D` split and the names found by reflection;
checked on `01c`, `03a` and `05a`. Then the backend beside GL45, the gallery
at parity, and only then decide what to remove.

## Open questions

- An optional module `Compages/App/` (window, loop, inputs to `ViewFrame`),
  outside the three layers and the only place with GLFW? RobotIK and the
  gallery each write their own window. It touches a rule of AGENTS.md.
  `Compages-Headless` (EGL, no window) is already such a module, outside the
  layers.
- The `state` of a material: keep it required (no silent default), or offer
  named presets (`solid()`, `seeThrough()`)?
- A `dispatch` with several named buffers: it would shorten `11e_Smoke` by a
  few lines only.

## Refused

So that they are not proposed again: a `GpuField` type (a grid and a
`StorageBuffer` already do it), a compute graph or `ComputeSequence`,
`gpu::run`, clusters built on the GPU, a benchmark tree compared with other
engines (the gallery shows CPU and GPU time per demo instead), one C++ class
per screen effect (`effects().add<Bloom>()`), `Shading::Toon`, hooks
`lighting` and `postLighting`, multi-stage `loadFiles({ … })`, an island
generator or any algorithm of a demo in the library, CMake export, the
Routine vs ECS vs GPU demo. Python bindings live in a separate repository
(`pycompages`).

For RobotIK: a robot model of RobotIK apart from `compages::world` (RobotIK
keeps the World as its ECS; one model, not two kept in step); classes of
RobotIK in Compages (an adapter, `showSkill`, `showResource`: they live in
RobotIK, from the generic `DebugDraw`); a `renderer.capture(camera,
{channels})` beside `RenderTarget` (one way to draw into a picture); fields
that live on the GPU (already refused as `GpuField`); a render between two
states (`render(alpha)`: 1 kHz of physics against 60 Hz of pictures, the gap
cannot be seen); a viewer of many RL environments, or a generator of signed
distance fields, in Compages (a `Field<float, Grid3D>` already stores one).
The URDF Z-up to Y-up tests asked for already exist
(`tests/World/UrdfLoaderTests.cpp`).
