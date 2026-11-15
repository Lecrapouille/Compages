# 🖼 Examples

`./build/linux-gcc-debug/bin/Compages-examples` opens the whole gallery: viewport, source of the demo, GPU counters and leak checks. The commented catalog, with screenshots and keys, is [the gallery reference](../doc/reference/gallery.md).

```sh
make -j"$(nproc)"
./build/linux-gcc-debug/bin/Compages-examples
./build/linux-gcc-debug/bin/Compages-examples 00a_Workshop
./build/linux-gcc-debug/bin/Compages-examples --check
./build/linux-gcc-debug/bin/Compages-examples --check 00a_Workshop     # one demo only
```

Read the folders in order: each folder answers one question, and each demo adds one idea to the previous one (`05b` is chapter 5, step b).

| Folder | Question | Layer | Read |
|--------|----------|-------|------|
| `00_Workshop/` | The tutorials, step by step: a robot workshop | the three layers | [Tutorials](../doc/tutorials/README.md) |
| `01_FirstSteps/` | How do I draw? | 🟧 GPU | [The GPU layer](../doc/concepts/the-gpu-layer.md) |
| `02_ChangingData/` | How do I send what changes? | 🟧 GPU | [The GPU layer](../doc/concepts/the-gpu-layer.md) |
| `03_Images/` | Textures and 3D on the raw GPU | 🟧 GPU | [Draw in several passes](../doc/guides/draw-in-several-passes.md) |
| `04_Passes/` | Pixels computed by a shader, several passes | 🟧 GPU | [Draw in several passes](../doc/guides/draw-in-several-passes.md) |
| `05_Simulation/` | Make the GPU compute | 🟧 GPU | [Compute on the GPU](../doc/guides/compute-on-the-gpu.md) |
| `06_Scene/` | Compose a scene, like Three.js | 🟩 Renderer | [Guides](../doc/guides/README.md) |
| `07_World/` | Things that move by themselves | 🟦 World | [The World](../doc/concepts/the-world.md) |
| `08_Assets/` | Load files (glTF, URDF, prefabs) | 🟩 Renderer | [Load a model](../doc/guides/load-a-model.md) |
| `09_Performance/` | Draw a lot | GPU, then Renderer | [Measure a frame](../doc/guides/measure-a-frame.md) |
| `10_Complete/` | Everything together, up to a game | the three layers | [Case studies](../doc/case-studies/README.md) |
| `11_Science/` | Fields: compute them, show them, read them | Fields, then GPU and Renderer | [Case studies](../doc/case-studies/README.md) |
| `12_Effects/` | Filters of the picture of a view | 🟩 Renderer | [Screen effects](../doc/guides/screen-effects.md) |

Textures, glTF and URDF files come from the repository `Compages-data`, fetched by `make` with the other third parties ([build and install](../doc/guides/build-and-install.md)).

PgUp / PgDn change demo, F1 hides the panels, F5 restarts, Esc quits.

To add a demo: a `.hpp`/`.cpp` pair in the right folder, and an entry in `Common/ExampleManifest.hpp` (the manifest is the list of demos).
