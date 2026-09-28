# Cheat Sheet

Entry points and common calls. For usage context, see [GPU.md](GPU.md), [World.md](World.md), [Renderer.md](Renderer.md). The walkthrough is in [Tutorial.md](Tutorial.md).

The names `Vector2f`, `Vector3f`, `Vector4f` come from the library headers (global namespace). The subsystems live in `compages::gpu`, `compages::world`, `compages::renderer`. `Status` and `Result<T>` are in `compages`.

## Includes

```cpp
#include <Compages/Compages.hpp>         // complete stack
#include <Compages/GPU/GPU.hpp>
#include <Compages/World/Entity.hpp>     // includes World
#include <Compages/Renderer/Scene.hpp>
```

## Errors

```cpp
COMPAGES_TRY(expr);                      // propagate error Status / Result
COMPAGES_TRY_ASSIGN(var, expr);          // Result<T> → T, or propagate error

COMPAGES_TRY(scene.prepare());           // first construction error
COMPAGES_TRY(drawable.prepare());

const std::string err = compages::gpu::takeFrameError();  // "" if no error
```

Loading (shader, file): `Result` is returned immediately.
Frame (name, type): message is stored, read at end of frame or via `prepare()`.

## GPU

| Action         | Call |
|----------------|------|
| Device         | `compages::gpu::init(loader)` / `shutdown()` / `initialized()` |
| Drawing        | `Drawable d; d.load(vs, fs); d["name"] = …; d.draw();` |
| Typed vertices | `d.vertices<Vertex>(span);` `d.vertex<Vertex>(i)` |
| Static buffer  | `Buffer<T>::from(data, { .usage = BufferUsage::Immutable, .cpu_mirror = false })` |
| Clear          | `clear({ r, g, b })` |
| State          | `d.state().depth_test = true;` `d.indices({ 0, 1, 2 });` |
| Multiple shaders | `Pipeline::create<Vertex>(program, state)` |
| Compute        | `ComputeProgram::fromSource` or `load`; `dispatchItems(n)`; `barrier(Barrier::VertexAttrib)` |
| Readback       | `buffer.download()` after an appropriate barrier |
| After ImGui    | `forgetRenderState()` |

GLSL names = keys for `operator[]`. Flush on `draw()` for a `Drawable`, or `upload()` if you're managing a `Buffer` yourself.

## World

| Action        | Call |
|---------------|------|
| Create entity | `world.entity("Name")` |
| Child entity  | `parent.child("Child")` |
| Path lookup   | `world.lookup("Parent/Child")` |
| Transform     | `.position(x,y,z)` `.rotate(rad, axis)` `.scale(s)` `.lookAt(target)` |
| Component     | `.set(T{})` `.add<T>(args)` `.get<T>()` `.has<T>()` `.remove<T>()` |
| Loop (system) | `world.each<A, B>([](Entity e, A& a, B& b){ … })` |
| Step/frame    | `Frame step; step.elapsed = dt; world.update(step);` |
| Behavior      | `struct X : Behavior { void update(float dt) override; };` then `.add<X>(…)` |
| Joint         | `.revolute(axis, min, max)` `.angle(a)` `.prismatic(…)` `.offset(o)` |
| Life cycle    | `if (entity)` ; `entity.enable(false)` |

## Renderer

| Action         | Call |
|----------------|------|
| Scene          | `Scene scene(world);` or `Scene scene(world, assets)` |
| Background/light | `scene.background(r,g,b)` `scene.ambient(r,g,b)` |
| Camera/sun/lamp | `scene.camera()` `scene.sun(name, color, intensity)` `scene.lamp(…)` |
| Primitives     | `scene.box(name, look)` `sphere` `plane` `cylinder` `cone` `pyramid` |
| Look           | `color(r,g,b)` `texture("a.jpg")` `normals()` `depth(near, far)` |
| File           | `scene.load("model.glb")` → `Result<Entity>` |
| Prefab         | `assets().load(path)` then `scene.instantiate(id)` |
| Frame          | `scene.draw(ViewFrame)` or `update` then `render()` / `render(cameraId)` |
| Fit scene      | `scene.frameAll()` |
| Animation      | `scene.play(model, "Walk")` `scene.clips(model)` |
| Skybox         | `scene.skybox({ "+x", "-x", "+y", "-y", "+z", "-z" })` |
| Picking        | `scene.pick({ x, y })` |
| Debug          | `scene.debug()` |

## Time

| Type               | Contents                           |
|--------------------|------------------------------------|
| `world::Frame`     | `elapsed`, `total`, `width`, `height` |
| `world::ViewFrame` | a `Frame` + keyboard and mouse info  |

## Example Gallery

```sh
./build/Compages-examples
./build/Compages-examples 50_ThreeJsLike
./build/Compages-examples --check
./build/Compages-examples --check --shots external/Compages-data/doc/examples
```

| Key         | Action                  |
|-------------|------------------------|
| PgUp/PgDn   | previous / next demo   |
| F1          | panels                 |
| F2          | wireframe mode         |
| F5          | restart demo           |
| F6 / F7     | pause / step frame     |
| F12         | capture viewport (`screenshots/`) |
| Shift+F12   | capture entire window  |
| Esc         | release mouse, or quit |

`--cycle` cycles the demos. `--no-overlay` starts with panels hidden.
