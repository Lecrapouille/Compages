# 🚀 2. Getting started

Your first goal is to **see a rotating cube in the gallery**. Follow sections 1–3 on Linux. Sections 4–5 are optional: tests, servers and system installation can wait until your first program works.

Run the commands in a terminal. A terminal accepts one command at a time; a trailing `\` continues a command on the next line. Commands below assume you stay in the root folder of your Compages clone after `cd Compages`. The first build needs an Internet connection and can take time to download and compile dependencies.

---

## 📦 1. Install the tools

Compages builds in **C++20**, mainly on Linux, with `make` (the build system is [MyMakefile](https://github.com/Lecrapouille/MyMakefile), in `.makefile/`). The `GL45` backend needs an OpenGL **4.5 core** context for the gallery and the GPU tests. The library itself does not depend on GLFW: the window belongs to your application.

| Tool | Role |
|------|------|
| g++ or clang, C++20 | compilation |
| Make, CMake | the build, and a few third-party libraries |
| Git | sources, the `.makefile` submodule, the third-party clones |
| pkg-config | `Compages.pc` after `make install` |
| GLFW and an OpenGL 4.5 driver | **the gallery and the GPU tests only** |
| EGL (Linux) | `Compages-headless`: a picture drawn with no screen |

### Ubuntu / Debian

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
    build-essential cmake git pkg-config \
    libglfw3-dev libgl1-mesa-dev libegl-dev
sudo apt-get install libgtest-dev libgmock-dev          # unit tests
sudo apt-get install xvfb mesa-utils libgl1-mesa-dri    # machine without screen (CI)
```

### Fedora

```sh
sudo dnf install gcc-c++ make cmake git pkgconf-pkg-config \
    glfw-devel mesa-libGL-devel mesa-libEGL-devel
sudo dnf install gtest-devel gmock-devel                # unit tests
sudo dnf install xorg-x11-server-Xvfb mesa-dri-drivers  # machine without screen (CI)
```

### macOS (Homebrew)

macOS stops at OpenGL 4.1 and cannot create the context required by the current GL45 drawing backend. Use Linux for the gallery and the visual tutorials. World-only code does not need OpenGL. The project has no Windows setup walkthrough here.

```sh
brew install cmake glfw googletest pkg-config
```

For CPU builds, the CI exports the Homebrew paths (`USER_CXXFLAGS`, `USER_LDFLAGS`, `PKG_CONFIG_PATH`): see `.github/workflows/compile.yml`.

---

