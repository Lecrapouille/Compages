# 🧊 2. A first scene

[◀ Install and run](01-install.md) · [⏫ Tutorials](README.md) · [Next: Make it move ▶](03-make-it-move.md)

![00a_Workshop](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/00a_Workshop.png)

You will read the code of this picture line by line: a camera, the light of the sun, a table made of five boxes, a part and a lamp. Then you will put the same scene in a window that your own program opens.

## 🧱 The scene, line by line

The whole room is one function, `buildRoom()`, in [Workshop/Room.cpp](../../examples/00_Workshop/Workshop/Room.cpp). It receives a `compages::renderer::Scene`: the object that knows how to draw. The gallery made it for us with a `World` behind it; we come back to this pair at the end of the page.

**The sky, the camera, the sun.** Three lines, one thing each:

```cpp
p_scene.background(0.62f, 0.70f, 0.78f);
p_scene.camera().position(1.3f, 1.55f, 1.55f).add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.85f, 0.0f));
p_scene.sun("Sun", { 1.0f, 0.96f, 0.9f }, 1.0f).position(3.0f, 6.0f, 2.0f).lookAt(0.0f, 0.0f, 0.0f);
```

Distances are in metres, and **y goes up**: the camera stands 1.55 m high, a little in front of the table and to its right. `Orbit` is what makes the right mouse button turn the camera round a point, here 85 cm above the floor, the height of the table. The sun is far away by definition: only the direction it shines in counts, set by `lookAt()`. Colours are three numbers from 0 to 1: red, green, blue.

Each call returns an **entity**: a thing of the scene, with a name and a place. `position()`, `lookAt()`, `add<>()` all return the entity again, so they chain.

**The floor and a grid.**

```cpp
p_scene.box("Floor", color(0.42f, 0.42f, 0.44f)).position(0.0f, -0.05f, 0.0f).scale(6.0f, 0.1f, 6.0f);
p_scene.grid(6.0f, 12u);
```

A box is one unit wide and centred on its position. `scale(6, 0.1, 6)` makes it a slab of 6 m by 6 m, 10 cm thick, and `position(0, -0.05, 0)` puts its top at height zero. The grid draws a line every 50 cm: a **helper**, drawn for you and not part of the world. Remember it for [step 6](06-what-the-robot-sees.md).

**The table: a parent and its children.**

```cpp
room.table = p_scene.world().entity("Table").position(0.0f, TABLE_TOP, 0.0f);
p_scene.box("Top", color(0.55f, 0.38f, 0.22f)).parent(room.table).position(0.0f, -0.03f, 0.0f).scale(1.6f, 0.06f, 1.0f);
for (float x : { -0.72f, 0.72f })
{
    for (float z : { -0.42f, 0.42f })
    {
        p_scene.box("Leg", color(0.30f, 0.22f, 0.14f))
            .parent(room.table)
            .position(x, -0.5f * TABLE_TOP, z)
            .scale(0.06f, TABLE_TOP - 0.06f, 0.06f);
    }
}
```

The table itself is an **empty entity**: a place, 78 cm above the floor (`TABLE_TOP`), with nothing to draw. The top and the four legs are its **children**: their positions are measured from the table, not from the floor. The top sits 3 cm under the table's point, so its upper face is exactly at 78 cm; the legs go down by half the height. Move the table, and the five boxes follow. This tree of parents and children is the **scene graph**; the next page builds a robot arm with it.

**The part and the lamp.**

```cpp
room.part = p_scene.box("Part", color(0.10f, 0.30f, 0.90f)).parent(room.table).position(0.02f, 0.05f, -0.12f).scale(0.18f, 0.1f, 0.12f);
compages::world::Entity lamp = p_scene.lamp("Lamp", { 1.0f, 0.85f, 0.6f }, 2.0f, 3.0f).position(0.2f, 1.5f, 0.3f);
p_scene.sphere("Bulb", compages::renderer::unlit(1.0f, 0.9f, 0.7f))
    .parent(lamp)
    .scale(0.08f)
    .get<compages::renderer::MeshRenderer>()
    .flags = compages::renderer::RenderFlags::NoShadowCaster;
```

A lamp lights round it, up to 3 m here, and casts no light beyond. It is invisible: the bulb is a small sphere, its child, with an **unlit** look (a colour that no light changes, as a bulb should be). The last two lines tell the Renderer that the bulb casts no shadow; without them, a small dark spot appears on the table under it.

