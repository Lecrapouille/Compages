# 🌍 Build for macOS and the web

[⏫ Guides](README.md)

Your program runs on Linux, and you want it on a Mac, or in a web page.

<img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/guides/web-gallery.png" alt="The gallery in Chromium, drawn with WebGL 2" width="640">

macOS stops at OpenGL 4.1, and a browser gives WebGL 2, which is OpenGL ES 3.0. Compages draws with OpenGL 4.5 by default. Two other backends exist for these places. You choose one when you build, with `GPU_BACKEND`. Your C++ and your shaders do not change: shaders stay written for `#version 450 core`, and the backend translates them ([architecture](../concepts/architecture.md#-the-backend-boundary)).

## 🍎 macOS: OpenGL 4.1

On a Mac, `make` picks `GPU_BACKEND=GL41` itself. Ask GLFW for the context this backend needs, as `examples/Common/Window.cpp` does:

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
// macOS gives a core profile only to a forward compatible context.
glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
```

The library defines `COMPAGES_GPU_BACKEND_GL41` for your code, so one window file can serve every backend.

You can try this backend on Linux before you own a Mac. The build goes into its own folder:

```bash
make BUILD_DIR=build-gl41 GPU_BACKEND=GL41
./build-gl41/linux-gcc-debug/bin/Compages-examples --check
```

## 🌐 The web: WebGL 2

Install [emsdk](https://emscripten.org/docs/getting_started/downloads.html), then build with its compiler. `make` then picks `GPU_BACKEND=GLES3`:

```bash
source ~/emsdk/emsdk_env.sh
make TOOLCHAIN=emscripten
cd build/emscripten-debug/bin
python3 -m http.server 8000      # then open http://localhost:8000/Compages-examples.html
```

The page needs a web server: a browser refuses to load the `.wasm` and `.data` files from `file://`. The `.data` file holds the textures and models of the demos (about 60 MB).

Your application needs three things that a desktop program does not:

- A WebGL 2 context. With the GLFW of Emscripten, link with `-sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2`, and ask GLFW for `GLFW_OPENGL_ES_API`, version 3.0.
- A frame loop that gives the browser its turn. The gallery keeps its `while` loop: it links with `-sASYNCIFY` and calls `emscripten_sleep(0)` after `glfwSwapBuffers()`. `emscripten_set_main_loop()` is the other way, with no ASYNCIFY cost.
- Its files in the page: `--preload-file` copies a folder into the memory file system, at the path you give.

`examples/examples.mk` has the whole list of flags. Threads need `-pthread` and a server that sends the COOP and COEP headers; the gallery builds without them, so the Archipelago demo computes its tiles in the frame.

You can also try this backend natively, with an OpenGL ES 3.0 context and `libGLESv2`:

```bash
make BUILD_DIR=build-gles3 GPU_BACKEND=GLES3
./build-gles3/linux-gcc-debug/bin/Compages-examples --check
```

## 🚧 What these backends cannot do

Compute shaders, storage blocks and images need OpenGL 4.3 or OpenGL ES 3.1. On `GL41` and `GLES3`, `gpu::device().compute` is false. A compute shader fails to load, with a message that says why. Check the flag and keep another path:

```cpp
if (!compages::gpu::device().compute)
{
    // Do the step on the CPU, or in a fragment shader with a TexturePingPong.
}
```

In your shaders, `COMPAGES_NO_STORAGE` is defined on these backends, for code that must do without a storage block.

The Renderer works, with less:

- only the four lamps with shadows light the scene: the other lamps use storage blocks (clustered lamps);
- no volumetric fog (two compute passes); the distance fog stays;
- the field views of `Scene` (`11_Science`) read a storage buffer and fail to load.

OpenGL ES adds a few limits of its own: no 1D textures (use a 2D texture one pixel high), no wireframe (`RenderState::polygon` draws filled triangles), no geometry stage (`device().geometry_shaders`), no GPU timer, and no implicit conversion from `int` to `float` in GLSL (`1.0`, not `1`).

In the gallery, 57 of 74 demos run on `GL41` and 56 on `GLES3`. The others are the compute demos and the field views of `11_Science`.

## 🔗 See also

- [The backend boundary](../concepts/architecture.md#-the-backend-boundary): how the three backends differ.
- [Compute on the GPU](compute-on-the-gpu.md): what needs `device().compute`.
- [Build and install](build-and-install.md): the build options of `make`.
