# 📚 Compages documentation

![The robot workshop of the tutorials](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/00e_RobotEye.png)

You do not need to know 3D or OpenGL. You need basic C++: variables, functions, structs, a loop, a lambda. Every 3D word is in the [glossary](concepts/glossary.md).

## 🧭 Choose a path

### 🚶 Use Compages

You want a program that shows a 3D scene: objects, light, a camera, things that move. Follow the **[tutorials](tutorials/README.md)**: they build a robot workshop step by step, from a table to an arm that sees, and each page ends with something new on the screen. Then pick the [guides](guides/README.md) you need: load a model, light the scene, move the camera, several views.

### 🎨 Make effects and simulations

You want a look of your own, a field computed on a grid, water, fog, a fluid. After the first tutorials, read the **[techniques](techniques/README.md)** (one effect per page: the idea, the lines that switch it on, the shader, the cost) and the **[case studies](case-studies/README.md)** (whole demos, from the equation or the paper to the picture). The guides [write a material](guides/write-a-material.md), [show a field](guides/show-a-field.md) and [compute on the GPU](guides/compute-on-the-gpu.md) are the tools.

### 🔧 Understand and extend the engine

You want to know why Compages is built this way, or to change it. Read the **[concepts](concepts/README.md)**, starting with [three layers](concepts/three-layers.md), then [architecture](concepts/architecture.md), and [AGENTS.md](../AGENTS.md) for the conventions of the code.

## 🗂 The four kinds of page

| Folder | Kind | You read it to… |
|--------|------|-----------------|
| [tutorials/](tutorials/README.md) | tutorial | learn by doing, in order |
| [guides/](guides/README.md) | how-to | do one task now |
| [techniques/](techniques/README.md), [case-studies/](case-studies/README.md) | explanation by example | see one effect or one demo built, with its cost and limits |
| [concepts/](concepts/README.md) | explanation | understand why |
| [reference/](reference/README.md) | reference | look up a call, a demo, a credit |

## ⚡ Quick answers

- *"I just want to see something."* → [Install and run](tutorials/01-install.md)
- *"I want my own window."* → [A first scene](tutorials/02-first-scene.md#-the-same-scene-in-your-own-window)
- *"I simulate things, no window needed."* → [Run without a screen](guides/run-without-a-screen.md), then [the World](concepts/the-world.md)
- *"I have a shader and numbers."* → [The GPU alone](tutorials/07-the-gpu-alone.md), then [the GPU layer](concepts/the-gpu-layer.md)
- *"I compute physics on a grid."* → [Values on a grid](tutorials/04-values-on-a-grid.md), then [the case studies](case-studies/README.md)
- *"I want one effect: bloom, a cartoon look, flowing water."* → [Techniques](techniques/README.md)
- *"My window is black."* → [A black window](guides/debug-a-black-window.md)
- *"Where is the call that…?"* → [API reference](reference/api.md)

## 📷 Pictures

Every screenshot of these pages lives in the [Compages-data](https://github.com/Lecrapouille/Compages-data) repository, in `doc/examples/`, one per demo. They are written by the gallery itself, so they show what the code really draws:

```sh
./build/linux-gcc-debug/bin/Compages-examples --check --shots external/Compages-data/doc/examples
```

## 🌍 A note on the language

This documentation is written in simple English, with short sentences and common words, so that readers whose first language is not English (French readers in particular) can follow it easily.
