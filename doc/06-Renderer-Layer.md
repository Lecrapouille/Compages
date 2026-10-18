# 🟩 6. The Renderer layer

The third layer turns a `world::World` into an image: camera, lights, materials, meshes, animations, sky. In this page you learn to build a scene, what happens in one frame, how files and prefabs are loaded, how to paint on a texture while the simulation runs, and how one world can be seen by several cameras.

| | |
|---|---|
| **Entry point** | `compages::renderer::Scene` |
| **Headers** | `<Compages/Compages.hpp>` (the whole stack) or `<Compages/Renderer/Scene.hpp>` |
| **Code** | `include/Compages/Renderer/`, `src/Renderer/` |
| **Behind it** | an asset catalog, a snapshot of the world, then `gpu::` calls |

The ten-line cube is commented in [chapter 3 §3](03-Tutorial.md#-3-a-picture-of-the-world).

![Three.js-like: a cube, an orbit, a sun](images/50_ThreeJsLike.png)

---

## 🎬 1. Build a scene

```cpp
#include <Compages/Compages.hpp>

compages::world::World world;
compages::renderer::Scene scene(world);

scene.background(0.04f, 0.05f, 0.08f);
scene.camera().position(0.0f, 1.0f, 3.0f).add<compages::world::Orbit>();
scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
compages::world::Entity cube =
    scene.box("Cube", compages::renderer::color(0.9f, 0.18f, 0.12f));

COMPAGES_TRY(scene.prepare());
// every frame:
scene.draw(frame);
```

`box`, `sphere`, `plane`, `cylinder`, `cone` and `pyramid` each return a `world::Entity`. Chain `.position()`, `.rotate()`, `.add<Spin>()` as on any entity. The mesh and the material are renderer components, invisible unless you include this layer.

`prepare()` uploads what was built to the GPU and returns the first construction error (missing image, internal shader…). Construction errors do not stop the program: they wait for this call.

### Looks

```cpp
compages::renderer::color(r, g, b);      // lit color
compages::renderer::texture("file.jpg"); // lit, tinted by the color (white by default)
compages::renderer::normals();           // normals shown as colors
compages::renderer::depth(near, far);    // distance, black to white
```

`33c_GeometryShowcase` lines the primitives up with these four looks; `33a_TexturedSpheres` shows a color, a photo, and a tinted photo.

Assets are searched in `external/Compages-data/`, or in `COMPAGES_DATA_PATH` (`examples/Common/DataPath.hpp`).

---

## 🔄 2. One frame in detail

```text
Scene::draw(ViewFrame)
  update
    World::update(frame)     behaviors
    AnimationSystem          clips -> local transforms
    World::update()          joints, world matrices
    skinning                 SkinInstance
  render(active camera)
    SceneExtractor           World -> RenderSnapshot
    Renderer                 snapshot + catalog -> gpu::
```

- Draws are **sorted** by material, then by mesh, to change pipelines as rarely as possible.
- **Culling** happens during extraction, before the snapshot: what the camera cannot see is not drawn. `20b_ManyCubes` (1,728 cubes, one mesh, five looks) shows the number of draw calls change while the camera moves.
- `RenderSnapshot` is a plain value: the renderer never reads the `World` while drawing. The renderer tests build snapshots by hand (`Renderer/Render/SceneExtractor.hpp`, `RenderSnapshot.hpp`, `Renderer.hpp`).

### Several views of one world

Call `update` once, then `render` for each view:

```cpp
compages::renderer::Scene map(world, scene.assets());   // same world, same catalog
compages::world::EntityId top = map.camera().id();

scene.update(frame);
scene.render();          // the active camera
map.render(top);         // a minimap
```

`32a_SplitViews`: a perspective orbit on the left, an orthographic top view on the right. A camera whose viewport covers part of the image clears and draws only this part.

---

## 📂 3. Files, the catalog and prefabs

| Call | Effect |
|------|--------|
| `scene.load("Duck.glb")` | loads and places in the world; returns the root entity (a `Result`) |
| `scene.assets().load(path)` | returns a `PrefabId`, does not touch the world |
| `scene.instantiate(prefab)` | adds one more copy; joints are remapped |
| `scene.frameAll()` | places camera and sun to see everything; returns the center, for `Orbit` |
| `scene.play(model, "Walk")` | plays an animation of the file |
| `scene.clips(model)` | the clip names, in file order |
| `scene.copy(entity)` | draws another entity with the same mesh and look (cheap duplicates) |

`load` reads **glTF / GLB**, saved **prefabs**, **STL** meshes and **URDF** robots. A URDF comes with its joints (`38_RobotArm`).

- `34_GltfModel` loads `Duck.glb` and frames the camera.
- `36b_GltfAnimation` plays the clips of `Soldier.glb` (keys 1, 2, 3).
- `35_PrefabAndSave` places a prefab three times, moves each copy alone, then writes `/tmp/compages_prefab_scene.json` with asset names.

---

## 🌅 4. Environment and interaction

```cpp
scene.ambient(r, g, b);
scene.skybox({ "+x.jpg", "-x.jpg", "+y.jpg", "-y.jpg", "+z.jpg", "-z.jpg" });
scene.lamp("Lamp", color, intensity, range);
scene.activeCamera(id);
```

- The sky (`37_Skybox`) turns with the camera but never comes closer.
- `scene.pick({ x, y })` reads the result of the last `render`, pixel counted from the bottom left, like the mouse coordinates (`32b_CameraPick`).
- `scene.debug()` queues debug lines for the next render, then forgets them (`52_MvpDemo`).
- Ready-made controls, as behaviors: `Orbit` (right button, wheel), `Fly` (right button to look, WASD, Q, E) and `Rig`. `32b` switches between `Orbit` and `Fly`. `FPSController` keeps you on the ground; it is applied directly, not as a behavior.
- `Rig` is a strategy-game camera, looking at a point of the ground. The left button drags the ground (the grabbed point stays under the mouse), the right or middle button turns, the wheel zooms; W A S D or the arrows move, Q and E turn, R and F zoom, Shift runs. The camera glides to the wanted pose; each frame covers the part `1 - exp(-sharpness * dt)` of the way, so the glide is the same at any frame rate and never overshoots (a `Lerp(a, b, dt * speed)` does both wrong). The logic is in `RigController`, tested without a window.

  ```cpp
  scene.camera().position(1.6f, 1.2f, 1.6f).add<compages::world::Rig>(compages::core::Vector3f(0.35f, 0.0f, 0.0f));
  ```

**Light and shadow.** The lit and PBR materials share one shading: the sun,
a hemisphere ambient (faces turned up get more sky than faces turned down),
a soft fill light from the eye, a small highlight, and a soft tone curve.
The sun casts shadows: before drawing, the renderer draws the depth of the
scene seen from the sun into a picture (`Environment::shadow_resolution`,
2048 by default), fitted to the objects that cast; the materials read it
with a 3 × 3 filter, so the edges are soft. A floor should not cast
(`RenderFlags::NoShadowCaster` on its `MeshRenderer`): it would make the
fitted box huge and the shadows blurred. `Environment::shadows = false`
turns them off.

A lamp (point light) casts shadows when its `PointLight::shadows` is true
(off by default):

```cpp
scene.lamp("Torch", { 1.0f, 0.55f, 0.22f }, 3.0f, 11.0f)
    .get<compages::world::PointLight>().shadows = true;
```

A lamp shines in every direction, so one picture is not enough: the renderer
draws the distance to the lamp six times, as six cameras of 90 degrees looking
along +X, −X, +Y, −Y, +Z and −Z (the six faces of a cube). The six faces of
the four lamps the shaders receive (the nearest to the camera) sit in one
depth picture, an atlas of 6 × 4 tiles of `Environment::lamp_shadow_resolution`
texels (512 by default, zero turns them off). The material takes the face the
point is in, reads the distance of the nearest surface, and compares it with
its own, with the same 3 × 3 filter as the sun. Only what is within the range
of the lamp is drawn into its faces. The cost is six more draws of the
nearby scene per lamp: turn it on for the lamps that matter. Something that
holds or surrounds a lamp (a torch bracket, a glowing flame) must not cast
(`RenderFlags::NoShadowCaster`), or it hides the lamp from everything.
`52_MvpDemo` (one lamp) and `53_DoomLike` (the torches) use them.

---

## 🎨 5. Paint on a texture: `CanvasTexture`

For a texture you change while the simulation runs (a robot that paints on a canvas, a height-field editor, a CPU simulation shown on a quad), use `compages::renderer::CanvasTexture`: an RGBA8 image in CPU memory, registered as a texture asset, sent to the GPU in part only.

📁 header `<Compages/Renderer/Assets/CanvasTexture.hpp>` · demo `39a_CanvasPaint`

```cpp
compages::renderer::CanvasTexture canvas;
COMPAGES_TRY(canvas.create(512u, 512u));  // white by default

// Matches the mesh: a 1x1 plane in local space, before scale()
canvas.setMapping(compages::core::AABB::fromCorners(
    { -0.5f, -0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }));

COMPAGES_TRY(canvas.bind(scene.assets(), "my_canvas"));
canvas.applyLook(scene, easel_entity);   // material with this texture as base color

// In the loop:
canvas.paintWorld(hit.point, easel.worldMatrix(), color, radius_metres);
COMPAGES_TRY(canvas.sync());             // sends the dirty region (or everything)
scene.draw(frame);
```

| Step | Role |
|------|------|
| `create` | allocates the pixels in CPU memory |
| `setMapping` | local mesh coordinates → texture coordinates (box, plus optional U/V flips) |
| `bind` | creates or updates the texture asset in the catalog |
| `paintLocal` / `paintWorld` / `paintLineWorld` | a round stamp, or a line of stamps |
| `sync` | `Texture::write`, partial when possible |
| `applyLook` | shortcut for `scene.look(entity, texture_id)` |

Coordinates:

- Pixels are stored **row by row from the bottom**, like OpenGL and `Texture::write`.
- `paintWorld` turns the world point into a local one with the **inverse** of the world matrix of the canvas.
- The brush radius is in **world units**; it becomes pixels from the local width in X and the texture width.
- The image is mirrored or upside down? Change `setMapping(..., flip_u, flip_v)`.

`scene.look(entity, texture_asset_id)` creates a new material instance whose base color reads your live texture, without any file on disk.

Performance:

- After local stamps, `sync()` sends only the **dirty rectangle**; `clear()` marks the whole texture.
- The texture uses **nearest** filtering, with no mipmaps to rebuild each frame.
- For very large canvases (4K and more), call `sync()` less often, or group the strokes.

---

## 🎮 6. The game

`53_DoomLike` shows the three layers together: textured walls, fog, glTF soldiers and robots, an animated gun, particles, impacts, barrels. Click in the window to capture the mouse, Escape to release it. WASD or arrows to move, Shift to run, Q and E to turn, left click to shoot, R to reload.

![Doom-like](images/53_DoomLike.png)

---

## 🧭 7. Demos of this layer

| Demo | Idea |
|------|------|
| `50_ThreeJsLike` | the minimum: shape, camera, light, `draw` |
| `33c_GeometryShowcase` | primitives and debug looks |
| `34_GltfModel` | `load`, `frameAll` |
| `36b_GltfAnimation` | animation clips |
| `32a_SplitViews` | one world, two cameras |
| `20b_ManyCubes` | sorting and culling |
| `39a_CanvasPaint` | painting on a live texture |
| `53_DoomLike` | the three layers together |
| `54_SweatAndDecals` | a custom `Material` on a glTF mesh, GPU passes in its UV map |

Next: [07-Architecture.md](07-Architecture.md), or the full list of demos in [09-Examples.md](09-Examples.md).
