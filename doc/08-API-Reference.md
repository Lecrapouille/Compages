# 📖 8. API reference

The entry points and the common calls of the three layers on one page. For the context, see [4. GPU](04-GPU-Layer.md), [5. World](05-World-Layer.md), [6. Renderer](06-Renderer-Layer.md); for a walkthrough, [3. Tutorial](03-Tutorial.md).

Namespaces: `compages::gpu`, `compages::world`, `compages::renderer`; vectors, matrices, `Frame`, `Status` and `Result<T>` are directly in `compages` (`compages::Vector3f`).

---

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
| shader in pieces | `ShaderSource::fromText(name, glsl)` or `fromFile(path)`; `.include(name, glsl)` `.directory(path)` `.define(name, value)` `.hook(name, glsl)`; `program.load(vertex, fragment)`, `computeProgram.load(source)`; `resolve()` → text, `pieces`, `files` |
| compute | `ComputeProgram::loadSource` or `loadFile`; `dispatchItems(n)`; `barrier(Barrier::VertexAttrib)` |
| ping-pong | `PingPong<T>::from(data)`: `current()`, `next()`, `swap()`; `dispatch(program, state, { .read = "In", .write = "Out" })`; `TexturePingPong::create(desc)`: `current()` (texture), `next()` / `currentTarget()` (framebuffers), `swap()` |
| read back | `buffer.download()` after the right barrier; `texture.read()` |
| save a picture | `texture.saveFile("out.png")` (2D, R8 / RGB8 / RGBA8) |
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
| routine | `struct X: Routine { void update(float dt) override; };` then `.add<X>(…)` |
| routine life | `onEnable` `start` `fixedUpdate` `update` `lateUpdate` `onDisable` `onDestroy`, `priority()`, `enabled(bool)` |
| fixed pace | `world.routines().fixedStep(1.0f / 120.0f)` |
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
| own material | `auto pbr = shaders::pbr(); pbr.vertex().hook("deform", glsl); pbr.fragment().hook("main", glsl);` then `Material::create(name, { .family, .shaders, .state, .see_through })`, `material.textures = { { .sampler, .texture } }`, `assets().addMaterial(name, material)`, `scene.look(entity, materialId)` |
| new geometry | `scene.reshape(entity, mesh)` (in place: its copies change too) |
| helpers | `scene.grid(size, divisions)` `scene.axes(length)` `scene.showBounds(id, true)` |
| colours | sRGB in, linear inside: `srgbToLinear(c)`; `environment().tone_mapping` (`Aces`, `Reinhard`, `None`), `environment().exposure` |
| large scenes | `PassDesc::reversed_depth`, `CameraFrame::reversed_projection`; `Orbit` controller: `zoom_step`, `boost` (Shift), `sharpness`, `min_distance`, `max_distance` |
| map camera (`Rig`) | left drag pans, wheel zooms toward the mouse; `RigController::flyTo(target, distance)`, `flying()`, `flight_pace` |
| picture without a screen | `scene.renderToFile("out.png", width, height)`; program `Compages-headless` |
| file | `scene.loadFile("model.glb")` → `Result<Entity>` |
| prefab | `assets().loadFile(path)` then `scene.instantiate(id)` |
| frame | `scene.draw(ViewFrame)`, or `update` then `render()` / `render(cameraId)` |
| fit the view | `scene.frameAll()` |
| animation | `scene.play(model, "Walk")` `scene.clips(model)` |
| sky | `scene.skybox({ "+x", "-x", "+y", "-y", "+z", "-z" })` |
| picking | `scene.pick({ x, y })` |
| debug lines | `scene.debug()` |
| text on the picture | `scene.font(file)`, `entity.set(Label{ .text, .color, .pixels, .hide_closer_than })`, `entity.set(Tooltip{ .title, .text, .color, .accent, .background, .opacity, .pixels })`, `camera.set(Caption{ .text, .color, .pixels, .at })` |
| live texture | `CanvasTexture`: `create`, `setMapping`, `bind`, `paintWorld`, `sync`, `applyLook` |

---

## 🗺 6. Fields

Structured data, directly in `compages` (`#include <Compages/Fields/HeightField.hpp>` gives the three).

| Action | Call |
|--------|------|
| a grid | `Grid2D{ .columns, .rows, .origin, .spacing }`; `count()` `index(i, j)` `column(n)` `row(n)` `contains(i, j)` `point(i, j)` `coordinates(place)` `nearest(place)` |
| neighbours | `for (std::size_t n : grid.neighbours4(index))`, `neighbours8(index)` |
| values on it | `Field<T> field(grid, value)`; `field(i, j)` `field[index]` `values()` (contiguous span) `fill(value)` `size()` `grid()` |
| between the points | `field.sample(place)`: bilinear, the edge value outside |
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

Keys: [chapter 9 §1](09-Examples.md#-1-the-window).

Next: [09-Examples.md](09-Examples.md).
