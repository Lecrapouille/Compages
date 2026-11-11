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
- Own material: `auto pbr = shaders::pbr();` (`ShaderPair`, `Procedural/ShaderLib.hpp`; also `lit()`,
  `unlit()`, `depth()`, `normals()`, `shadow()`, `lampShadow()`), hooks `pbr.vertex().hook("deform",
  "void deform(inout vec4 world, inout vec3 normal) {...}")` and `pbr.fragment().hook("main", ...)`
  (writes `oColor`; `shade`, `applyFog`, `present`... included; library pieces `<compages/*.glsl>`).
  `Material::create(name, { .family, .shaders, .state, .see_through })`, `material.textures =
  { { .sampler, .texture } }` (units 2-6, bound by the Renderer), `assets().addMaterial`, then
  `scene.look(entity, materialId)`; `scene.reshape(entity, mesh)` replaces a mesh in place. The
  Renderer sets family uniforms only if the shader reads them (`src/Renderer/Render/Offer.hpp`).
  Demos: `10e_Archipelago` (Sea.cpp), `10c_SweatAndDecals`.
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
  (`Fields/Slice.hpp`) for cuts; demos `11e_Smoke`, `11f_Volume`.
- Structured data (`Fields/`, namespace `compages`, includes only Core): `Grid2D`, `Field<T>`
  (`sample` bilinear), `intersect(ray, Field<float>)`, `intersectHeight(ray, h(x,z), from, to, step)`.
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
- `scene.renderToFile("out.png", w, h)` (after update); `Compages-headless` makes an EGL
  context with no surface; `gpu::Texture::saveFile(path)` writes a 2D R8/RGB8/RGBA8 PNG.
- Files: `scene.loadFile(path)` → `Result<Entity>` (glTF/GLB, prefab, STL, URDF
  with joints); `assets().loadFile(path)` → `PrefabId`, `instantiate(id)`;
  `frameAll()`; `play(model, "Walk")`, `clips(model)`; `copy(entity)` shares
  mesh and look. Assets from `external/Compages-data/` or `COMPAGES_DATA_PATH`.
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

- The renderer never owns the simulation: it reads a snapshot, never mutates
  world state while drawing. World components hold asset ids, never GPU objects.
- Construction errors are stored and returned by `prepare()`; per-frame errors
  go through `gpu::reportError`.
- `Scene` must not open windows or touch input devices: input arrives in `ViewFrame`.
- Renderer tests can build `RenderSnapshot` by hand (`tests/Renderer/`).

Reference: doc/06-Renderer-Layer.md, doc/03-Tutorial.md §3.
