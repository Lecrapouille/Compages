# 🧭 Guides: one task each

[⏫ Documentation](../README.md)

A guide answers "how do I…?" for one task, with the code and the result. It assumes you finished the [tutorials](../tutorials/README.md), or at least the first two. Why things work this way is in the [concepts](../concepts/README.md); every call is in the [reference](../reference/README.md).

## 🛠 Set up and check

| Guide | When you want to… |
|-------|-------------------|
| [Build and install](build-and-install.md) | build the tests, a release, install Compages for another project |
| [Run without a screen](run-without-a-screen.md) | simulate on a server, write a picture into a file, run the gallery in CI |
| [A black window](debug-a-black-window.md) | find why nothing shows: messages, apitrace, RenderDoc |
| [Measure a frame](measure-a-frame.md) | know whether the CPU or the GPU is slow |

## 🟩 Build a scene

| Guide | When you want to… |
|-------|-------------------|
| [Load a model](load-a-model.md) | load glTF, STL, URDF, play animations, copy, save |
| [Light the scene](light-the-scene.md) | sun, lamps, shadows, exposure, hundreds of lamps, volumetric fog |
| [Move the camera](move-the-camera.md) | orbit, fly, drag a map, fly to a place |
| [Several views](several-views.md) | a split screen, a minimap, a picture read by the program |
| [Draw helpers](draw-helpers.md) | grids, axes, bounds, debug lines, and hide them from a sensor |
| [Pick with the mouse](pick-with-the-mouse.md) | know what is under the mouse |
| [Text on the picture](text-on-the-picture.md) | labels, tooltips, captions |
| [Very large scenes](very-large-scenes.md) | a metre and the Moon in one picture |

## 🎨 Change how things look

| Guide | When you want to… |
|-------|-------------------|
| [Write a material](write-a-material.md) | your own shading with the hooks `surface`, `light`, `main`, `deform` |
| [See-through things](see-through-things.md) | glass, water, a ghost |
| [Make pictures and meshes](make-pictures-and-meshes.md) | a texture from computed pixels, a shape from triangles |
| [Paint a texture](paint-a-texture.md) | a picture that changes while the program runs |
| [Screen effects](screen-effects.md) | filters of the picture of a view, bloom |

## 🧮 Compute and simulate

| Guide | When you want to… |
|-------|-------------------|
| [Show a field](show-a-field.md) | colours, relief, arrows, volumes, curves, the value under the mouse |
| [Write routines](write-routines.md) | behaviour on one entity: fixed steps, timers, messages |
| [Plug a physics engine](plug-a-physics-engine.md) | let MuJoCo, Bullet or Jolt move the entities |
| [Compute on the GPU](compute-on-the-gpu.md) | particles, cells, stars: compute shaders, ping-pong, barriers |
| [Draw in several passes](draw-in-several-passes.md) | render to a texture, one mesh and several shaders, shaders in pieces, hot reload |

For one visual effect explained from the idea to its cost, see the [techniques](../techniques/README.md).
