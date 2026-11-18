# 🖥 Run without a screen

[⏫ Guides](README.md)

Run a simulation, draw a picture, or check the gallery on a machine with no screen: a server, a container, a CI job, a unit test.

There are two very different cases. A **World** alone needs no graphics at all. **Drawing** needs an OpenGL context, but not a window.

## 🟦 A simulation: nothing to do

The World never touches the GPU. A program that only moves entities, joints and routines builds against the library and runs anywhere, with no context and no driver:

```cpp
#include <Compages/World/Entity.hpp>
#include <iostream>

using namespace units::literals;

int main()
{
    compages::world::World world;
    compages::world::Entity shoulder = world.entity("Robot").child("Shoulder")
        .position(0.0f, 0.0f, 0.5f)
        .revolute({ 1, 0, 0 }, -90.0_deg, 90.0_deg);
    compages::world::Entity hand = shoulder.child("Hand").position(0.0f, 0.0f, 1.0f);

    compages::Frame step;
    step.elapsed = compages::si::Seconds(1.0 / 60.0);
    for (auto angle : { 0.0_deg, 90.0_deg, 120.0_deg })
    {
        shoulder.joint().angle(angle);
        world.update(step);
        compages::Vector3f p = hand.worldPosition();
        std::cout << "angle " << angle.value() << " deg -> hand at ("
                  << p.x << ", " << p.y << ", " << p.z << ")\n";
    }
    return 0;
}
```

```sh
export PKG_CONFIG_PATH="$PWD/build/linux-gcc-debug/lib/pkgconfig"
g++ -std=c++20 robot.cpp $(pkg-config --cflags Compages-World) build/linux-gcc-debug/lib/libCompages-World.a -o robot
./robot
```

```text
angle 0 deg -> hand at (0, 0, 1.5)
angle 90 deg -> hand at (0, -1, 0.5)
angle 120 deg -> hand at (0, -1, 0.5)
```

120° is outside the limits of the joint: it stays at 90°. A tiny residual such as `-4.37114e-08` where the output shows zero is floating-point rounding. A gallery demo can test itself this way: `07a_HeadlessWorld` calls `gpu::reportError()` if its player did not move, and `--check` fails, with zero pixels to look at.

## 🖼 A picture into a file

`Scene::renderToFile()` draws what the active camera sees into an off-screen picture and writes a PNG:

```cpp
scene.update(frame);
COMPAGES_TRY(scene.renderToFile("scene.png", 1280u, 720u));
```

It needs an OpenGL context, not a window. `Compages-headless` ([examples/Headless/main.cpp](../../examples/Headless/main.cpp)) makes one with **EGL and no surface**, then calls `gpu::init(eglGetProcAddress)`: the same code runs on a server, in a container or in CI, on a GPU or with Mesa llvmpipe (no GPU).

```sh
./build/linux-gcc-debug/bin/Compages-headless scene.png 1280 720
```

`renderToFile` waits for the GPU: use it for a report, a thumbnail, or a test that compares pictures, not at every frame. For a picture you read each frame (a robot camera), draw into a `RenderTarget` and read it at the rate of the sensor ([what the robot sees](../tutorials/07-what-the-robot-sees.md)).

## 🤖 The gallery and the tests in CI

The gallery opens a window, so give it a virtual screen and a software OpenGL:

```sh
xvfb-run -a env \
    LIBGL_ALWAYS_SOFTWARE=1 \
    MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 \
    ./build/linux-gcc-debug/bin/Compages-examples --check
```

The same prefix works for `Compages-UnitTest`. `--check` runs each demo a few frames, closes it, and fails on a frame error or a GPU handle left behind; `--check NAME` checks one demo.

## 🧯 Common problems

- **`--check` blocks forever on a desktop.** On Wayland, a locked session or a hidden window makes the swap wait. Unlock the screen, or use `xvfb-run` as above.
- "EGL could not use the context without a surface". The driver lacks `EGL_KHR_surfaceless_context`; Mesa has it.

## 🔗 See also

[◀ The World](../concepts/the-world.md) · [Build and install](build-and-install.md) · [A black window](debug-a-black-window.md)
