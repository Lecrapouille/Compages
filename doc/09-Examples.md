# 🖼 9. Examples: the gallery

One binary, `Compages-examples`, holds every demo: viewport, list, inspector, source, console, GPU leak checks. It is the living manual of the project. The sources are written to be read in the order of the folders; each `description()` explains the idea the demo adds.

```sh
make -j"$(nproc)"
./build/Compages-examples
./build/Compages-examples 01b_Triangle
./build/Compages-examples --check
```

A short guide of the folders is in [examples/README.md](../examples/README.md).

---

## 🎛 1. The window

The viewport draws the demo into a texture: the panels never hide part of the image.

| Key | Action |
|-----|--------|
| PgUp / PgDn | previous / next demo |
| F1 | show or hide the panels |
| F2 | wireframe |
| F5 | restart the demo |
| F6 / F7 | pause / one frame |
| F12 | save the viewport as PNG in `screenshots/` |
| Shift+F12 | save the whole window, panels included |
| Esc | release the mouse (games), or quit |

| Option | Effect |
|--------|--------|
| `--check` | runs each demo for a few frames; fails if a GPU handle is left |
| `--cycle` | goes through the demos by itself (for a second screen) |
| `--shots DIR` | saves one PNG per demo; use with `--check` or `--cycle` |
| `--shots-with-panels DIR` | the same, with the panels in the picture |
| `--no-overlay` | starts with the panels hidden |

```sh
./build/Compages-examples --check --shots external/Compages-data/doc/examples
```

