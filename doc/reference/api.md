# 📖 API reference

[⏫ Reference](README.md)

The entry points and the common calls of Compages, by task, on one page. Rows show the shape of a call, not complete programs: names such as `Scene`, `Routine` and `Field` are shortened. For the why, see the [concepts](../concepts/README.md); for working code, the [tutorials](../tutorials/README.md) and the [guides](../guides/README.md). The Doxygen comments of the headers in `include/Compages/` have the details.

Namespaces: `compages::gpu`, `compages::world`, `compages::renderer`; vectors, matrices, `Frame`, `Status`, `Result<T>`, grids and fields are directly in `compages` (`compages::Vector3f`).

## 📎 1. Includes

```cpp
#include <Compages/Compages.hpp>         // the whole stack
#include <Compages/GPU/GPU.hpp>
#include <Compages/Fields/HeightField.hpp> // Grid2D, Field<T>, intersect
#include <Compages/World/Entity.hpp>     // includes World
#include <Compages/Renderer/Scene/Scene.hpp>
```

---

## 🚨 2. Errors

```cpp
COMPAGES_TRY(expr);                      // returns the error of a Status / Result
COMPAGES_TRY_ASSIGN(var, expr);          // Result<T> -> T, or returns the error

COMPAGES_TRY(scene.prepare());           // first construction error
COMPAGES_TRY(drawable.prepare());

const std::string err = compages::gpu::takeFrameError();  // "" when no error
```

- Load (shader, file): the `Result` comes back at once.
- Frame (name, type): the message is kept, read at the end of the frame or by `prepare()`.

---

For a visual program, initialize a graphics context and the GPU, create resources, check `prepare()`, draw inside a `RenderPass`, then release resources before shutdown. A World-only program skips all graphics setup.

## 🟧 3. GPU

| Action | Call |
|--------|------|
| device | `compages::gpu::init(loader)` / `shutdown()` / `initialized()` |
| draw | `Drawable d; d.loadSources(vs, fs); d["name"] = …; d.draw();` |
| typed vertices | `d.setVertices<Vertex>(data)` or `BufferUsage::Dynamic` (no default); `d.editVertex<Vertex>(i)`, `d.editVertices<Vertex>(first, count)`, `d.editVertices<Vertex>()`, `d.vertexCount()` |
| GPU-made or shared vertices | `d.borrowVertices(buffer)`: no copy, the buffer must outlive `d` |
| buffers | `VertexBuffer<T>`, `IndexBuffer<T>`, `StorageBuffer<T>`; `::from(data, usage)`, `::create(count, usage)`; `BufferUsage::Static` (no CPU copy, never written), `Stream` (no CPU copy, `write()`), `Dynamic` (CPU copy, `[]`, `upload()`) |
| clear | `clear({ r, g, b })` |
| state | `d.state().depth_test = true;` `d.indices({ 0, 1, 2 });` |
| several shaders | `Pipeline::create<Vertex>(program, state)` |
| hot reload | `ShaderWatch::watch(vertex, fragment, load)`, `poll()` each frame, `reloads()` |
| storage buffer in a draw | `program.bind("Block", buffer.handle())`: any program, a vertex shader can read what a compute shader wrote |
| shader in pieces | `ShaderSource::fromText(name, glsl)` or `fromFile(path)`; `.include(name, glsl)` `.directory(path)` `.define(name, value)` `.hook(name, glsl)`; `program.load(vertex, fragment)`, `computeProgram.load(source)`; `resolve()` → text, `pieces`, `files` |
| compute | `ComputeProgram::loadSource` or `loadFile`; `dispatchItems(n)`; `barrier(Barrier::VertexAttrib)` |
| ping-pong | `PingPong<T>::from(data)`: `current()`, `next()`, `swap()`; `dispatch(program, state, { .read = "In", .write = "Out" })`; `TexturePingPong::create(desc)`: `current()` (texture), `next()` / `currentTarget()` (framebuffers), `swap()` |
| read back | `buffer.download()` after the right barrier; `texture.read()` |
| save a picture | `texture.saveFile("out.png")` (2D, R8 / RGB8 / RGBA8) |
| measure a frame | `frameStatistics()` (draws, vertices, passes, dispatches), `resetFrameStatistics()`; GPU time: `beginGpuTimer()` … `endGpuTimer()`, then `gpuMilliseconds()` (the last frame ready, read without waiting; negative before) |
| after ImGui | `forgetRenderState()` |

