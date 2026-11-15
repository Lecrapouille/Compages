---
name: compages-renderer
description: Write or change Compages Renderer-layer code (compages::renderer): Scene (primitives, looks, camera, sun, lamps, skybox, picking, debug lines), AssetManager and loaders (glTF/GLB, STL, URDF, prefabs), animation clips, SceneExtractor/RenderSnapshot/Renderer, several views of one world, CanvasTexture live painting. Use for 3D scenes, model loading, rendering bugs, split views.
---

# Compages Renderer layer

`Scene` is a façade over a `World`: it returns `world::Entity` handles, keeps
meshes/materials/clips in an `AssetManager`, snapshots the world, then calls
`gpu::`. Header `<Compages/Compages.hpp>`.

```cpp
compages::world::World world;
compages::renderer::Scene scene(world);
scene.background(0.04f, 0.05f, 0.08f);
scene.camera().position(0.0f, 1.0f, 3.0f).add<compages::world::Orbit>();  // place, then Orbit
scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
compages::world::Entity cube = scene.box("Cube", compages::renderer::color(0.9f, 0.18f, 0.12f));
COMPAGES_TRY(scene.prepare());           // uploads, returns first construction error
// per frame
cube.rotate(frame.elapsed, { 0.4f, 1.0f, 0.0f });
scene.draw(frame);                       // = update(frame) + render()
```

- Primitives: `box sphere plane cylinder cone pyramid (name, look)`; looks:
  `color(r,g,b)`, `texture("f.jpg")`, `unlit(r,g,b)`, `normals()`, `depth(near, far)`,
  `seeThrough(look, opacity)` (drawn after the solid ones, farthest first, blended, no depth write).
- Colours given to a Scene (looks, lights, ambient, fog) are sRGB; MaterialInstance and the
  snapshot hold linear (`srgbToLinear`, done in Looks and the extractor). Shaders end with
  `present()` = exposure + tone curve (`environment().tone_mapping`, `.exposure`) + sRGB encode.
- Screen effects per view: `camera.set(ScreenEffects{ .filters = { { .name, .glsl, .values } }, .bloom = {
  .enabled, .threshold, .strength } })` (`Components/ScreenEffects.hpp`). The view is then drawn into an
  RGBA16F picture (`src/Renderer/Render/ScreenPost`), `present()` stays linear (`linearTarget`), the
  last pass does tone curve + sRGB. A filter is `vec3 effect(vec2 uv)` reading `screenColor`,
  `screenDepth` (reversed), `screenSize`. Shaders writing a screen colour use `screen(c)` (inverse
  tone curve) so it stays the same; demos `12a_ScreenFilters`, `12b_Bloom`.
- Own material: `auto pbr = shaders::pbr();` (`ShaderPair`, `Procedural/ShaderLib.hpp`; also `lit()`,
  `unlit()`, `depth()`, `normals()`, `shadow()`, `lampShadow()`), hooks `pbr.vertex().hook("deform",
  "void deform(inout vec4 world, inout vec3 normal) {...}")`, `pbr.fragment().hook("surface",
  "void surface(inout Surface s) {...}")` (preferred: change albedo/normal/metallic/roughness/
  emission/opacity, the Renderer lights it, fogs it, tone-maps it), `hook("light", "vec3 light(Surface
  s) {...}")` for another model of light (cel shading `06o`; default `sceneLight(s)`, linear, before
  emission/fog) and `hook("main", ...)` only for unlit or multi-lighting looks (writes `oColor`;
  `instanceSurface()`, `surface(s)`, `light(s)`, `toScreen(s, lit)`,
  `shade`, `applyFog`, `present`... included; library pieces `<compages/*.glsl>`).
  `assets().createMaterial(name, { .family, .shaders, .state, .see_through, .textures = { {
  .sampler, .texture } }, .buffers = { { .block, .buffer } } })` → MaterialId (units 2-6, bound by
  the Renderer; `Material::create` + `addMaterial` is the two-step form), then
  `scene.look(entity, materialId)`; `scene.reshape(entity, mesh)` replaces a mesh in place. The
  Renderer sets family uniforms only if the shader reads them (`src/Renderer/Render/Offer.hpp`).
  Demos: `10e_Archipelago` (Sea.cpp), `10c_SweatAndDecals`, `11d_BimetalBeam` (surface + emission).
- `scene.debug()` (DebugDraw, one frame): line, ray, box, polyline, arrow, frame(pose, length),
  frustum(pose, lens, aspect, length, colour) (06j_SplitViews).
- Helps for the operator: `RenderFlags::Helper` on a MeshRenderer; a camera with
  `world::Camera::sees_helpers = false` (a robot sensor) skips them, grid/axes/bounds and
  `scene.debug()` lines (left queued for the next view).
