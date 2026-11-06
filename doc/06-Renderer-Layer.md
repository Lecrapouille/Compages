# 🟩 6. The Renderer layer

The third layer turns a `world::World` into an image: camera, lights, materials, meshes, animations, sky. In this page you learn to build a scene, what happens in one frame, how files and prefabs are loaded, how to paint on a texture while the simulation runs, and how one world can be seen by several cameras.

| | |
|---|---|
| **Entry point** | `compages::renderer::Scene` |
| **Headers** | `<Compages/Compages.hpp>` (the whole stack) or `<Compages/Renderer/Scene/Scene.hpp>` |
| **Code** | `include/Compages/Renderer/`, `src/Renderer/` |
| **Behind it** | an asset catalog, a snapshot of the world, then `gpu::` calls |

The ten-line cube is commented in [chapter 3 §3](03-Tutorial.md#-3-a-picture-of-the-world).

![Three.js-like: a cube, an orbit, a sun](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06a_FirstScene.png)

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
compages::renderer::unlit(r, g, b);      // a flat color no light changes: a bulb, a marker
compages::renderer::normals();           // normals shown as colors
compages::renderer::depth(near, far);    // distance, black to white
compages::renderer::seeThrough(look, 0.35f); // any look, see-through (1 solid, 0 invisible)
```

`06f_GeometryShowcase` lines the primitives up with these looks; `06e_TexturedSpheres` shows a color, a photo, a tinted photo, and a glass ball in front of them.

**See-through objects.** A look with an opacity below one is drawn after all the solid objects, sorted from the farthest to the nearest, and blended over what is already drawn. It does not write the depth, so a second glass behind the first still shows. The sorting is per object (by the centre of its box): two see-through objects that cross each other can still be drawn in the wrong order.

### Helpers

Lines that help to read a scene, one line each. They are entities or components like any other: move them, hang them from a parent, remove them.

```cpp
scene.grid(10.0f, 20u);               // a grid on the ground, 20 cells across
scene.axes(0.5f).parent(arm);         // X red, Y green, Z blue, turning with the arm
scene.showBounds(cube.id(), true);    // the box around the mesh of the cube
```

`06b_Transforms` uses the three: the axes show the turn of the box, the bounds the size of the cone. `06c_Hierarchy` shows the pivot of a car, `06i_CameraPick` the bounds of the selection, and `08d_RobotArm` the frame of every joint of a robot (Try it hides them).

Assets are searched in `external/Compages-data/`, or in `COMPAGES_DATA_PATH` (`examples/Common/DataPath.hpp`).

---

## 🔄 2. One frame in detail

```text
Scene::draw(ViewFrame)
  update
    World::update(frame)     routines
    AnimationSystem          clips -> local transforms
    World::update()          joints, world matrices
    skinning                 SkinInstance
  render(active camera)
    SceneExtractor           World -> RenderSnapshot
    Renderer                 snapshot + catalog -> gpu::
```

- Draws are **sorted** by material, then by mesh, to change pipelines as rarely as possible.
- **Culling** happens during extraction, before the snapshot: what the camera cannot see is not drawn. `09b_ManyCubes` (1,728 cubes, one mesh, five looks) shows the number of draw calls change while the camera moves.
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

`06j_SplitViews`: a perspective orbit on the left, an orthographic top view on the right. A camera whose viewport covers part of the image clears and draws only this part.

---

## 📂 3. Files, the catalog and prefabs

| Call | Effect |
|------|--------|
| `scene.loadFile("Duck.glb")` | loads and places in the world; returns the root entity (a `Result`) |
| `scene.assets().loadFile(path)` | returns a `PrefabId`, does not touch the world |
| `scene.instantiate(prefab)` | adds one more copy; joints are remapped |
| `scene.frameAll()` | places camera and sun to see everything; returns the center, for `Orbit` |
| `scene.play(model, "Walk")` | plays an animation of the file |
| `scene.clips(model)` | the clip names, in file order |
| `scene.copy(entity)` | draws another entity with the same mesh and look (cheap duplicates) |

`load` reads **glTF / GLB**, saved **prefabs**, **STL** meshes and **URDF** robots. A URDF comes with its joints (`08d_RobotArm`): the World builds the chain, the renderer adds the visuals of the links (`Renderer/Import/UrdfLoader.hpp`).

- `08a_GltfModel` loads `Duck.glb` and frames the camera.
- `08b_GltfAnimation` plays the clips of `Soldier.glb` (keys 1, 2, 3).
- `08c_PrefabAndSave` places a prefab three times, moves each copy alone, then writes `/tmp/compages_prefab_scene.json` with asset names.

---

## 🌅 4. Environment and interaction

```cpp
scene.ambient(r, g, b);
scene.skybox({ "+x.jpg", "-x.jpg", "+y.jpg", "-y.jpg", "+z.jpg", "-z.jpg" });
scene.lamp("Lamp", color, intensity, range);
scene.activeCamera(id);
```

- The sky (`06h_Skybox`) turns with the camera but never comes closer.
- `scene.pick({ x, y })` reads the result of the last `render`, pixel counted from the bottom left, like the mouse coordinates (`06i_CameraPick`).
- `scene.debug()` queues debug lines for the next render, then forgets them (`10a_MvpDemo`).
- Ready-made controls, as routines: `Orbit` (right button, wheel; the zoom is logarithmic and glides, Shift is faster), `Fly` (right button to look, WASD, Q, E) and `Rig`. `32b` switches between `Orbit` and `Fly`. `FPSController` keeps you on the ground; it is applied directly, not as a routine.
- `Rig` is a strategy-game camera, looking at a point of the ground. The left button drags the ground (the grabbed point stays under the mouse), the right or middle button turns, the wheel zooms; W A S D or the arrows move, Q and E turn, R and F zoom, Shift runs. The camera glides to the wanted pose; each frame covers the part `1 - exp(-sharpness * dt)` of the way, so the glide is the same at any frame rate and never overshoots (a `Lerp(a, b, dt * speed)` does both wrong). The logic is in `RigController`, tested without a window.

  ```cpp
  scene.camera().position(1.6f, 1.2f, 1.6f).add<compages::world::Rig>(compages::Vector3f(0.35f, 0.0f, 0.0f));
  ```

**Colours.** Every colour you give a `Scene` (looks, lights, ambient, fog) is
**sRGB**, the colour of a colour picker or a web page. The renderer turns it
into **linear** light, where adding two lights gives twice the light, and the
shaders compute in linear. At the end, a **tone curve** turns the light, which
has no upper limit, into a screen colour between 0 and 1, and the result is
encoded back to sRGB for the screen. The background is written as it is.

```cpp
scene.environment().tone_mapping = compages::renderer::ToneMapping::Aces;  // the default
scene.environment().exposure = 1.5f;   // brighter, like opening the lens
```

`ToneMapping::Aces` (the film curve) keeps contrast and keeps bright colours
coloured; `Reinhard` is softer; `None` clamps, so what is too bright turns flat
white. `06d_Lights` has both settings in its Try it panel. Pictures given with
`texture()` are read as sRGB too; glTF colour factors are already linear, as
the format says.

**Light and shadow.** The lit and PBR materials share one shading: the sun,
a hemisphere ambient (faces turned up get more sky than faces turned down),
a soft fill light from the eye and a small highlight; then the tone curve.
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
`10a_MvpDemo` (one lamp) and `10b_DoomLike` (the torches) use them.

### Draw into a file, without a screen

```cpp
scene.update(frame);
COMPAGES_TRY(scene.renderToFile("scene.png", 1280u, 720u));
```

`renderToFile` draws what the active camera sees into an off-screen picture of
that size and writes it as PNG. It only needs an OpenGL context, not a window:
the program `Compages-headless` (`examples/Headless/main.cpp`) makes one with
EGL and no surface, so the same code runs on a server, in a container or in a
CI job, on a GPU or with Mesa llvmpipe (no GPU):

```sh
./build/linux-gcc-debug/bin/Compages-headless scene.png 1280 720
```

The library still never makes the context: the program does, then calls
`gpu::init(eglGetProcAddress)`. Like `render`, `renderToFile` draws the world
as the last `update` left it. It waits for the GPU: use it for a report, a
thumbnail, or a test that compares pictures, not for every frame.

### Very large scenes: from a rocket to the Moon

Three things let a metre and 384 000 km live in one picture (`10d_EarthMoon`):

- **Logarithmic zoom.** Each notch of the wheel moves the `Orbit` camera the same *part* of the way (`zoom_step`, about 11 %), and the move glides. Shift makes a notch six (`boost`). Set `controller.min_distance` and `controller.max_distance` for the span of your scene.
- **Reversed depth.** The depth buffer gives each pixel a distance, to know which face hides the other. Stored the usual way, nearly all its precision sits near the camera: with a near plane of 1 m, two faces 10 km away must be 6 m apart, two faces at the Moon 9 million km apart, or they flicker (z-fighting). The `Scene` stores it **reversed** (1 near, 0 far): with a `Depth32F` target, as the gallery and `renderToFile` have, the precision stays the same in proportion at every distance (about 1 mm at 10 km, 40 m at the Moon). A near plane of half a metre and a far plane of two million kilometres work together. Nothing changes in the materials: the renderer gives them `CameraFrame::reversed_projection` as `projection`. Only a drawable drawn over the picture of a `Scene`, testing its depth, must follow: it opens a pass with `reversed_depth = true` and uses `lastCamera().reversed_projection` (the particles of `10b_DoomLike`). Why it works and what the backend does: [chapter 4, Reversed depth](04-GPU-Layer.md#reversed-depth).
- **Floating origin.** A `float` has 24 bits: near the Moon, 3.8 × 10⁸ m from the origin, two neighbouring floats are 32 m apart, and the rocket would shake by 32 m. Changing the unit does not help (in km, the step is still 32 m). Keep the simulation in `double`, outside Compages (as in `07e_ExternalPhysics`), and each frame write into the `World` the positions **relative to the rocket**, which stay small. Far things get the large numbers, where 32 m is less than a pixel.

```cpp
const Point origin = rocket;                 // double, from the simulation
m_earth.position(toFloat(EARTH - origin));
m_moon.position(toFloat(MOON - origin));
m_rocket.position(0.0f, 0.0f, 0.0f);
```

A sphere wears an **equirectangular** picture (a planet, a 360° sky) the way a globe does: the top row on the north pole (+Y), east to the right seen from outside.

---

## 🎨 5. Paint on a texture: `CanvasTexture`

For a texture you change while the simulation runs (a robot that paints on a canvas, a height-field editor, a CPU simulation shown on a quad), use `compages::renderer::CanvasTexture`: an RGBA8 image in CPU memory, registered as a texture asset, sent to the GPU in part only.

📁 header `<Compages/Renderer/Procedural/CanvasTexture.hpp>` · demo `06l_CanvasPaint`

```cpp
compages::renderer::CanvasTexture canvas;
COMPAGES_TRY(canvas.create(512u, 512u));  // white by default

// Matches the mesh: a 1x1 plane in local space, before scale()
canvas.setMapping(compages::AABB::fromCorners(
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

`10b_DoomLike` shows the three layers together: textured walls, fog, glTF soldiers and robots, an animated gun, particles, impacts, barrels. Click in the window to capture the mouse, Escape to release it. WASD or arrows to move, Shift to run, Q and E to turn, left click to shoot, R to reload.

![Doom-like](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10b_DoomLike.png)

---

## 🧭 7. Demos of this layer

The folders `06_Scene/` (compose a scene) and `08_Assets/` (load files) follow this chapter.

| Demo | Idea |
|------|------|
| `06a_FirstScene` | the minimum: shape, camera, light, `draw` |
| `06b_Transforms`, `06c_Hierarchy`, `06d_Lights` | place shapes, hang them from a parent, light them |
| `06f_GeometryShowcase` | primitives and debug looks |
| `08a_GltfModel` | `load`, `frameAll` |
| `08b_GltfAnimation` | animation clips |
| `06j_SplitViews` | one world, two cameras |
| `09b_ManyCubes` | sorting and culling |
| `06l_CanvasPaint` | painting on a live texture |
| `10b_DoomLike` | the three layers together |
| `10c_SweatAndDecals` | a custom `Material` on a glTF mesh, GPU passes in its UV map |

Next: [07-Architecture.md](07-Architecture.md), or the full list of demos in [09-Examples.md](09-Examples.md).
