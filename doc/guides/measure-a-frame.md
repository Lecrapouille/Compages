# Measure a frame

[Guides](README.md)

Know where the time of a frame goes, CPU or GPU, and what the GPU was asked to do.

![09b_ManyCubes](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/09b_ManyCubes.png)

## CPU or GPU?

The CPU *sends* the work (extraction, sorting, uniforms, draw calls); the GPU *does* it, later, in parallel. A slow frame is slow on one side or the other, and the cure is not the same.

```cpp
compages::gpu::beginGpuTimer();
scene.draw(frame);
compages::gpu::endGpuTimer();
// later, any frame:
double gpu = compages::gpu::gpuMilliseconds();
```

The timer is read a few frames later, without making the CPU wait for the GPU. Measure the CPU side yourself with `std::chrono` around the same call. The gallery does both and shows them in its status bar and inspector: **CPU** is the time spent in the demo's `draw()`, **GPU** the time the card spent on it.

| CPU high, GPU low | GPU high, CPU low |
|-------------------|-------------------|
| too many draw calls: share meshes and looks (`09b_ManyCubes`), one draw with data per instance (`09a_SpriteBatch`), indirect draws (`09c_IndirectDraw`) | heavy shaders or too many pixels: fewer lamps with shadows, a smaller fog volume, a lower resolution |

Compare the same demo before and after a change, on the same machine. llvmpipe (Mesa without a GPU) gives times, not a measure of a card.

## What the GPU was asked to do

```cpp
compages::gpu::resetFrameStatistics();
scene.draw(frame);
compages::gpu::FrameStatistics const stats = compages::gpu::frameStatistics();
// stats.draw_calls, stats.passes, stats.dispatches, stats.vertices, stats.instances
```

The gallery shows these counters too. `09b_ManyCubes` (1,728 cubes) shows the draw calls fall as culling removes what the camera does not see.

## See also

[One frame](../concepts/one-frame.md) · [A black window](debug-a-black-window.md)
