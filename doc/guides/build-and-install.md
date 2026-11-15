# 🔨 Build and install

[⏫ Guides](README.md)

**Goal:** build Compages with its tests, choose a build mode, and install it on your system so that another project can use it with `pkg-config`. For a first build, [tutorial 1](../tutorials/01-install.md) is shorter.

## 📦 The tools

Compages builds in C++20, on Linux, with `make` (the build system is [MyMakefile](https://github.com/Lecrapouille/MyMakefile), in `.makefile/`). The GL45 backend needs an OpenGL **4.5 core** context for the gallery and the GPU tests. The library itself does not depend on GLFW: the window belongs to your application.

| Tool | Role |
|------|------|
| g++ or clang, C++20 | compilation |
| Make, CMake | the build, and a few third-party libraries |
| Git | sources, the `.makefile` submodule, the third-party clones |
| pkg-config | `Compages.pc` |
| GLFW and an OpenGL 4.5 driver | **the gallery and the GPU tests only** |
| EGL (Linux) | `Compages-headless`: a picture drawn with no screen |

**Ubuntu / Debian**

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
    build-essential cmake git pkg-config \
    libglfw3-dev libgl1-mesa-dev libegl-dev
sudo apt-get install libgtest-dev libgmock-dev          # unit tests
sudo apt-get install xvfb mesa-utils libgl1-mesa-dri    # machine without screen (CI)
```

**Fedora**

```sh
sudo dnf install gcc-c++ make cmake git pkgconf-pkg-config \
    glfw-devel mesa-libGL-devel mesa-libEGL-devel
sudo dnf install gtest-devel gmock-devel                # unit tests
sudo dnf install xorg-x11-server-Xvfb mesa-dri-drivers  # machine without screen (CI)
```

**macOS (Homebrew)** stops at OpenGL 4.1 and cannot create the context of the GL45 backend: the gallery and the drawing tests do not run there. World-only code builds. For CPU builds the CI exports the Homebrew paths (`USER_CXXFLAGS`, `USER_LDFLAGS`, `PKG_CONFIG_PATH`): see `.github/workflows/compile.yml`.

```sh
brew install cmake glfw googletest pkg-config
```

## 🔨 Build

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/Compages.git
cd Compages
make                             # the library and the gallery
```

Cloned without submodules? `git submodule update --init --recursive` repairs it: without `.makefile/`, `make` has no rules.

The third-party libraries are not submodules. `external/external.mk` declares them; the first `make` fetches them at the commits of `external/versions.lock`, in a cache shared by your projects (`~/.cache/mymakefile`):

| Repository | Role |
|------------|------|
| `skypjack/entt` | the ECS (public headers) |
| `nholthaus/units` | angle literals such as `90.0_deg` (public header) |
| `jkuhlmann/cgltf` | reading glTF files (compilation only) |
| `nlohmann/json` | saving scenes |
| `nothings/stb` | images, fonts |
| `zeux/pugixml` | URDF files |
| `ocornut/imgui` | the panels of the gallery |
| `Lecrapouille/Compages-data` | textures, glTF and URDF files of the demos, and the screenshots of this documentation |

The demos find their assets in the cache, then in `external/Compages-data/` (a clone of yours, to regenerate the screenshots), then in the `COMPAGES_DATA_PATH` environment variable.

After the build, in `build/linux-gcc-debug/` (`make COMPILATION_MODE=release` builds in `build/linux-gcc-release/`):

| Path | Contents |
|------|----------|
| `lib/libCompages.a`, `lib/libCompages.so` | the static and shared libraries |
| `lib/pkgconfig/Compages.pc` | its flags, to build against it without installing it |
| `bin/Compages-examples` | the gallery |
| `bin/Compages-headless` | a scene drawn into a PNG with no screen (Linux) |
| `bin/Compages-UnitTest` | the unit tests |

`make help` lists the variables (`GPU_BACKEND`, `PREFIX`, …).

## 🧪 Tests

```sh
make check                                                # the unit tests
./build/linux-gcc-debug/bin/Compages-examples --check     # every demo: no frame error, no GPU handle left
```

`make check` rebuilds the test binary first; a plain `make` may leave it stale. `make coverage` gives the local coverage. On a machine without screen, see [run without a screen](run-without-a-screen.md).

## 📥 Install on your system

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

This installs both libraries, the headers `include/Compages/`, EnTT and `units.h` (the public headers include them), and `Compages.pc`. Nothing of `src/` is installed. To look at the installed files without touching the system: `make DESTDIR="$PWD/stage" PREFIX=/usr install`.

Then build against it:

```sh
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
c++ -std=c++20 app.cpp -o app $(pkg-config --cflags --libs Compages)
```

A drawing application also makes its window and its OpenGL context ([a first scene](../tutorials/02-first-scene.md#-the-same-scene-in-your-own-window)); a World-only application needs neither.

## 🧯 Common problems

- **`libCompages.so` not found after the install (Fedora, RHEL).** The loader reads only `/etc/ld.so.cache`, `/lib64` and `/usr/lib64`. Add `/usr/local/lib`: `echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/usrlocal.conf && sudo ldconfig`. For one run: `LD_LIBRARY_PATH=/usr/local/lib ./app`.
- **Not linked**: GLEW, SOIL, Bullet, bzip2 are not part of the build. ImGui and GLFW stay in the gallery and the tests; glad is private to the GL45 backend.

## 🔗 See also

[Run without a screen](run-without-a-screen.md) · [A black window](debug-a-black-window.md) · [Architecture](../concepts/architecture.md)
