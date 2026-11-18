# 🚀 1. Install and run

[◀ Glossary](00-glossary.md) · [⏫ Tutorials](README.md) · [Next: the GPU ▶](02-the-gpu.md)

You want to check that Compages works before writing a 3D program. You need a terminal, a C++20 compiler and, the first time, an Internet connection and a few minutes.

![The triangle you will run](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06a_FirstScene.png)

## 🎯 Goal

Build Compages, run the triangle in its gallery, and compile a program against the library. You will check drawing in the gallery and print an entity's position from your own program, without a system install.

## 📦 The tools

Compages builds on Linux with `make`. Drawing needs an OpenGL 4.5 driver. Check that your driver supports this version. Mesa can also draw in software when no graphics card is available.

On Ubuntu or Debian:

```sh
sudo apt-get install --no-install-recommends \
    build-essential cmake git pkg-config \
    libglfw3-dev libgl1-mesa-dev libegl-dev
```

On Fedora:

```sh
sudo dnf install gcc-c++ make cmake git pkgconf-pkg-config \
    glfw-devel mesa-libGL-devel mesa-libEGL-devel
```

macOS stops at OpenGL 4.1. This is too old for the drawing part of Compages, but its World part builds there. The [build guide](../guides/build-and-install.md) has the details, the unit tests and the system install.

## 🔨 Build

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make
```

`--recurse-submodules` brings the build rules, in the `.makefile/` folder. The first `make` also downloads the third-party libraries (EnTT, units, cgltf, stb, ImGui…) and the pictures and models of the demos. The next pages assume that you stay in this `Compages` folder.

## 🖼 Open the gallery

```sh
./build/linux-gcc-debug/bin/Compages-examples 01c_Triangle
```

A window opens. The picture is in the middle, with panels around it: the list of the demos on the left, the source code of the demo, its controls. **PgDn** goes to the next demo, **F1** hides the panels, **Esc** quits. In a 3D demo, hold the right mouse button and drag to turn the camera, and use the wheel to come closer.

You should see a triangle with a red, a green and a blue corner. If the window stays black or does not open, see [a black window](../guides/debug-a-black-window.md).

The gallery has more than seventy demos, in folders that follow the layers: `01` to `05` for the GPU, `06` to `08` for scenes and the World, then bigger demos. The [gallery reference](../reference/gallery.md) lists them all.

## 🧰 Compile a program before installing

First check that your compiler can use the library you built. You can do this inside the repository, without installing anything on your system. Save this complete program as `hello.cpp` in the `Compages` folder:

```cpp
#include <Compages/World/World.hpp>
#include <Compages/World/Entity.hpp>

#include <cstdio>

int main()
{
    compages::world::World world;
    auto box = world.entity("Box").position(1.0f, 0.0f, 0.0f);
    world.update(compages::Frame{});
    std::printf("Box x = %.1f m\n", double(box.worldPosition().x));
    return 0;
}
```

Build and run it from that same folder:

```sh
export PKG_CONFIG_PATH="$PWD/build/linux-gcc-debug/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
pkg-config --modversion Compages
g++ -std=c++20 hello.cpp $(pkg-config --cflags Compages-World) \
    build/linux-gcc-debug/lib/libCompages-World.a -o hello
./hello
```

`pkg-config` prints the library version. The program prints:

```text
Box x = 1.0 m
```

`--cflags` gives the include paths, including those of the public dependencies. The `.a` file is the static library: this command uses it directly. Keep the `PKG_CONFIG_PATH` export in each new terminal used for these tutorials. The later build commands use the same arrangement.

This program creates one entity and reads its position after an update. Chapter 3 explains those calls. It opens no window and needs no OpenGL context. Seeing the triangle checks drawing; running `hello` checks compilation and linking in your own program.

## 📥 Install for another project

You can finish all tutorials from the repository. Install Compages when another project needs it. For a local install without administrator rights:

```sh
make install PREFIX="$HOME/compages"
export PKG_CONFIG_PATH="$HOME/compages/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$HOME/compages/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
g++ -std=c++20 hello.cpp -o hello $(pkg-config --cflags --libs Compages)
./hello
```

The install copies public headers, both libraries, and `Compages.pc` under the chosen prefix. `pkg-config --libs` selects the library. `LD_LIBRARY_PATH` lets Linux find the shared library when the program starts. These exports affect the current terminal. The [build guide](../guides/build-and-install.md) covers system installation and other paths.

Before chapter 3, return to the build-tree `PKG_CONFIG_PATH` export above. Its commands link the repository's static library.

## 🧯 If a step fails

- If `make` cannot find its rules, run `git submodule update --init --recursive`, then `make` again.
- If `pkg-config` cannot find Compages, check that you exported the path from the repository root after `make` completed.
- If the compiler cannot find `Compages/World/World.hpp`, check the `pkg-config --cflags Compages` output.
- If the triangle is black, use [debug a black window](../guides/debug-a-black-window.md).
- Without a display, use [run without a screen](../guides/run-without-a-screen.md) for drawing checks.

## 🧪 Try

- In `hello.cpp`, change `1.0f` to `2.0f`. Recompile and run: the printed x coordinate becomes `2.0 m`.
- Start the gallery without a demo name. Select `01c_Triangle` from its list to find the same picture.

You can now build, run, and edit a program that uses Compages. Next, you will connect the triangle's data to its shaders.

[◀ Glossary](00-glossary.md) · [⏫ Tutorials](README.md) · [Next: the GPU ▶](02-the-gpu.md)
