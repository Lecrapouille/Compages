# 🟩 4. A scene

[◀ The World](03-the-world.md) · [⏫ Tutorials](README.md) · [Next: values on a grid ▶](05-values-on-a-grid.md)

![13a_Workshop](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13a_Workshop.png)

The third layer draws a World. A `compages::renderer::Scene` gives entities a shape and a look, adds a camera and lights, and draws the picture with the GPU layer. You will build the workshop around the arm of the last page, give the arm its shapes, and then draw the whole scene in a window that your own program opens.

## 🏠 The room

The room is one function, `buildRoom()`, in [Workshop/Room.cpp](../../examples/13_Workshop/Workshop/Room.cpp).

The sky, the camera and the sun take one line each:

```cpp
p_scene.background(0.62f, 0.70f, 0.78f);
p_scene.camera().position(1.3f, 1.55f, 1.55f).add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.85f, 0.0f));
p_scene.sun("Sun", { 1.0f, 0.96f, 0.9f }, 1.0f).position(3.0f, 6.0f, 2.0f).lookAt(0.0f, 0.0f, 0.0f);
```

The camera stands 1.55 m high, in front of the table and to its right. `Orbit` is a routine like the one of the arm: it turns the camera round a point when you drag with the right mouse button. The sun is very far away, so only its direction counts, and `lookAt()` sets it. A colour is three numbers from 0 to 1: red, green, blue.

Each call returns an entity of the World. The camera and the sun are entities, like the joints of the arm.

The floor and a grid:

```cpp
p_scene.box("Floor", color(0.42f, 0.42f, 0.44f)).position(0.0f, -0.05f, 0.0f).scale(6.0f, 0.1f, 6.0f);
p_scene.grid(6.0f, 12u);
```

A box is one unit wide and centred on its position. `scale(6, 0.1, 6)` makes a slab of 6 m by 6 m, 10 cm thick, and `position(0, -0.05, 0)` puts its top at height zero. The grid draws a line every 50 cm. It is a **helper**: it is drawn for you, but it is not part of the world. Page 7 comes back to it.

The table is an empty entity with five boxes as children:

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

The positions of the children are measured from the table, not from the floor. Move the table, and the five boxes follow.

Last, a blue part that the arm cannot reach, and a lamp with a bulb:

```cpp
room.part = p_scene.box("Part", color(0.10f, 0.30f, 0.90f)).parent(room.table).position(0.55f, 0.05f, 0.2f).scale(0.18f, 0.1f, 0.12f);
compages::world::Entity lamp = p_scene.lamp("Lamp", { 1.0f, 0.85f, 0.6f }, 2.0f, 3.0f).position(0.2f, 1.5f, 0.3f);
p_scene.sphere("Bulb", compages::renderer::unlit(1.0f, 0.9f, 0.7f))
    .parent(lamp)
    .scale(0.08f)
    .get<compages::renderer::MeshRenderer>()
    .flags = compages::renderer::RenderFlags::NoShadowCaster;
```

A lamp lights round it, up to 3 m here. The lamp itself is invisible, so the bulb is a small sphere, its child. Its look is **unlit**: no light changes its colour, as for a real bulb. The last two lines say that the bulb casts no shadow. Without them, a small dark spot appears on the table under it.

## 🦾 The shapes of the arm

The arm of the last page has joints and no shapes. `dressArm()`, in [Workshop/ArmShapes.cpp](../../examples/13_Workshop/Workshop/ArmShapes.cpp), hangs shapes under the joints:

```cpp
links.push_back(p_scene.box("Upper arm", orange)
                    .parent(p_arm.shoulder)
                    .position(0.0f, 0.5f * UPPER_ARM, 0.0f)
                    .scale(0.08f, UPPER_ARM, 0.08f));
```

The upper arm is 50 cm long, and it is moved up by half its length. Without this, the box would be centred on the shoulder and would turn round its middle, like a propeller. A shape that must turn round its end is always placed this way. The foot, the forearm, two spheres at the joints and a cone at the tip are made the same way.

The demo then calls the two halves together:

```cpp
workshop::buildRoom(m_scene);
m_arm = workshop::buildArm(m_world);   // the World, page 3
workshop::dressArm(m_scene, m_arm);    // the Renderer
return m_scene.prepare();
```

and at each frame, `m_scene.draw(p_frame)`. `prepare()` checks the whole scene once, before the first frame, and returns the first error. `draw(frame)` updates the World (the routine of the arm runs there), then draws it.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 13a_Workshop
```

The arm moves its tip along the table, as the numbers of page 3 said. Drag with the right mouse button to turn round the table. **F6** pauses and **F7** advances one frame.

## 🪟 The same scene in your own window

The gallery opens the window and calls `draw()` for you. Your own program does it itself. Compages never opens a window: it draws into the window your program made, here with GLFW (SDL or Qt work the same way). Save this as `workshop.cpp` in the `Compages` folder:

```cpp
#include "13_Workshop/Workshop/Workshop.hpp"

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
    examples::workshop::Arm arm = examples::workshop::buildArm(world);
    examples::workshop::dressArm(scene, arm);
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
        frame.elapsed = compages::si::Seconds(now - previous);
        frame.total = compages::si::Seconds(now - started);
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

Build it with the three files of the workshop it uses, and run it:

```sh
export PKG_CONFIG_PATH="$PWD/build/linux-gcc-debug/lib/pkgconfig"
g++ -std=c++20 workshop.cpp examples/13_Workshop/Workshop/Room.cpp \
    examples/13_Workshop/Workshop/Arm.cpp examples/13_Workshop/Workshop/ArmShapes.cpp -Iexamples \
    $(pkg-config --cflags Compages glfw3) build/linux-gcc-debug/lib/libCompages.a \
    $(pkg-config --libs glfw3) -o workshop
./workshop
```

The workshop appears in a window of 1024 by 768 pixels, and the arm moves. The camera does not turn: `Orbit` reads the mouse from `frame.input`, and this program does not fill it. The gallery does, in [examples/Common/Window.cpp](../../examples/Common/Window.cpp).

The order of the steps matters:

1. Make a window and its OpenGL context, then call `gpu::init`. Compages needs a context and never makes one.
2. Make the `World` and the `Scene`, build the scene, call `prepare()`.
3. At each frame, fill a `ViewFrame` with the time and the size of the window, open a `RenderPass` (where to draw), call `draw`, then show the picture.
4. At the end, leave `run()` first, so the scene frees its GPU objects, then call `gpu::shutdown()` while OpenGL still exists.

## 🧪 Try

- Move the table: `position(0.5f, TABLE_TOP, 0.0f)` in `buildRoom()`. The top, the legs and the part follow. The arm and the lamp stay, because they are not children of the table.
- Set the range of the lamp to `0.5f`. Its light no longer reaches the table.
- Remove `.position(0.0f, 0.5f * UPPER_ARM, 0.0f)` from the upper arm. It turns round its middle, and no longer joins the elbow.

The World holds where things are; the Scene adds what they look like and draws them. [Three layers](../concepts/three-layers.md) explains why they are kept apart, and [one frame](../concepts/one-frame.md) what `draw()` does inside.

---

[◀ The World](03-the-world.md) · [⏫ Tutorials](README.md) · [Next: values on a grid ▶](05-values-on-a-grid.md)
