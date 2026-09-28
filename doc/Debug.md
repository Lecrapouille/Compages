# Debugging

## Driver Messages

The `compages::gpu` layer installs a `KHR_debug` callback when available (OpenGL 4.3+). Warnings and errors from the driver show up on **stderr**:

```text
[gpu] warning: …
[gpu] error: …
```

In the example gallery, messages are also found in the console, and can be filtered by log level. A driver error often appears alongside a failed `Result` or a `takeFrameError()` call higher up in your code.

## Library Messages

| When                              | Where to read |
|------------------------------------|--------------|
| Shader compilation error or missing file | The returned `compages::Result`, usually via `COMPAGES_TRY` |
| Unknown uniform name, wrong type, empty draw | `compages::gpu::takeFrameError()` (returns an empty string if there was no error) |
| Before your loop starts            | `drawable.prepare()` or `scene.prepare()` |

Calling `setBreakOnError(true)` will trigger an assertion on the first recorded error, which is useful for breaking into the debugger at the exact line of failure. (Has no effect if assertions are disabled in your build.)

The example gallery also shows the error message as an overlay. An example using `compages::gpu::reportError(...)` will cause the `--check` to fail: this is how the `30_HeadlessWorld` example reports that a system didn't move the player.

## apitrace

To see the actual OpenGL calls:

```sh
apitrace trace --api gl ./build/Compages-examples 04_DepthAndTransforms
qapitrace Compages-examples.trace
```

If a trace with the same name already exists, apitrace will write `.1.trace`, `.2.trace`, etc. Project: [apitrace](https://github.com/apitrace/apitrace).

## RenderDoc

Capture individual frames: shaders, render targets, draw calls. Target: `build/Compages-examples`.

The ImGui overlay uses its own backend **after** the Compages flush. `gpu::forgetRenderState()` ensures that the render state left by ImGui doesn't hide the demo's state. The demo itself remains fully inspectable.

## Reproducing the CI

Without a display, use the same environment variables as [Install.md](Install.md):

```sh
xvfb-run -a env \
    LIBGL_ALWAYS_SOFTWARE=1 \
    MESA_GL_VERSION_OVERRIDE=4.5 \
    MESA_GLSL_VERSION_OVERRIDE=450 \
    ./build/Compages-examples --check
```

## See also

- [GPU.md](GPU.md) — covers flush semantics and barriers.
- [CheatSheet.md](CheatSheet.md) — `prepare`, `takeFrameError`.
