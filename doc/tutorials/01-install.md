# 🚀 1. Install and run

[⏫ Tutorials](README.md) · [Next: the GPU ▶](02-the-gpu.md)

You will build Compages and open its gallery. You need a terminal, a C++20 compiler and, the first time, an Internet connection and a few minutes.

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

---

[⏫ Tutorials](README.md) · [Next: the GPU ▶](02-the-gpu.md)