This fills the [Compages-data](https://github.com/Lecrapouille/Compages-data) repository; five of these images are copied in `doc/images/` for the documentation.

The list of demos is the manifest, `examples/Common/ExampleManifest.hpp`: a new demo is a `.hpp`/`.cpp` pair in its folder plus one line there.

---

## 🧭 2. The folders and the layers

| Folder | Layers |
|--------|--------|
| `00_GettingStarted/`, `10_ScientificAndCompute/` | GPU |
| `20_Performance/` | GPU, then Renderer (for the cubes) |
| `30_WorldAndAssets/` | World, then World + Renderer |
| `50_Complete/` | all three, up to a game |

`30_HeadlessWorld` ends before any `Scene` code.

Demos that read files need `make download-external-libs` (the `Compages-data` repository).

---

## 🟧 3. 00 — Getting started

| Demo | Idea |
|------|------|
| `00a_Dummy` | the gallery cycle, zero GPU resource |
| `00b_CpuGpuSync` | CPU copy; orange bars = not sent yet; a button prints both copies to the console |
| `00c_Compute` | one computation; `download()` brings the numbers back; a button prints them to the console |
| `01a_ClearScreen` | `clear` and two passes, no shader |
| `01b_Triangle` | `Drawable`, data by name |
| `01c_InterleavedTriangle` | a `Vertex` struct, an immutable buffer |
| `02_DynamicGeometry` | a dirty vertex follows the mouse |
| `03a_TexturedQuad` | `quad["image"] = texture` |
| `03b_MultiTextureBlend` | five textures, a blend map painted with the mouse |
| `04_DepthAndTransforms` | indices, depth, culling, three matrices |
| `05a_MultiPassMesh` | one `Buffer<Vertex>`, three pipelines |
| `05b_RenderToTexture` | a cube drawn off-screen, the texture read twice |
| `05c_PostProcess` | a scene off-screen, then a full-screen pass |
| `06a_Mandelbrot` | no vertex buffer, only `gl_VertexID` |
| `06b_ComplexShader` | a long shader on a quad |
| `07_PointClouds` | a sphere drawn with `GL_POINTS` |

![Triangle](images/01b_Triangle.png)

---

## 🧪 4. 10 — Scientific and compute

| Demo | Idea |
|------|------|
| `10a_HeightMap` | a hundred thousand heights computed on the CPU |
| `10b_Terrain3D` | an island colored by a 3D texture (water to snow) |
| `11a_GameOfLife` | one cell = one pixel, two ping-pong textures |
| `11b_GrayScott` | the same ping-pong, with floats |
| `12_ComputeParticles` | compute writes the buffer, the draw reads it |
| `13_Galaxy` | eight thousand stars in a dark matter halo, tiles in shared memory, ping-pong; Save writes `galaxy.csv` |
| `14_SpiralGalaxy` | density waves: black-body ramp, point sprites |
| `15_Lorenz` | a growing curve; only the new part is sent |
| `16_NeuralNetwork` | a neural network learns on the GPU; the CPU gives the examples and reads the answers |

![Particles](images/12_ComputeParticles.png)

![Galaxy](images/13_Galaxy.png)

`13_Galaxy`: each star starts on a circular orbit, with the speed that balances the pull of what is inside its radius (the stars nearer the centre, and a dark matter halo). Slower, the disk falls into the centre; a cold, heavy disk breaks into clumps within one turn. The halo and a little disorder in the speeds keep it a disk. The arms wind up as it turns, as they do in a real galaxy (the stars near the centre go round faster): `14_SpiralGalaxy` shows the density waves that keep arms in place. Save reads the stars back from the GPU and writes `galaxy.csv` (`x,y,vx,vy`, one line per star) in the current folder. To plot those positions in Python (scatter of `x` and `y`, comment lines ignored), run `python3 examples/10_ScientificAndCompute/13_Galaxy_plot_xy.py path/to/galaxy.csv` (`matplotlib` required; `-o out.png` saves instead of opening a window).

`16_NeuralNetwork`: a multilayer perceptron, 2 → 16 → 16 → 1 (tanh, tanh, sigmoid), learns to tell two classes of points apart (circle, XOR or two spirals). The CPU makes the examples and the first weights, and reads the answers back; the GPU does all the computation in three compute shaders:

1. gradients: one invocation per example, the forward pass then the backward pass (chain rule); each writes its own gradient, one float per weight;
2. update: one invocation per weight, the mean of its gradients over the examples, then an Adam step (one more invocation averages the loss);
3. evaluate: one invocation per point, the forward pass only. It colours the plane, and answers the point chosen in the Try it panel.

A `barrier(Storage)` sits between the passes: the update reads every gradient, the next gradients read the new weights. A button prints every example and the answer of the network to the console.

---

## ⚡ 5. 20 — Performance

| Demo | Idea |
|------|------|
| `20a_SpriteBatch` | a hundred thousand sprites, one draw, per-instance data |
| `20b_ManyCubes` | 1,728 cubes, one mesh, sorted by look, culling |
| `21_IndirectDraw` | compute writes the survivors and the draw command; the CPU never reads the count |

---

## 🟦 6. 30 — World and files

| Demo | Idea |
|------|------|
| `30_HeadlessWorld` | ECS and behavior, no drawing |
| `31_MovingRobot` | boxes linked by joints, a `Walk` behavior |
| `32a_SplitViews` | one world, two cameras, one update |
| `32b_CameraPick` | orbit or fly, click to select |
| `32c_MiscLookAt` | a thousand cones look at a sphere |
| `33a_TexturedSpheres` | color, photo, tinted photo |
| `33b_TextureGallery` | nine images of the assets repository |
| `33c_GeometryShowcase` | primitives: lit, textured, depth, normals |
| `34_GltfModel` | `Duck.glb`, `frameAll` |
| `35_PrefabAndSave` | three copies, then a JSON of asset names |
| `36a_AnimatedModel` | hips and shoulders as joints |
| `36b_GltfAnimation` | animation clips of `Soldier.glb` |
| `37_Skybox` | six faces; the sky turns with the camera |
| `38_RobotArm` | URDF IRB 2400, forward kinematics |
| `39a_CanvasPaint` | live albedo: a CPU brush, `CanvasTexture` |

---

## 🟩 7. 50 — Complete

| Demo | Idea |
|------|------|
| `50_ThreeJsLike` | shape, camera, light, one draw per frame |
| `51_Behaviors` | three cubes, three behaviors; Space for the green one |
| `52_MvpDemo` | falling cubes, a lamp that casts shadows, debug boxes |
| `53_DoomLike` | the game: glTF, particles, fog, shadows of the torches, two levels |
| `54_SweatAndDecals` | a head that sweats, with decals: GPU passes in the UV map of a mesh |

![Cube](images/50_ThreeJsLike.png)

![Doom-like](images/53_DoomLike.png)

`53_DoomLike`: click to capture the mouse. WASD or arrows to move, Shift to run, Q and E to turn, left click to shoot, R to reload, F for the torch, L for the lantern. The green light leaves the level.

`54_SweatAndDecals`: the head of the three.js `webgl_decals` demo (Lee Perry-Smith). Nothing is simulated in 3D. The mesh is drawn into its own UV map (`gl_Position = uv`), so one texel of a texture is one small piece of skin:

1. Force pass: gravity projected on the skin. `dFdx` and `dFdy` of the world position give the two tangents of the UV map, so the slope is written in texels per second. The pass also writes where the skin faces down (drops fall there) and where it sweats (the forehead).
2. Water pass (ping-pong): pores fill small beads. A texel holds one drop and the place of the drop inside the texel, so drops glide instead of jumping. A bead heavier than a threshold runs down the force field; when it crosses the border of its texel it goes to that neighbour (diagonals too) and leaves a thin wet film. It stops when it has left most of its water behind, at a threshold that changes from texel to texel (rough skin). Each texel computes what its eight neighbours send it, so water is never created on the way.
3. Decal pass: a box in front of the click is projected on the skin it contains, like `DecalGeometry` in three.js, and the picture is baked into a decal map.
4. Wash pass (ping-pong): sweat dilutes the decals. Where a drop runs, the paint is read from upstream, so it is carried down in streaks; wherever there is water, it fades (Washing in the Try it panel).

The skin material is the PBR shader of the Renderer (`shaders::pbrFragment()`) with another `main()`: normal map, decals, darker wet skin. Each drop is drawn as a small round cap at its place (radius from the cube root of its water), so drops stay round and merge when they touch. Left click paints a decal or drops water (Try it panel), right drag turns. Pitch and Roll tilt the head, and the drops change their way. Water stops at the seams of the UV map: it does not cross from one island to the next.

---

## 🪟 8. Window, ImGui and physics

- Only `examples/Common/Window.cpp` talks to GLFW. The library receives a context that already exists.
- The panels use ImGui (the `docking` branch of the manifest). After them, `gpu::forgetRenderState()` keeps the ImGui state from being taken for the state of the demo.
- The old contact demos are in `attic/Physics/`, not in the binary ([chapter 7](07-Architecture.md#-9-window-and-physics)).

Next: [10-Debugging.md](10-Debugging.md).
