# 🖼 Examples

`./build/linux-gcc-debug/bin/Compages-examples` opens the whole gallery: viewport, source of the demo, GPU counters and leak checks. The commented catalog, with screenshots and keys, is [doc/09-Examples.md](../doc/09-Examples.md).

```sh
make -j"$(nproc)"
./build/linux-gcc-debug/bin/Compages-examples
./build/linux-gcc-debug/bin/Compages-examples 06a_FirstScene
./build/linux-gcc-debug/bin/Compages-examples --check
./build/linux-gcc-debug/bin/Compages-examples --check 06a_FirstScene   # one demo only
```

Read the folders in order: each folder answers one question, and each demo adds one idea to the previous one (`05b` is chapter 5, step b).

| Folder | Question | Layer | Guide |
|--------|----------|-------|-------|
| `01_FirstSteps/` | How do I draw? | 🟧 GPU | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `02_ChangingData/` | How do I send what changes? | 🟧 GPU | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `03_Images/` | Textures and 3D on the raw GPU | 🟧 GPU | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `04_Passes/` | Pixels computed by a shader, several passes | 🟧 GPU | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `05_Simulation/` | Make the GPU compute | 🟧 GPU | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `06_Scene/` | Compose a scene, like Three.js | 🟩 Renderer | [doc/06-Renderer-Layer.md](../doc/06-Renderer-Layer.md) |
| `07_World/` | Things that move by themselves | 🟦 World | [doc/05-World-Layer.md](../doc/05-World-Layer.md) |
| `08_Assets/` | Load files (glTF, URDF, prefabs) | 🟩 Renderer | [doc/06-Renderer-Layer.md](../doc/06-Renderer-Layer.md) |
| `09_Performance/` | Draw a lot | GPU, then Renderer | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `10_Complete/` | Everything together, up to a game | the three layers | [doc/06-Renderer-Layer.md](../doc/06-Renderer-Layer.md) |

Textures, glTF and URDF files come from the repository `Compages-data`, fetched by `make` with the other third parties ([doc/02-Getting-Started.md](../doc/02-Getting-Started.md)).

PgUp / PgDn change demo, F1 hides the panels, F5 restarts, Esc quits.

To add a demo: a `.hpp`/`.cpp` pair in the right folder, and an entry in `Common/ExampleManifest.hpp` (the manifest is the list of demos).
