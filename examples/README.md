# Examples

`./build/Compages-examples` opens the full gallery: viewport, demo source code, GPU counters, and leak tests.

The commented catalog, with screenshots, is available at [doc/Examples.md](../doc/Examples.md).

```sh
make -j"$(nproc)"
./build/Compages-examples
./build/Compages-examples 50_ThreeJsLike
./build/Compages-examples --check
```

Browse the source folders in order. Each demo builds on the previous one and introduces a new idea.

| Folders                                    | Layer                  | Guide                                                  |
|---------------------------------------------|------------------------|--------------------------------------------------------|
| `00_GettingStarted/`, `10_ScientificAndCompute/` | GPU                   | [doc/GPU.md](../doc/GPU.md)                            |
| `30_HeadlessWorld`, then the rest of `30_WorldAndAssets/` | World, then renderer   | [doc/World.md](../doc/World.md), [doc/Renderer.md](../doc/Renderer.md) |
| `50_Complete/`                              | All three layers       | [doc/Renderer.md](../doc/Renderer.md)                  |

Textures, glTF, and URDF files are located in `external/Compages-data/`, which are downloaded using `make download-external-libs`. More details: [doc/Install.md](../doc/Install.md).

PgUp / PgDn switch between demos, F1 hides panels, F5 reloads, and Esc exits. The complete list of keyboard shortcuts is in [doc/CheatSheet.md](../doc/CheatSheet.md).
