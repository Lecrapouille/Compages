# 📚 Compages documentation

Read the chapters in order: each one uses the words of the previous ones. Chapters 1 to 3 are for everybody, even without any 3D or OpenGL background. Then open the chapter of the layer you need (4, 5 or 6). Chapters 7 to 11 are references.

![The three layers](logos/compages-layers.png)

## 🎓 Learn

| # | Chapter | You will learn | Time |
|---|---------|----------------|------|
| 1 | [Introduction](01-Introduction.md) | 3D words (shader, buffer, entity, component…), the three layers and why they are separate | 15 min |
| 2 | [Getting started](02-Getting-Started.md) | Install, build, run the gallery, run without a screen | 20 min |
| 3 | [Your first programs](03-Tutorial.md) | Three small programs, one per layer, explained line by line | 30 min |

## 🧱 The three layers

| # | Chapter | Read it when… |
|---|---------|---------------|
| 4 | [The GPU layer](04-GPU-Layer.md) | you have shaders and arrays: draw, compute, several passes |
| 5 | [The World layer](05-World-Layer.md) | you have things that move: entities, ECS, joints, behaviors, no screen |
| 6 | [The Renderer layer](06-Renderer-Layer.md) | you want a picture of the world: scene, camera, lights, files, painted textures |

## 📖 Reference

| # | Chapter | Contents |
|---|---------|----------|
| 7 | [Architecture](07-Architecture.md) | Folders, who includes whom, one frame from start to end |
| 8 | [API reference](08-API-Reference.md) | The calls of the three layers on one page |
| 9 | [Examples](09-Examples.md) | The gallery: every demo, its idea, its keys |
| 10 | [Debugging](10-Debugging.md) | Error messages, driver messages, apitrace, RenderDoc, CI without a screen |
| 11 | [Credits](11-Credits.md) | History, inspirations, third-party libraries |

## 🧭 Quick paths

- *"I just want to see something."* → [2. Getting started](02-Getting-Started.md), then `./build/Compages-examples`
- *"I have a shader and numbers."* → [3. Tutorial §1](03-Tutorial.md#-1-a-triangle-named-by-the-shader), then [4. GPU](04-GPU-Layer.md)
- *"I simulate things, no window needed."* → [3. Tutorial §2](03-Tutorial.md#-2-a-world-without-a-screen), then [5. World](05-World-Layer.md)
- *"I want a 3D scene in ten lines."* → [3. Tutorial §3](03-Tutorial.md#-3-a-picture-of-the-world), then [6. Renderer](06-Renderer-Layer.md)
- *"My window is black."* → [10. Debugging](10-Debugging.md)
- *"I want to change Compages itself."* → [7. Architecture](07-Architecture.md), and [AGENTS.md](../AGENTS.md) for the conventions

## 📷 Images

Five screenshots live in `doc/images/` so that the pages render without the assets repository. The full gallery is written into the [Compages-data](https://github.com/Lecrapouille/Compages-data) repository by:

```sh
./build/Compages-examples --check --shots external/Compages-data/doc/examples
```

## 🌍 A note on the language

This documentation is written in simple English, with short sentences and common words, so that readers whose first language is not English (French readers in particular) can follow it easily.
