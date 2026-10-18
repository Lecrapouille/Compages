# 🚀 2. Getting started

In this page you install the tools, build Compages, open the gallery of demos, run it on a machine without a screen, and install the library on your system. It takes about 20 minutes (most of it is compilation).

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

### Ubuntu / Debian

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
    build-essential cmake git pkg-config \
    libglfw3-dev libgl1-mesa-dev
sudo apt-get install libgtest-dev libgmock-dev          # unit tests
sudo apt-get install xvfb mesa-utils libgl1-mesa-dri    # machine without screen (CI)
```

### Fedora

```sh
sudo dnf install gcc-c++ make cmake git pkgconf-pkg-config \
    glfw-devel mesa-libGL-devel
sudo dnf install gtest-devel gmock-devel                # unit tests
sudo dnf install xorg-x11-server-Xvfb mesa-dri-drivers  # machine without screen (CI)
```

### macOS (Homebrew)

macOS stops at OpenGL 4.1. The 4.5 backend is not complete there: the gallery and the GPU tests are limited; the tests that do not touch the GPU pass.

```sh
brew install cmake glfw googletest pkg-config
```

The CI then exports the Homebrew paths (`USER_CXXFLAGS`, `USER_LDFLAGS`, `PKG_CONFIG_PATH`): see `.github/workflows/compile.yml`.

---

## 🔨 2. Build

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make download-external-libs      # once: clones the third parties of external/manifest
make compile-external-libs       # once: builds the ones that need it
make -j"$(nproc)"                # the library and the gallery
```

`--recurse-submodules` fetches `.makefile`, the build system. Without it, `make` has no rules.

The third-party libraries are not git submodules. `external/manifest` lists them, one GitHub repository per line:

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
| `DanielChappuis/reactphysics3d` | `attic/Physics/` only |

The assets land in `external/Compages-data/`. The demos also look at the `COMPAGES_DATA_PATH` environment variable.

After the build:

| Path | Contents |
|------|----------|
| `build/libCompages.a` | the static library |
| `build/libCompages.so` | the shared library |
| `build/Compages-examples` | the gallery |

`make help` lists the variables (`GPU_BACKEND`, `PREFIX`, …). `GPU_BACKEND` is `GL45` in `Makefile.common`. The day a second backend exists, you will change it there, not in the code that calls `gpu::`.

---

## 📷 3. Open the gallery

```sh
./build/Compages-examples                 # every demo, with panels
./build/Compages-examples 13_Galaxy       # start on one demo
```

One window holds a viewport, the list of demos, an inspector, the source of the demo, a console and GPU counters. **PgUp / PgDn** change demo, **F1** hides the panels, **Esc** quits. All keys: [chapter 9](09-Examples.md#-1-the-window).

![Galaxy: eight thousand stars in a compute shader](images/13_Galaxy.png)

Check that every demo builds, draws and frees its GPU objects:

```sh
./build/Compages-examples --check
```

`--check` runs each demo for a few frames, closes it, and fails if a GPU handle is left.

---

## 🧪 4. Tests, and a machine without screen

```sh
make tests -j"$(nproc)"
```

The tests binary is `build/Compages-UnitTest`. Local coverage: `make -C tests coverage`.

On a Linux machine without screen (CI, server), give a software OpenGL to the program:

```sh
xvfb-run -a env \
    LIBGL_ALWAYS_SOFTWARE=1 \
    MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 \
    ./build/Compages-examples --check
```

The same prefix works for `./build/Compages-UnitTest`. A program that only uses the **World** layer needs none of this: it never touches the GPU ([chapter 3 §2](03-Tutorial.md#-2-a-world-without-a-screen)).

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

In `app.cpp`: create your OpenGL 4.5 context, call `compages::gpu::init(yourGetProcAddress)`, and include the layer you need ([chapter 8](08-API-Reference.md#-1-includes)).

**Not linked**: GLEW, SOIL, Bullet, bzip2 are not part of the build. ImGui and GLFW stay in the gallery and the tests. glad is private to the GL45 backend.

---

## 👉 6. Where to go next

- [03-Tutorial.md](03-Tutorial.md): your first three programs.
- [10-Debugging.md](10-Debugging.md): a black window, driver messages, traces.