GLSL names are the keys of `operator[]`. `draw()` sends the changes of a `Drawable`; `upload()` does it for a `Buffer` you manage yourself.

---

## 🟦 4. World

| Action | Call |
|--------|------|
| create an entity | `world.entity("Name")` |
| child | `parent.child("Child")` |
| find by path | `world.lookup("Parent/Child")` |
| transform | `.position(x, y, z)` `.rotate(rad, axis)` `.scale(s)` `.lookAt(target)` |
| world placement | `.worldMatrix()`, `.worldPosition()` |
| component | `.set(T{})` `.add<T>(args)` `.get<T>()` `.has<T>()` `.remove<T>()` |
| system (loop) | `world.each<A, B>([](Entity e, A& a, B& b) { … })` |
| step | `compages::Frame step; step.elapsed = dt; world.update(step);` |
| pause the world, keep the camera | `frame.elapsed = 0.0_s; frame.wall_elapsed = dt;` (`ViewFrame`: routines see the first, `Orbit`, `Rig`, `Fly` the second) |
| routine | `struct X: Routine { void update(compages::si::Seconds dt) override; };` then `.add<X>(…)` |
| routine life | `onEnable` `start` `fixedUpdate` `update` `lateUpdate` `onDisable` `onDestroy`, `priority()`, `enabled(bool)` |
| fixed pace | `world.routines().fixedStep(compages::si::Seconds(1.0 / 120.0))` |
| timers (in a routine) | `after(s, fn)` `every(s, fn)` `cancel(id)` |
| messages | `world.routines().send(entity, value)`, `broadcast(value)`; `onMessage(m)`, `m.as<T>()`; `Contact` for shocks |
| step in parts | `world.step(frame)`, `world.update()`, `world.endStep()` |
| joint | `.revolute(axis, min, max)` `.joint().angle(a)` `.prismatic(axis, min, max)` `.joint().offset(o)` |
| joint value | `.joint().angle()` `.joint().offset()` (applied, inside the limits), `.joint().commandedAngle()` `.joint().commandedOffset()` (as set) |
| who writes the pose | `.set(PoseDriver{ PoseDriver::External })` — also `Kinematic`, `Animation` |
| URDF robot, headless | `compages::world::loadUrdf(world, "robot.urdf")` |
| life | `if (entity)`, `entity.enable(false)` |

---

## 🟩 5. Renderer

