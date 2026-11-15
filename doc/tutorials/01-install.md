# 🚀 1. Install and run

[◀ Glossary](00-glossary.md) · [⏫ Tutorials](README.md) · [Next: the GPU ▶](02-the-gpu.md)

You will build Compages, open its gallery, then install it and build a program of your own against it. You need a terminal, a C++20 compiler and, the first time, an Internet connection and a few minutes.

## 📦 The tools

Compages builds on Linux with `make`. Drawing needs an OpenGL 4.5 driver. Any recent Linux driver has one, and Mesa gives one in software when there is no graphics card.

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


## 📥 Use Compages in your own project

The gallery is built inside the Compages folder. Your own program lives in another folder, and it must find the headers and the library of Compages. Install them on your system once:

```sh
sudo make install
sudo ldconfig
```

`make install` copies the headers to `/usr/local/include/Compages/`, the libraries `libCompages.so` and `libCompages.a` to `/usr/local/lib/`, and a file `Compages.pc` to `/usr/local/lib/pkgconfig/`. This last file tells the compiler which flags Compages needs: `pkg-config` reads it. `ldconfig` updates the list of libraries that the system knows. On Fedora, `/usr/local/lib` is not in this list, so add it once:

```sh
echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/usrlocal.conf
sudo ldconfig
```

Check the install:

```sh
pkg-config --modversion Compages
```

It prints `1.0.0`. To install somewhere else, without `sudo`, give a folder: `make install PREFIX=$HOME/compages`, then `export PKG_CONFIG_PATH=$HOME/compages/lib/pkgconfig` and `export LD_LIBRARY_PATH=$HOME/compages/lib` in each terminal.

The repository [LinkAgainstMyLibs](https://github.com/Lecrapouille/LinkAgainstMyLibs) holds small projects that use the installed library, as your project will. Its `OpenGL/` folder has two programs: `Triangle` opens a window and draws the triangle of the gallery, and `HeadlessCompute` runs a compute shader in a hidden window and prints the result.

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/LinkAgainstMyLibs.git
cd LinkAgainstMyLibs/OpenGL
make
make run-HeadlessCompute
make run-Triangle
```

`HeadlessCompute` prints:

```text
CPU before:  0 0 0 0 0 0 0 0 0 0 0 0
GPU after:   1 2 3 4 5 6 7 8 9 10 11 12
Headless compute OK.
```

The build uses the same rules as Compages (the `.makefile/` submodule). The whole [Makefile](https://github.com/Lecrapouille/LinkAgainstMyLibs/blob/master/OpenGL/Makefile) of a program is a few lines. `PKG_CONFIG` names the libraries to find through `pkg-config`:

```make
TARGETS := Triangle HeadlessCompute

Triangle.SOURCES := Triangle/main.cpp src/GpuContext.cpp
Triangle.INCLUDES := src
Triangle.PKG_CONFIG := Compages glfw3
```

Without these rules, one `g++` line does the same, with any build tool that calls `pkg-config`:

```sh
g++ -std=c++20 HeadlessCompute/main.cpp src/GpuContext.cpp -Isrc \
    $(pkg-config --cflags --libs Compages glfw3) -o HeadlessCompute
```

Look at `src/GpuContext.cpp`: the program opens the window itself, with GLFW, and only then calls `compages::gpu::init()`. Compages never opens a window. So you can use the window library you want (GLFW, SDL, Qt…), or no window at all for the World. The [build guide](../guides/build-and-install.md) lists the install variables (`PREFIX`, `LIBDIR`…) and the common errors.

---

[◀ Glossary](00-glossary.md) · [⏫ Tutorials](README.md) · [Next: the GPU ▶](02-the-gpu.md)
