# 🧭 Guides

[⏫ Documentation](../../README.md#-documentation)

Each guide helps you complete a task in an existing application. They assume the basics from the [tutorials](../tutorials/README.md). For the reasons behind a choice, read the linked [concepts](../concepts/README.md) or effect-specific [techniques](../techniques/README.md).

Set up and check:

- [Build and install](build-and-install.md): the unit tests, a release build, a system install for another project.
- [Build for macOS and the web](build-for-macos-and-the-web.md): the OpenGL 4.1 and WebGL 2 backends, and what they lack.
- [Run without a screen](run-without-a-screen.md): a simulation on a server, a picture into a file, the gallery in CI.
- [A black window](debug-a-black-window.md): the messages of Compages and of the driver, apitrace, RenderDoc.
- [Measure a frame](measure-a-frame.md): is the CPU or the GPU slow?

Build a scene:

- [Load a model](load-a-model.md): glTF, STL, URDF, animations, copies, saving.
- [Light the scene](light-the-scene.md): configure lights, exposure, shadows and fog; effect algorithms are in Techniques.
- [Move the camera](move-the-camera.md): orbit, fly, drag a map, fly to a place.
- [Several views](several-views.md): a split screen, a minimap, a picture read by the program.
- [Draw helpers](draw-helpers.md): grids, axes, debug lines, hidden from a robot camera.
- [Pick with the mouse](pick-with-the-mouse.md): what is under the mouse.
- [Text on the picture](text-on-the-picture.md): labels, tooltips, captions.
- [Very large scenes](very-large-scenes.md): a metre and the Moon in one picture.

Change how things look:

- [Write a material](write-a-material.md): the hooks `surface`, `light`, `main` and `deform` of the PBR shader.
- [See-through things](see-through-things.md): glass, water.
- [Make pictures and meshes](make-pictures-and-meshes.md): a texture from computed pixels, a shape from triangles.
- [Paint a texture](paint-a-texture.md): a picture that changes while the program runs.
- [Screen effects](screen-effects.md): filters on the picture of a view, bloom.

Compute and simulate:

- [Show a field](show-a-field.md): colours, relief, arrows, volumes, curves, the value under the mouse.
- [Write routines](write-routines.md): code on one entity, fixed steps, timers, messages.
- [Plug a physics engine](plug-a-physics-engine.md): MuJoCo, Bullet or Jolt move the entities.
- [Compute on the GPU](compute-on-the-gpu.md): compute shaders, ping-pong, barriers.
- [Draw in several passes](draw-in-several-passes.md): render to a texture and reuse one mesh across drawing passes.
- [Share and reload shader source](manage-shader-sources.md): GLSL includes, shared definitions, compiler messages and hot reload.
