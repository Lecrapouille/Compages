# 🚨 Errors

[⏫ Concepts](README.md)

Some mistakes are found once: a shader that does not compile, a file that does not exist. Others could happen at every frame: a uniform name with a typo, a `vec3` given two numbers, a draw with no vertices. Throwing an exception sixty times a second would stop a simulation for a typo in a colour; checking every assignment of every frame would slow it down; staying silent gives a black window and no clue.

## ✌ Two moments

| When | What happens | Where to read it |
|------|--------------|------------------|
| **Load**: a shader, a file, a material | the call fails at once and returns why | the `compages::Status` or `Result<T>` it returns |
| **Frame**: an unknown name, a wrong type, an empty draw | the first message is kept, the next ones are counted | `gpu::takeFrameError()`, and `prepare()` before the loop |

**At load**, every call that can fail for a reason outside the program returns a `Status` (success or a message) or a `Result<T>` (a value or a message). Two macros keep the code short in a function that itself returns a `Status`:

```cpp
COMPAGES_TRY(drawable.loadSources(vertex, fragment));       // returns the error, if any
COMPAGES_TRY_ASSIGN(m_target, RenderTarget::create(640u, 480u)); // stores the value, or returns the error
```

In `main()`, which returns an `int`, test it: `if (!result) { std::cerr << result.error(); }`.

**At each frame**, nothing is thrown. The first error is recorded, with a message that names the call and lists what the shader really declares; `frameErrorCount()` says how many came after. Read it once per frame:

```cpp
if (const std::string error = compages::gpu::takeFrameError(); !error.empty())
{
    std::cerr << error << '\n';
}
```

**`prepare()`** runs the per-frame checks once, at the end of the setup: `drawable.prepare()` or `scene.prepare()` return the first problem as a `Status`, before the loop starts. Most frame errors are found there.

## 🛠 Tools

- `gpu::setBreakOnError(true)` asserts on the first recorded error, so a debugger stops on the faulty line.
- `gpu::reportError("…")` records your own error the same way. A gallery demo that does it fails `--check`: this is how `07a_HeadlessWorld` says that its player did not move, with no picture to look at.
- The driver's own warnings (through `KHR_debug`) go to stderr as `[gpu] warning: …` and to the console of the gallery.

## 🚨 Only the first error

Only the first frame error is kept: a second, unrelated error waits until the first is fixed. In return, a broken frame never stops the program, and the message comes back where you read it, not from deep in a driver.

## 🔗 See also

- [A black window](../guides/debug-a-black-window.md): the checks to run, in order, and apitrace, RenderDoc.
- [Reference: errors](../reference/api.md#-2-errors).
