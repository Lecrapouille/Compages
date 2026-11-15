# 🐞 A black window

[⏫ Guides](README.md)

Find why nothing shows, or why something is wrong, from the smallest check to a capture of a whole frame.

## 🧭 Start with the smallest check

Work through these in order. You rarely need a GPU capture tool for a first error.

| What happened? | First check | Next action |
|----------------|-------------|-------------|
| The build stops | the first compiler or download error | the packages and submodules of [build and install](build-and-install.md) |
| No window opens | stderr | a display and an OpenGL 4.5 driver |
| The gallery works, your program is black | `prepare()`, the current context, the open `RenderPass` | compare your loop with [the complete program](../tutorials/04-a-scene.md#-the-same-scene-in-your-own-window) |
| A shape is missing | the camera position, the object position and scale, then the light | try a known shape at the origin before loading a model |
| A file cannot be read | the path from the working directory of the program | keep a model beside its textures; try an absolute path |
| Things move twice as fast | count the updates in one frame | `scene.draw(frame)`, or `scene.update(frame)` plus renders; never `world.update` as well |

Change one thing at a time, and rerun the smallest demo that shows the problem. When you ask for help, give the command, the first full error message, the OS and driver, and the smallest code that fails.

## 📨 The messages of Compages

| When | Where to read it |
|------|------------------|
| a shader does not compile, a file is missing | the returned `Status` / `Result`, often through `COMPAGES_TRY` |
| unknown name, wrong type, empty draw | `gpu::takeFrameError()` (empty when nothing happened) |
| before the loop | `drawable.prepare()` or `scene.prepare()` |

Only the first frame error is kept; `frameErrorCount()` counts the others. `gpu::setBreakOnError(true)` asserts on the first one, so a debugger stops on the faulty line. The gallery writes the message over the demo. Why errors come at two moments: [errors](../concepts/errors.md).

## 🚗 The messages of the driver

The GPU layer installs a `KHR_debug` callback when the driver has it. Its warnings and errors go to stderr, and to the console of the gallery:

```text
[gpu] warning: …
[gpu] error: …
```

A driver error often comes with a failed `Result` or a `takeFrameError()` higher in your code.

## 🔍 Every OpenGL call: apitrace

```sh
apitrace trace --api gl ./build/linux-gcc-debug/bin/Compages-examples 03c_DepthAndTransforms
qapitrace Compages-examples.trace
```

[apitrace](https://github.com/apitrace/apitrace) records every call; `qapitrace` replays them one by one, with the state and the pictures.

## 📸 One frame in detail: RenderDoc

[RenderDoc](https://renderdoc.org/) captures single frames: shaders, targets, draw calls. Target: `build/linux-gcc-debug/bin/Compages-examples`. The panels of the gallery use their own ImGui backend after the demo; `gpu::forgetRenderState()` keeps the state ImGui leaves from hiding the state of the demo, so the demo stays inspectable.

## 🧯 Common problems

| Symptom | Look at |
|---------|---------|
| black window, no message | `prepare()`; `takeFrameError()` after the first frame; a missing `draw()` |
| particles flicker or lag a frame | a missing `barrier(…)` after a dispatch ([compute on the GPU](compute-on-the-gpu.md)) |
| the panel of a demo looks wrong after your ImGui code | `gpu::forgetRenderState()` after ImGui |
| far faces flicker over each other | depth precision: [very large scenes](very-large-scenes.md) |
| textures missing | in the gallery, the downloaded assets or `COMPAGES_DATA_PATH`; in your program, the paths you give |

A passing `--check NAME` says the demo has no GPU error and leaves no handle; it does not prove a simulation is right.

## 🔗 See also

[◀ Errors](../concepts/errors.md) · [Measure a frame](measure-a-frame.md) · [Run without a screen](run-without-a-screen.md)
