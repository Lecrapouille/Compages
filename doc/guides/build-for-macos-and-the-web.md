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

The page needs a web server: a browser refuses to load the `.wasm` and `.data` files from `file://`. The `.data` file holds the textures and models of the demos (about 48 MB). `?args=05f_Galaxy` in the address opens one demo first. `COMPILATION_MODE=release` makes a smaller and faster page: the one the CI publishes.

The CI publishes this page on [GitHub Pages](https://lecrapouille.github.io/Compages/) at each push on `master`. Before that, it runs a few demos in a headless Chrome, with `examples/Web/check.mjs`:

```bash
python3 -m http.server -d build/emscripten-release/bin 8080 &
node examples/Web/check.mjs "http://127.0.0.1:8080/Compages-examples.html?args=--check 05f_Galaxy" 300
```

The page itself is `examples/Web/shell.html`: a canvas that fills the window, and a line that says what is loading.

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

## 🚧 What these backends do differently

**Compute is emulated.** OpenGL 4.1 and WebGL 2 have no compute shader. Your compute shaders still run, unchanged: the backend rewrites each one into a vertex shader whose outputs are written into the buffers. It is several times slower, and only for shaders where each invocation writes its own element. `shared` memory goes through `<compages/tiles.glsl>`; atomics and images are refused with a message. [Compute on the GPU](compute-on-the-gpu.md#-on-macos-and-in-the-browser) gives the rules and the measured cost. `gpu::device().compute_emulated` tells your program.

**Storage blocks are read from copies.** A vertex or fragment shader that reads a storage block works unchanged: the backend reads a copy of the buffer kept in a texture, made again when the buffer changed. So the clustered lamps of the Renderer and the field views of `Scene` (`11_Science`, `13b` to `13d`) work. A drawing shader that writes a storage block is refused: that needs OpenGL 4.3 everywhere.

**No images.** No shader loads or stores images (`gpu::device().images` is false). The Renderer then draws without its volumetric fog (two compute passes that write images); the distance fog stays.

OpenGL ES adds a few limits of its own: no 1D textures (use a 2D texture one pixel high), no wireframe (`RenderState::polygon` draws filled triangles), no geometry stage (`device().geometry_shaders`), no GPU timer, and no implicit conversion from `int` to `float` in GLSL (`1.0`, not `1`).

In the gallery, 72 of 74 demos run on `GL41` and on `GLES3`, with the same picture as `GL45` but for the volumetric fog (and, on `GLES3`, the wireframe pass of `04c`). The two others need real compute: `05g_NeuralNetwork` (one invocation writes all the gradients of its sample) and `09c_IndirectDraw` (`atomicAdd`, indirect draw). The CI checks this list (`.github/gallery-expected-failures.txt`) on both backends.

## 🔗 See also

- [The backend boundary](../concepts/architecture.md#-the-backend-boundary): how the three backends differ.
- [Compute on the GPU](compute-on-the-gpu.md): `shared` memory, and compute on macOS and the web.
- [Build and install](build-and-install.md): the build options of `make`.