| Action | Call |
|--------|------|
| scene | `Scene scene(world);` or `Scene scene(world, assets)` |
| background, light | `scene.background(r, g, b)` `scene.ambient(r, g, b)` |
| camera, sun, lamp | `scene.camera()` `scene.sun(name, color, intensity)` `scene.lamp(…)` |
| shadows | `environment().shadows` (sun), `PointLight::shadows` (one lamp), `environment().shadow_resolution`, `environment().lamp_shadow_resolution` |
| primitives | `scene.box(name, look)` `sphere` `plane` `cylinder` `cone` `pyramid` |
| looks | `color(r, g, b)` `texture("a.jpg")` `unlit(r, g, b)` `normals()` `depth(near, far)` `seeThrough(look, opacity)` |
| own material | `auto pbr = shaders::pbr(); pbr.vertex().hook("deform", glsl); pbr.fragment().hook("surface", glsl);` (`"light"` for another model of light, `"main"` for an unlit picture) then `assets().createMaterial(name, { .family, .shaders, .state, .see_through, .textures = { { .sampler, .texture } }, .buffers = { { .block, .buffer } } })` → `MaterialId`, `scene.look(entity, materialId)` |
| new geometry | `scene.reshape(entity, mesh)` (in place: its copies change too) |
| show a field | `scene.show(field, FieldLook{ .colors, .low, .high, .relief, .lit }, name)` → entity; `scene.show(entity, field, look)` for new values; `ColorMap::Viridis` `Inferno` `Coolwarm` `Terrain` `Grey` |
| field on the GPU | `scene.show(grid, storageBuffer, look, name)`, `scene.show(entity, storageBuffer, look)`: a `StorageBuffer<float>` a compute shader writes |
| read a field | `scene.probe(field, look, pixel)` → `std::optional<FieldProbe>{ .point, .value }` |
| show vectors | `scene.show(Field<Vector2f>, ArrowLook{ .colors, .low, .high, .scale, .every }, name)`, `scene.show(entity, field, look)` |
| curves over time | `Chart::create(scene, desc, name)` with `desc.scale` set to `ChartScale::Fixed` or `ChartScale::FollowValues`, `series(color)`, `add(series, time, value)`, `draw()`, `pixelOf(time, value)`, `applyLook(scene, entity)` |
| show a volume | `scene.show(Field<float, Grid3D>, look, name)`, `scene.show(entity, volume, look)` |
| material buffers | `material.buffers = { { .block, .buffer } }`: storage buffers bound by the Renderer |
| helpers | `scene.grid(size, divisions)` `scene.axes(length)` `scene.showBounds(id, true)`; a mesh as a help: `MeshRenderer::flags = RenderFlags::Helper`; a sensor camera that sees none: `world::Camera::sees_helpers = false` |
| colours | sRGB in, linear inside: `srgbToLinear(c)`; `environment().tone_mapping` (`Aces`, `Reinhard`, `None`), `environment().exposure` |
| large scenes | `PassDesc::reversed_depth`, `CameraFrame::reversed_projection`; `Orbit` controller: `zoom_step`, `boost` (Shift), `sharpness`, `min_distance`, `max_distance` |
| map camera (`Rig`) | left drag pans, wheel zooms toward the mouse; `RigController::flyTo(target, distance)`, `flying()`, `flight_pace` |
| picture without a screen | `scene.renderToFile("out.png", width, height)`; program `Compages-headless` |
| OpenGL context with no window | `compages::HeadlessContext::open()`, then `gpu::init(&HeadlessContext::load)` (library `Compages-Headless`) |
| picture of its own (a robot camera) | `RenderTarget::create(w, h)`, `scene.render(camera, target)`, `target.color()` `depth()` `readColor()` (RGBA8, top row first) `readDepth(lens)` (metres, top row first, +inf = nothing) `saveColor(path)` `resize(w, h)` `time()` (of the World it shows) |
| lens of a calibrated camera | `compages::world::calibrate(lens, { .fx, .fy, .cx, .cy, .width, .height })` (pixels, top left origin; sets `fov` and `lens_shift`; square pixels only, no distortion) |
| read a camera without waiting | after the render `target.readLater({ .color = true, .depth = true })`; next frame `if (target.readReady()) target.takeRead()` → `Capture{ time, color, depth }`; any texture: `texture.readLater()` → `gpu::PendingRead` `ready()` `take()` |
| filters, bloom of one view | `camera.set(ScreenEffects{ .filters = { { .name, .glsl = "vec3 effect(vec2 uv) {...}", .values = { { "uniform", v } } } }, .bloom = { .enabled, .threshold, .strength } })`; GLSL reads `screenColor` `screenDepth` `screenSize`; a screen colour of your own: `screen(toSrgb(c))` |
| file | `scene.loadFile("model.glb")` → `Result<Entity>` |
| prefab | `assets().loadFile(path)` then `scene.instantiate(id)` |
| frame | `scene.draw(ViewFrame)`, or `update` then `render()` / `render(cameraId)` |
| fit the view | `scene.frameAll()` |
| animation | `scene.play(model, "Walk")` `scene.clips(model)` |
| sky | `scene.skybox({ "+x", "-x", "+y", "-y", "+z", "-z" })` |
| picking | `scene.pick({ x, y })` |
| debug lines | `scene.debug()`: `.line(a, b, c)` `.ray(o, d, l, c)` `.box(bounds, matrix, c)` `.polyline(points, c)` `.arrow(from, to, c)` `.frame(pose, length)` `.frustum(pose, lens, aspect, length, c)` |
| text on the picture | `scene.font(file)`, `entity.set(Label{ .text, .color, .pixels, .hide_closer_than })`, `entity.set(Tooltip{ .title, .text, .color, .accent, .background, .opacity, .pixels })`, `camera.set(Caption{ .text, .color, .pixels, .at })` |
| live texture | `CanvasTexture`: `create`, `setMapping`, `bind`, `paintWorld`, `paintPixelLine`, `fillPixels`, `sync`, `applyLook` |

