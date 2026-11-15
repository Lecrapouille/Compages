# 🚀 1. Install and run

[⏫ Tutorials](README.md) · [Next: A first scene ▶](02-first-scene.md)

![00a_Workshop](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/00a_Workshop.png)

You will build Compages and open its gallery on this picture: a table in a workshop, waiting for a robot. It takes a terminal, a C++20 compiler and, the first time, an Internet connection and a few minutes.

## 📦 The tools

Compages builds on Linux with `make`. Drawing needs an OpenGL 4.5 driver: any recent Linux driver has one, and Mesa gives one in software when there is no graphics card.

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

macOS stops at OpenGL 4.1, too old for the drawing part of Compages; its simulation part builds there. The [build guide](../guides/build-and-install.md) has the details, the unit tests and the system install.

## 🔨 Build

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make
```

`--recurse-submodules` brings the build rules (the `.makefile/` folder). The first `make` also downloads the third-party libraries (EnTT, units, cgltf, stb, ImGui…) and the pictures and models of the demos. The commands of the next pages assume that you stay in this `Compages` folder.

## 🖼 Open the gallery

```sh
./build/linux-gcc-debug/bin/Compages-examples 00a_Workshop
```

One window opens, with the picture in the middle and panels around it: the list of the demos on the left, the **Source** of the demo, the **Inspector** and its controls. Hold the **right mouse button** and drag: the camera turns round the table. The **wheel** brings it closer. **PgDn** goes to the next demo, **F1** hides the panels, **Esc** quits.

**Check:** you see the table, a blue part on it, a lamp above it, and the camera turns when you drag. If the window stays black or does not open, see [A black window](../guides/debug-a-black-window.md).

Each demo of the gallery shows one idea, from `00a` to `12b`. The first five are the steps of these tutorials. The [gallery reference](../reference/gallery.md) lists all of them.

## ➡ Next

The table and the lamp come from twenty lines of C++. [The next page](02-first-scene.md) reads them, then puts them in a window of your own.

---

[⏫ Tutorials](README.md) · [Next: A first scene ▶](02-first-scene.md)
