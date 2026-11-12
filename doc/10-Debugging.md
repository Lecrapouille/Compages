# 🐞 10. Debugging

A black window, a shader that compiles but shows nothing, a demo that fails `--check`: this page lists where Compages reports problems and which tools show what the GPU really received.

---

## 🧭 0. Start with the smallest check

Work through these checks in order. You usually do not need a GPU capture tool for a first error.

| What happened? | First check | Next action |
|----------------|-------------|-------------|
| The build stops before making a binary | Read the first compiler or download error | Check the packages and submodules in [Getting started](02-Getting-Started.md) |
| No window opens | Read stderr, the terminal error output | Check that a display and an OpenGL 4.5 driver are available |
| The gallery works, but your application is black | Check `prepare()`, the current context and the open `RenderPass` | Compare your loop with [the complete program](03-Tutorial.md#-5-your-own-window-a-complete-program) |
| A shape is missing | Check camera position, object position and scale, then light | Try a known shape at the origin before loading a model |
| A file cannot be read | Check the exact path from the program's working directory | Keep a model beside its textures; use an absolute path to isolate a path problem |
| Movement is twice as fast as expected | Count simulation updates in one frame | Use `scene.draw(frame)` or `scene.update(frame)` plus renders; do not add another World update |

Change one thing at a time and rerun the smallest demo that reproduces the problem. When asking for help, include the command you ran, the first full error message, your OS and driver, and the smallest code example that fails.

## 📨 1. Messages of the library

| When | Where to read it |
|------|------------------|
| a shader does not compile, a file is missing | the returned `compages::Result`, often through `COMPAGES_TRY` |
| unknown uniform name, wrong type, empty draw | `compages::gpu::takeFrameError()` (empty string when nothing happened) |
| before your loop starts | `drawable.prepare()` or `scene.prepare()` |

- Only the first frame error is kept; `frameErrorCount()` counts the others.
- `setBreakOnError(true)` asserts on the first recorded error, so that the debugger stops on the faulty line (no effect when assertions are disabled in your build).
- The gallery shows the message over the demo. A demo that calls `compages::gpu::reportError(...)` fails `--check`: this is how `07a_HeadlessWorld` reports that the player did not move.

---

## 🚗 2. Messages of the driver

The GPU layer installs a `KHR_debug` callback when the driver has it (OpenGL 4.3 and more). Warnings and errors of the driver go to **stderr**:

```text
[gpu] warning: …
[gpu] error: …
```

In the gallery they also appear in the console panel, filtered by level. A driver error often comes with a failed `Result` or a `takeFrameError()` higher in your code.

---

## 🔍 3. apitrace: every OpenGL call

```sh
apitrace trace --api gl ./build/linux-gcc-debug/bin/Compages-examples 03c_DepthAndTransforms
qapitrace Compages-examples.trace
```

When a trace of the same name exists, apitrace writes `.1.trace`, `.2.trace`… Project: [apitrace](https://github.com/apitrace/apitrace).

---

## 📸 4. RenderDoc: one frame in detail

Capture single frames: shaders, render targets, draw calls. Target: `build/linux-gcc-debug/bin/Compages-examples`.

The ImGui panels use their own backend **after** the Compages flush. `gpu::forgetRenderState()` makes sure the state left by ImGui does not hide the state of the demo: the demo stays fully inspectable.

---

## 🤖 5. Reproduce the CI

Without a screen, use a software OpenGL ([chapter 2 §4](02-Getting-Started.md#-4-tests-and-a-machine-without-screen)):

```sh
xvfb-run -a env \
    LIBGL_ALWAYS_SOFTWARE=1 \
    MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 \
    ./build/linux-gcc-debug/bin/Compages-examples --check
```

---

## 🧯 6. Common problems

| Symptom | Look at |
|---------|---------|
| black window, no message | `prepare()` result; `takeFrameError()` after the first frame; a missing `draw()` or `clear` only |
| particles flicker or lag one frame | a missing `barrier(...)` after `dispatchItems` ([chapter 4 §5](04-GPU-Layer.md#-5-compute)) |
| the demo panel looks wrong after your ImGui code | call `gpu::forgetRenderState()` after ImGui |
| `libCompages.so` not found after install (Fedora) | `/usr/local/lib` missing from the loader ([chapter 2 §5](02-Getting-Started.md#-5-install-on-your-system)) |
| textures missing | In the gallery, check downloaded assets or `COMPAGES_DATA_PATH`; in your own application, check the file paths you supply |

## ✅ Before returning to your program

You should know whether the failure happens during build, setup or a frame. Confirm it with the first useful message, then rerun after the fix. A passing `--check NAME` confirms the gallery demo reports no GPU error or leaked handle; it does not prove that a simulation equation is correct.

Return to [your learning path](README.md), or read [Credits](11-Credits.md) for the ideas and sources behind the library.
