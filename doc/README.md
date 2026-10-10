# Reading Compages

This documentation is a journey. Start with your intent, write three small programs, then dive into the layer you need. Foundational files (architecture, cheat sheet, debugging) are most useful when you’re looking for a specific fact, not when discovering the project for the first time.

## Guide

1. [README](../README.md) — introduction, three code snippets, quick install.
2. [Philosophy.md](Philosophy.md) — why there are three layers, and what each deliberately does not do.
3. [Tutorial.md](Tutorial.md) — the three programs, explained line by line.
4. Open the layer relevant for your use case:
   - [GPU.md](GPU.md) — shaders, flushing, compute.
   - [World.md](World.md) — graph, ECS, articulations, without a screen.
   - [Renderer.md](Renderer.md) — `Scene`, files, image rendering.

From here, the order is up to you.

| Need                        | Document                        |
|-----------------------------|---------------------------------|
| Quick function reference    | [CheatSheet.md](CheatSheet.md)  |
| Pick a demo to read         | [Examples.md](Examples.md)       |
| Paint on a live texture     | [CanvasTexture.md](CanvasTexture.md) |
| Explore the `src/` codebase | [Architecture.md](Architecture.md) |
| Build on Ubuntu or Fedora, install | [Install.md](Install.md)  |
| Debug black frame, GL trace | [Debug.md](Debug.md)            |
| Discover the inspirations   | [Credits.md](Credits.md)        |

## One sentence per layer

- **GPU** — I write a shader; I set data by the names declared in it; `draw()` sends any changes.
- **World** — I name entities, parent them, attach structs, and step simulations, all without the GPU.
- **Renderer** — I call `scene.box` and `scene.draw`; the asset catalog and world snapshot bridge to the GPU.

## Images

Six screenshots are included in `doc/images/` so the README renders correctly without a separate asset repository. The complete gallery can be regenerated from the asset repo with:

```sh
./build/Compages-examples --check --shots external/Compages-data/doc/examples
```

An OpenGL 4.5 context is required. For headless usage, see [Install.md](Install.md), headless section.
