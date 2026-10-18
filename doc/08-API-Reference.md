# 📖 8. API reference

The entry points and the common calls of the three layers on one page. For the context, see [4. GPU](04-GPU-Layer.md), [5. World](05-World-Layer.md), [6. Renderer](06-Renderer-Layer.md); for a walkthrough, [3. Tutorial](03-Tutorial.md).

Namespaces: `compages::gpu`, `compages::world`, `compages::renderer`; vectors, matrices and `Frame` are in `compages::core` (`compages::core::Vector3f`); `Status` and `Result<T>` are in `compages`.

---

## 📎 1. Includes

```cpp
#include <Compages/Compages.hpp>         // the whole stack
#include <Compages/GPU/GPU.hpp>
#include <Compages/World/Entity.hpp>     // includes World
#include <Compages/Renderer/Scene.hpp>
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
| draw | `Drawable d; d.load(vs, fs); d["name"] = …; d.draw();` |
| typed vertices | `d.vertices<Vertex>(span)`, `d.vertex<Vertex>(i)` |
| static buffer | `Buffer<T>::from(data, { .usage = BufferUsage::Immutable, .cpu_mirror = false })` |
| clear | `clear({ r, g, b })` |
| state | `d.state().depth_test = true;` `d.indices({ 0, 1, 2 });` |
| several shaders | `Pipeline::create<Vertex>(program, state)` |
| compute | `ComputeProgram::fromSource` or `load`; `dispatchItems(n)`; `barrier(Barrier::VertexAttrib)` |
| read back | `buffer.download()` after the right barrier |
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
| step | `compages::core::Frame step; step.elapsed = dt; world.update(step);` |
| behavior | `struct X: Behavior { void update(float dt) override; };` then `.add<X>(…)` |
| joint | `.revolute(axis, min, max)` `.angle(a)` `.prismatic(axis, min, max)` `.offset(o)` |
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
| looks | `color(r, g, b)` `texture("a.jpg")` `normals()` `depth(near, far)` |
| file | `scene.load("model.glb")` → `Result<Entity>` |
| prefab | `assets().load(path)` then `scene.instantiate(id)` |
| frame | `scene.draw(ViewFrame)`, or `update` then `render()` / `render(cameraId)` |
| fit the view | `scene.frameAll()` |
| animation | `scene.play(model, "Walk")` `scene.clips(model)` |
| sky | `scene.skybox({ "+x", "-x", "+y", "-y", "+z", "-z" })` |
| picking | `scene.pick({ x, y })` |
| debug lines | `scene.debug()` |
| live texture | `CanvasTexture`: `create`, `setMapping`, `bind`, `paintWorld`, `sync`, `applyLook` |

---

## ⏰ 6. Time

| Type | Holds |
|------|-------|
| `compages::core::Frame` | `elapsed`, `total`, `width`, `height` |
| `compages::world::ViewFrame` | a `Frame` plus keyboard and mouse |

---

## 📷 7. Gallery

```sh
./build/Compages-examples
./build/Compages-examples 50_ThreeJsLike
./build/Compages-examples --check
./build/Compages-examples --check --shots external/Compages-data/doc/examples
```

Keys: [chapter 9 §1](09-Examples.md#-1-the-window).

Next: [09-Examples.md](09-Examples.md).