- Show a field: `scene.show(Field<float>, FieldLook{ .colors = ColorMap::Viridis|Inferno|Coolwarm|
  Terrain|Grey, .low, .high, .relief, .lit }, name)` → entity with a `FieldView` component;
  `scene.show(entity, field, look)` writes new values (same grid size); GPU values:
  `scene.show(grid, StorageBuffer<float>, look, name)` / `show(entity, buffer, look)` (borrowed).
  Values in a storage buffer read by `gl_VertexID` + PBR hooks (`src/Renderer/Visualization/FieldViews.cpp`),
  relief clamped to the range; sun shadow from its own program (`Material::casts_shadow`, made by
  `Material::create` when the vertex replaces "deform"; no lamp shadow). `scene.probe(field, look, pixel)` → point + value.
  `Material::buffers` = storage buffers bound by the Renderer. Demos `11a_HeatPlate`,
  `11b_RippleTank` (compute), `11c_FieldProbe` (Tooltip). Volumes: `scene.show(Field<float, Grid3D>,
  look, name)` (box + ray marching a 3D texture, see-through, no shadow), `compages::slice()`
  (`Fields/Slice.hpp`) for cuts, `compages::isosurface()` (`Fields/Isosurface.hpp`) +
  `renderer::meshOf(surface)` for the surface of a level; demos `11e_Smoke`, `11f_Volume`. Vectors: `scene.show(Field<Vector2f>,
  ArrowLook{ .colors, .low, .high, .scale, .every }, name)` (CPU mesh), `compages::streamline()`
  (`Fields/Streamline.hpp`); demo `11g_FlowAroundCylinder`. Curves over time: `Chart`
  (`Visualization/Chart.hpp`: create(scene, {width, height, seconds, low, high}, name), series, add,
  draw, applyLook) on a CanvasTexture (`paintPixelLine`, `fillPixels`); demo `11h_Thermometers`.
- Structured data (`Fields/`, namespace `compages`, includes only Core): `Grid2D`, `Field<T>`
  (`sample` bilinear), `CurvilinearGrid::create(columns, rows, place(i, j))` (body-fitted, `locate()` is a
  search; `show()` takes it too, demo `11g`), concept `PlanarGrid` (`locate` → `GridCell`), `intersect(ray, Field<float>)`, `intersectHeight(ray, h(x,z), from, to, step)`.
- Helpers: `scene.grid(size, divisions)`, `scene.axes(length)` (hang from an entity = gizmo),
  `scene.showBounds(id, bool)`; components in `Renderer/Components/Helpers.hpp`, drawn as
  DebugDraw lines at each render.
- Text (`Renderer/Components/Text.hpp`): `scene.font(ttf)` once, then `entity.set(Label{ .text,
  .color, .pixels, .hide_closer_than })` (name next to an entity) and `camera.set(Caption{ .text,
  .color, .pixels, .at })` (fixed place of that camera's picture, top-left pixels, '\n' lines;
  update `.text` each frame). SDF atlas baked by stb_truetype (`src/Renderer/Render/TextDraw.*`,
  Latin-1), one draw after the debug lines; caption first, a label covering earlier text is
  skipped; labels not occluded by geometry. Font in Compages-data `fonts/`. Prefer it to the
  gallery `hud()` (ImGui, gallery only).
- The Scene pass uses reversed depth (`PassDesc::reversed_depth`, `CameraFrame::reversed_projection`,
  glClipControl 0..1, compares mirrored in the backend). Drawing over a Scene picture with its depth:
  open a pass with `reversed_depth = true`, use `lastCamera().reversed_projection` (10b_DoomLike particles).
  Planet scales: floating origin (double sim outside in `Vector3m`, write `toMetres(p - origin)`), `10d_EarthMoon`.
- `RenderTarget::create(w, h)` + `scene.render(camera, target)`: off-screen colour + Depth32F,
  `readColor()` RGBA8 top row first, `readDepth(lens)` metres along the axis (+inf = nothing),
  `resize()` no-op on same size (robot cameras, thumbnails).
- `scene.renderToFile("out.png", w, h)` (after update, built on RenderTarget); `Compages-headless` makes an EGL
  context with no surface; `gpu::Texture::saveFile(path)` writes a 2D R8/RGB8/RGBA8 PNG.
- Files: `scene.loadFile(path)` → `Result<Entity>` (glTF/GLB, prefab, STL, URDF
  with joints); `assets().loadFile(path)` → `PrefabId`, `instantiate(id)`;
  `frameAll()`; `play(model, "Walk")`, `clips(model)`; `copy(entity)` shares
  mesh and look. Assets from `external/Compages-data/` or `COMPAGES_DATA_PATH`.