**Then the gallery calls `prepare()` once, and `draw()` at each frame.** `prepare()` checks the whole scene before the first frame and returns the first error, if any. `draw(frame)` moves the world by the time of the frame, then draws it.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 00a_Workshop
```

## 🧪 Try

Change a value in [Room.cpp](../../examples/00_Workshop/Workshop/Room.cpp), run `make`, and run the demo again:

- Paint the part red: `color(0.9f, 0.15f, 0.1f)`. Every step after this one shows it red.
- Move the table: `position(0.5f, TABLE_TOP, 0.0f)`. The top, the legs and the part follow; the lamp stays, since it is not a child of the table.
- Set the range of the lamp to `0.5f`: its light no longer reaches the table.

Put the values back before the next page: step 6 looks for a blue part.

## 🪟 The same scene in your own window

The gallery opens the window and calls `draw()` for you. Your own program does it itself, and it is not long. Compages never opens a window: it draws into the one your program made (with GLFW here; SDL or Qt work the same way). Save this as `workshop.cpp` in the `Compages` folder:

```cpp
#include "00_Workshop/Workshop/Workshop.hpp"

#include <Compages/Compages.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

// Everything the scene owns is destroyed when this function returns,
// while OpenGL is still there.
int run(GLFWwindow* p_window)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    examples::workshop::buildRoom(scene);
    if (auto ready = scene.prepare(); !ready)
    {
        std::cerr << ready.error() << '\n';
        return 1;
    }

    const double started = glfwGetTime();
    double previous = started;
    while (glfwWindowShouldClose(p_window) == GLFW_FALSE)
    {
        glfwPollEvents();
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(p_window, &width, &height);
        if ((width <= 0) || (height <= 0))
        {
            continue; // a minimized window has nothing to draw into
        }
        const double now = glfwGetTime();
        compages::world::ViewFrame frame;
        frame.width = static_cast<std::uint32_t>(width);
        frame.height = static_cast<std::uint32_t>(height);
        frame.elapsed = static_cast<float>(now - previous);
        frame.total = static_cast<float>(now - started);
        previous = now;
        {
            const compages::gpu::RenderPass pass({ .width = frame.width, .height = frame.height });
            scene.draw(frame);
        } // the pass ends before the picture is shown
        if (const auto error = compages::gpu::takeFrameError(); !error.empty())
        {
            std::cerr << error << '\n';
            return 1;
        }
        glfwSwapBuffers(p_window);
    }
    return 0;
}

int main()
{
    if (glfwInit() == GLFW_FALSE)
    {
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Workshop", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    int result = 1;
    if (auto ready = compages::gpu::init(reinterpret_cast<compages::gpu::LoadProc>(glfwGetProcAddress)); !ready)
    {
        std::cerr << ready.error() << '\n';
    }
    else
    {
        result = run(window);
        compages::gpu::shutdown();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
```

Build it with the room of the workshop, against the library you built, and run it:

```sh
export PKG_CONFIG_PATH="$PWD/build/linux-gcc-debug/lib/pkgconfig"
g++ -std=c++20 workshop.cpp examples/00_Workshop/Workshop/Room.cpp -Iexamples \
    $(pkg-config --cflags Compages glfw3) build/linux-gcc-debug/lib/libCompages.a \
    $(pkg-config --libs glfw3) -o workshop
./workshop
```

The same table appears in a window of 1024 by 768 pixels. The camera does not turn: `Orbit` reads the mouse from `frame.input`, and this program does not fill it (the gallery does, in [examples/Common/Window.cpp](../../examples/Common/Window.cpp)).

The program has five parts, in an order that matters:

| Part | When | Why |
|------|------|-----|
| a window and its OpenGL context, then `gpu::init` | once, first | Compages needs a context to talk to the GPU, and never makes one |
| `World`, `Scene`, `buildRoom`, `prepare` | once, before the loop | build the scene and check it |
| `ViewFrame` | each frame | the time since the last frame, and the size of the window |
| `RenderPass`, `draw`, swap | each frame | choose where to draw, move and draw, then show the picture |
| leave `run()`, then `gpu::shutdown` | at the end | the scene frees its GPU objects while OpenGL still exists |

## 💡 What happened

Two objects did the work. The **`World`** holds the entities: where each thing is, who is the child of whom. It knows nothing of pictures, and runs in a test with no screen. The **`Scene`** gives some entities a shape and a look, and draws them. This split is the heart of Compages: [three layers](../concepts/three-layers.md) explains why, and [one frame](../concepts/one-frame.md) what `draw()` does inside.

---

[◀ Install and run](01-install.md) · [⏫ Tutorials](README.md) · [Next: Make it move ▶](03-make-it-move.md)