---

## 🗺 6. Fields

Structured data, directly in `compages` (`#include <Compages/Fields/HeightField.hpp>` gives the three).

| Action | Call |
|--------|------|
| a grid | `Grid2D{ .columns, .rows, .origin, .spacing }`; `count()` `index(i, j)` `column(n)` `row(n)` `contains(i, j)` `point(i, j)` `coordinates(place)` `nearest(place)` |
| neighbours | `for (std::size_t n : grid.neighbours4(index))`, `neighbours8(index)` |
| a grid fitted round a shape | `CurvilinearGrid::create(columns, rows, place(i, j))` → `Result`; same calls as `Grid2D`, plus `low()` `high()`; `Field<T, CurvilinearGrid>` (`<Compages/Fields/CurvilinearGrid.hpp>`) |
| the cell of a place | `grid.locate(place)` → `GridCell{ i, j, s, t, inside }`; any grid of the plane is a `PlanarGrid` (`<Compages/Fields/PlanarGrid.hpp>`) |
| values on it (a grid with a size of zero or less holds no point: `count()` is 0, `valid()` false) | `Field<T> field(grid, value)`; `field(i, j)` `field[index]` `values()` (contiguous span) `fill(value)` `size()` `grid()` |
| between the points | `field.sample(place)`: bilinear in its cell, the edge value outside (any `PlanarGrid`) |
| a volume | `Grid3D{ .columns, .rows, .layers, .origin, .spacing }`, `index(i, j, k)`, `layer(n)`, `neighbours6(index)`; `Field<T, Grid3D> volume(grid, value)`, `volume(i, j, k)`, `volume.sample(Vector3f)` (trilinear) |
| streamline | `compages::streamline(field, start, step, steps)` → points (`<Compages/Fields/Streamline.hpp>`) |
| isosurface | `compages::isosurface(volume, level)` → `Surface{ positions, normals }`; `renderer::meshOf(surface)` → `MeshAsset` |
| a cut of a volume | `compages::slice(volume, Axis::X / Y / Z, layer)` → `Field<T>` (`<Compages/Fields/Slice.hpp>`) |
| field to the GPU | `gpu::Texture::fromField(field)`: 2D or 3D, R32F / RG32F / RGBA32F; the points are in the order of the texels |
| ray and ground | `intersect(ray, heights)` → `std::optional<Vector3f>`; `intersectHeight(ray, height(x, z), from, to, step)` → distance along the ray |

---

## ⏰ 7. Time

| Type | Holds |
|------|-------|
| `compages::Frame` | `elapsed`, `total`, `width`, `height` |
| `compages::world::ViewFrame` | a `Frame` plus keyboard and mouse |

| Maths (`Core/`) | Call |
|-----------------|------|
| position in metres, in double | `compages::Vector3m` (`Vector3g::UNIT_Y * 6371.0_km`); `vector::norm`, `distance`, `dot` keep the units, `normalize` gives a `Vector3g` |
| into the World (float metres) | `compages::toMetres(1.0_au)`, `toMetres(vector3m)`; not `.to<float>()`, which keeps the unit of the type |
| turn a direction onto another | `Quatf::fromTo(from, to)`, `upTo(direction)` (+Y onto it), `upFrom(direction)` (it onto +Y) |
| double rotation to float | `compages::Quatf(quatd)` |

---

## 📷 8. Gallery

```sh
./build/linux-gcc-debug/bin/Compages-examples
./build/linux-gcc-debug/bin/Compages-examples 06a_FirstScene
./build/linux-gcc-debug/bin/Compages-examples --check
./build/linux-gcc-debug/bin/Compages-examples --check 06a_FirstScene
./build/linux-gcc-debug/bin/Compages-examples --check --shots external/Compages-data/doc/examples
```

Keys: [the window](gallery.md#-1-the-window).

Next: [the gallery](gallery.md).