## 🔨 2. Build

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make                             # the library and the gallery
```

If you already cloned without submodules, repair the clone with:

```sh
git submodule update --init --recursive
```

`--recurse-submodules` fetches `.makefile`, the build system ([MyMakefile](https://github.com/Lecrapouille/MyMakefile)). Without it, `make` has no rules. `make` uses all the cores.

The third-party libraries are not git submodules. `external/external.mk` declares them; the first `make` fetches them at the commits of `external/versions.lock`, in a cache shared by all your projects (`~/.cache/mymakefile`):

| Repository | Role |
|------------|------|
| `skypjack/entt` | the ECS (public headers) |
| `nholthaus/units` | angle literals such as `90.0_deg` (public header) |
| `jkuhlmann/cgltf` | reading glTF files (compilation only) |
| `nlohmann/json` | saving scenes |
| `nothings/stb` | images |
| `zeux/pugixml` | URDF files |
| `ocornut/imgui` | the panels of the gallery |
| `Lecrapouille/Compages-data` | textures, glTF and URDF files of the demos |

The demos find the assets in the cache, then in `external/Compages-data/` (a clone of yours, to regenerate the screenshots) and in the `COMPAGES_DATA_PATH` environment variable.

After the build, in `build/linux-gcc-debug/` (`make COMPILATION_MODE=release` builds in `build/linux-gcc-release/`):

| Path | Contents |
|------|----------|
| `lib/libCompages.a` | the static library |
| `lib/libCompages.so` | the shared library |
| `lib/pkgconfig/Compages.pc` | its flags, to build against it without installing it |
| `bin/Compages-examples` | the gallery |

`make help` lists the variables (`GPU_BACKEND`, `PREFIX`, …). `GPU_BACKEND` is `GL45` in the `Makefile`. The day a second backend exists, you will change it there, not in the code that calls `gpu::`.

---

## 📷 3. Open the gallery

```sh
./build/linux-gcc-debug/bin/Compages-examples 06a_FirstScene  # start here: a rotating cube
./build/linux-gcc-debug/bin/Compages-examples                 # every demo, with panels
./build/linux-gcc-debug/bin/Compages-examples 05f_Galaxy       # start on one demo
```

One window holds a viewport, the list of demos, an inspector, the source of the demo, a console and GPU counters. **PgUp / PgDn** change demo, **F1** hides the panels, **Esc** quits. All keys: [chapter 9](09-Examples.md#-1-the-window).

![Galaxy: eight thousand stars in a compute shader](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05f_Galaxy.png)

**Checkpoint:** the window shows a lit, rotating cube. Hold the right mouse button and drag: the camera turns around it. Use the wheel: the camera moves closer or farther away. If no window opens or the image stays black, use the [first checks in chapter 10](10-Debugging.md#-0-start-with-the-smallest-check).

When this works, go to [Your first programs](03-Tutorial.md). You do not need to install Compages system-wide to follow the tutorials.

Optional: check that every demo draws and frees its GPU objects:

```sh
./build/linux-gcc-debug/bin/Compages-examples --check
```

`--check` runs each demo for a few frames, closes it, and fails if a GPU handle is left. Add the name of a demo to check only that one: `--check 06a_FirstScene`.

---

## 🧪 4. Tests, and a machine without screen

```sh
make check
```

The tests binary is `build/linux-gcc-debug/bin/Compages-UnitTest`. Local coverage: `make coverage`.

On a Linux machine without screen (CI, server), give a software OpenGL to the program:

```sh
xvfb-run -a env \
    LIBGL_ALWAYS_SOFTWARE=1 \
    MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 \
    ./build/linux-gcc-debug/bin/Compages-examples --check
```

The same prefix works for `./build/linux-gcc-debug/bin/Compages-UnitTest`. A program that only uses the **World** layer needs none of this: it never touches the GPU ([chapter 3 §2](03-Tutorial.md#-2-a-world-without-a-screen)).

---

## 📥 5. Install on your system

| Variable | Default |
|----------|---------|
| `PREFIX` | `/usr/local` |
| `INCLUDEDIR` | `$(PREFIX)/include` |
| `LIBDIR` | `$(PREFIX)/lib` |
| `PKGLIBDIR` | `$(LIBDIR)/pkgconfig` |

```sh
sudo make install
sudo ldconfig
```

This installs the static and shared libraries, the headers `include/Compages/`, EnTT and `units.h` (public headers include them), and `Compages.pc`. Nothing of `src/` is installed.

On **Fedora / RHEL**, the loader only reads `/etc/ld.so.cache`, `/lib64` and `/usr/lib64`: without an entry for `/usr/local/lib`, `libCompages.so` stays "not found" after the install. If `sudo ldconfig` is not enough:

```sh
echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/usrlocal.conf
sudo ldconfig
```

For a single run: `LD_LIBRARY_PATH=/usr/local/lib ./your-program`.

To look at the installed files without touching the system:

```sh
make DESTDIR="$PWD/stage" PREFIX=/usr install
```

Use the installed library:

```sh
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
c++ -std=c++20 app.cpp -o app $(pkg-config --cflags --libs Compages)
```

For a drawing application, `app.cpp` must create an OpenGL 4.5 context, initialize the GPU layer and open a render pass before drawing. The [complete program in chapter 3](03-Tutorial.md#-5-your-own-window-a-complete-program) shows all of these steps. A World-only application needs none of them. Include the layer you need ([chapter 8](08-API-Reference.md#-1-includes)).

**Not linked**: GLEW, SOIL, Bullet, bzip2 are not part of the build. ImGui and GLFW stay in the gallery and the tests. glad is private to the GL45 backend.

---

## 👉 6. Where to go next

- [03-Tutorial.md](03-Tutorial.md): your first three programs.
- [10-Debugging.md](10-Debugging.md): a black window, driver messages, traces.
