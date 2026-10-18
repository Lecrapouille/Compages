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
  `color(r,g,b)`, `texture("f.jpg")`, `normals()`, `depth(near, far)`.
- Files: `scene.load(path)` → `Result<Entity>` (glTF/GLB, prefab, STL, URDF
  with joints); `assets().load(path)` → `PrefabId`, `instantiate(id)`;
  `frameAll()`; `play(model, "Walk")`, `clips(model)`; `copy(entity)` shares
  mesh and look. Assets from `external/Compages-data/` or `COMPAGES_DATA_PATH`.
- Environment: `ambient`, `skybox({6 faces})`, `lamp(name, color, intensity, range)`,
  `activeCamera(id)`, `pick({x, y})` (bottom-left origin, last render), `debug()`.
- Several views: `Scene map(world, scene.assets());` then `scene.update(frame);
  scene.render(); map.render(map.camera().id());`.
- Frame order: `World::update(frame)` (behaviors) → `AnimationSystem` →
  `World::update()` (joints, matrices) → skinning → `SceneExtractor` →
  `RenderSnapshot` (sorted by material then mesh, culled) → `Renderer`.
- Live texture (`Renderer/Assets/CanvasTexture.hpp`, demo `39a_CanvasPaint`):
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
