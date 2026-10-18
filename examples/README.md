# 🖼 Examples

`./build/Compages-examples` opens the whole gallery: viewport, source of the demo, GPU counters and leak checks. The commented catalog, with screenshots and keys, is [doc/09-Examples.md](../doc/09-Examples.md).

```sh
make -j"$(nproc)"
./build/Compages-examples
./build/Compages-examples 50_ThreeJsLike
./build/Compages-examples --check
```

Read the folders in order: each demo adds one idea to the previous one.

| Folders | Layer | Guide |
|---------|-------|-------|
| `00_GettingStarted/`, `10_ScientificAndCompute/` | 🟧 GPU | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md) |
| `20_Performance/` | GPU, then Renderer | [doc/04-GPU-Layer.md](../doc/04-GPU-Layer.md), [doc/06-Renderer-Layer.md](../doc/06-Renderer-Layer.md) |
| `30_HeadlessWorld`, then the rest of `30_WorldAndAssets/` | 🟦 World, then Renderer | [doc/05-World-Layer.md](../doc/05-World-Layer.md), [doc/06-Renderer-Layer.md](../doc/06-Renderer-Layer.md) |
| `50_Complete/` | 🟩 the three layers | [doc/06-Renderer-Layer.md](../doc/06-Renderer-Layer.md) |

Textures, glTF and URDF files come from `external/Compages-data/`, downloaded by `make download-external-libs` ([doc/02-Getting-Started.md](../doc/02-Getting-Started.md)).

PgUp / PgDn change demo, F1 hides the panels, F5 restarts, Esc quits.

To add a demo: a `.hpp`/`.cpp` pair in the right folder, and an entry in `Common/ExampleManifest.hpp` (the manifest is the list of demos).