- Lighting: the 4 lamps nearest the camera get uniforms (`pointLightPos0`...) and shadows; up to 256 more
  go through clusters (`Render/LightClusters`, 16x9 tiles x 24 log depth slices, storage bindings 9-11 of the
  lit shaders: keep them free), read by `pointLightsContribution()` and `shadePbr()`; demo `06m_ManyLights`.
  Volumetric fog: `environment().volumetric` (`Render/FogVolume`: froxels 128x72x64, two compute shaders
  from `shaders::compute(name, text)`, read by `applyFog()`, texture unit 10); demo `06n_VolumetricFog`.
  glTF instances use `shadePbr` (metal/rough factors + maps, normal map via `bumpNormal`, units 8 and 9;
  uniforms prefixed `pbr`), looks keep `shade()`.
- Environment: `ambient`, `skybox({6 faces})`, `lamp(name, color, intensity, range)`,
  `activeCamera(id)`, `pick({x, y})` (bottom-left origin, last render), `debug()`.
- Several views: `Scene map(world, scene.assets());` then `scene.update(frame);
  scene.render(); map.render(map.camera().id());`.
- Frame order: `World::step(frame)` (routines) → `AnimationSystem` →
  `World::update()` once (joints, matrices) → skinning → `SceneExtractor` →
  `RenderSnapshot` (sorted by material then mesh, culled) → `Renderer`
  (`ShadowPass` for the shadows, `Skybox` behind). Folders: `Scene/` (Scene, Looks, Environment, RenderSettings), `Assets/`,
  `Import/` (glTF, STL, URDF visuals), `Procedural/`, `Animation/`, `Render/`.
- Live texture (`Renderer/Procedural/CanvasTexture.hpp`, demo `06l_CanvasPaint`):
  `create(w, h)`, `setMapping(AABB local, flip_u, flip_v)`, `bind(scene.assets(), "name")`,
  `applyLook(scene, entity)`, `paintWorld(point, worldMatrix, color, radius_m)` /
  `paintLocal` / `paintLineWorld`, then `sync()` (dirty rectangle only). Rows from
  the bottom. The RobotIK simulator uses it for painted canvases.

## Rules

- Data from the vertex to the fragment shader goes in one struct, as in
  Unity: `struct V2F { vec3 normal; vec2 uv; }; out V2F v2f;` in the vertex
  shader, the same struct with `in V2F v2f;` in the fragment shader, read as
  `v2f.uv`. No loose `out vec2 vUv;`. Vertex inputs stay plain `in`
  variables (filled by name); integer data makes the whole variable `flat`.
  A hook that adds data to the PBR shader declares its own struct
  (`struct MetalV2F {…}; out MetalV2F metal;`), never a second `V2F`.
- With a geometry shader between them, the same convention as Unity: the
  vertex shader writes `out V2G v2g;`, the geometry shader reads the corners
  of a primitive as an array, `in V2G v2g[];`, and writes `out G2F g2f;`
  (one per emitted vertex, before `EmitVertex()`); the fragment shader reads
  `in G2F g2f;` instead of `v2f`.
- SI units first: a time, an angle or a speed is a `compages::si` type
  (`Seconds`, `Radians`, `AngularVelocity`, `LinearVelocity`, `Length`…, in
  `Core/Units.hpp`), not a bare `float`/`double`, above all in a public API.
  Write values with `using namespace compages::si::literals;` (`0.5_s`,
  `90.0_deg`, `1.2_rad_per_s`, `9.81_mps_sq`). A speed times `frame.elapsed`
  gives the step (`spin * dt` is `Radians`). Trigonometry and maths on
  quantities use the units library: `units::math::sin(2.0_rad_per_s * t)`,
  `cos`, `atan2`, `hypot`, `abs`, `exp(-t / 5.0_s)`; never
  `std::sin(compages::toSeconds(t) * 2.0f)`. `toSeconds`/`toRadians`/
  `toMetres` only hand a number to code that takes floats. A
  uniform takes a time or an angle as is (`drawable["time"] = frame.total`).
  Positions, sizes and colours stay `float`/`Vector3f` (metres).
- The renderer never owns the simulation: it reads a snapshot, never mutates
  world state while drawing. World components hold asset ids, never GPU objects.
- Construction errors are stored and returned by `prepare()`; per-frame errors
  go through `gpu::reportError`.
- `Scene` must not open windows or touch input devices: input arrives in `ViewFrame`.
- Renderer tests can build `RenderSnapshot` by hand (`tests/Renderer/`).

Reference: doc/guides/ (one task each: load-a-model, light-the-scene, write-a-material, several-views, screen-effects, show-a-field…), doc/concepts/one-frame.md, doc/tutorials/02-first-scene.md.
