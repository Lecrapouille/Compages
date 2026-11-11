# 🖼 9. Examples: the gallery

One binary, `Compages-examples`, holds every demo: viewport, list, inspector, source, console, GPU leak checks. It is the living manual of the project. The sources are written to be read in the order of the folders; each `description()` explains the idea the demo adds.

```sh
make -j"$(nproc)"
./build/linux-gcc-debug/bin/Compages-examples
./build/linux-gcc-debug/bin/Compages-examples 01c_Triangle
./build/linux-gcc-debug/bin/Compages-examples --check
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
| `--check NAME` | the same for one demo only, for example `--check 10d_EarthMoon` |
| `--cycle` | goes through the demos by itself (for a second screen) |
| `--shots DIR` | saves one PNG per demo; use with `--check` or `--cycle` |
| `--shots-with-panels DIR` | the same, with the panels in the picture |
| `--no-overlay` | starts with the panels hidden |

```sh
./build/linux-gcc-debug/bin/Compages-examples --check --shots external/Compages-data/doc/examples
```

This fills the [Compages-data](https://github.com/Lecrapouille/Compages-data) repository, where every page of the documentation takes its screenshots. It writes in a clone of yours, not in the copy fetched by MyMakefile: `git clone https://github.com/Lecrapouille/Compages-data.git external/Compages-data` first.

The list of demos is the manifest, `examples/Common/ExampleManifest.hpp`: a new demo is a `.hpp`/`.cpp` pair in its folder plus one line there.

Next to the gallery, `Compages-headless` (`examples/Headless/main.cpp`, Linux) draws a scene into a PNG with **no screen at all**: an EGL context without surface, then `Scene::renderToFile`. It runs on a server or in CI, on a GPU or with Mesa llvmpipe ([chapter 6](06-Renderer-Layer.md)):

```sh
./build/linux-gcc-debug/bin/Compages-headless scene.png 1280 720
```

---

## 🧭 2. The folders and the layers

| Folder | Question | Layer | Guide |
|--------|----------|-------|-------|
| `01_FirstSteps/` | How do I draw? | 🟧 GPU | [doc/04-GPU-Layer.md](04-GPU-Layer.md) |
| `02_ChangingData/` | How do I send what changes? | 🟧 GPU | [doc/04-GPU-Layer.md](04-GPU-Layer.md) |
| `03_Images/` | Textures and 3D on the raw GPU | 🟧 GPU | [doc/04-GPU-Layer.md](04-GPU-Layer.md) |
| `04_Passes/` | Pixels computed by a shader, several passes | 🟧 GPU | [doc/04-GPU-Layer.md](04-GPU-Layer.md) |
| `05_Simulation/` | Make the GPU compute | 🟧 GPU | [doc/04-GPU-Layer.md](04-GPU-Layer.md) |
| `06_Scene/` | Compose a scene, like Three.js | 🟩 Renderer | [doc/06-Renderer-Layer.md](06-Renderer-Layer.md) |
| `07_World/` | Things that move by themselves | 🟦 World | [doc/05-World-Layer.md](05-World-Layer.md) |
| `08_Assets/` | Load files (glTF, URDF, prefabs) | 🟩 Renderer | [doc/06-Renderer-Layer.md](06-Renderer-Layer.md) |
| `09_Performance/` | Draw a lot | GPU, then Renderer | [doc/04-GPU-Layer.md](04-GPU-Layer.md) |
| `10_Complete/` | Everything together, up to a game | the three layers | [doc/06-Renderer-Layer.md](06-Renderer-Layer.md) |
| `11_Science/` | Fields: compute them, show them, read them | Fields, then GPU and Renderer | [doc/06-Renderer-Layer.md](06-Renderer-Layer.md) |

`07a_HeadlessWorld` ends before any `Scene` code.

Demos that read files need the `Compages-data` repository, fetched by `make` with the other third parties.

---

## 🟧 3. 01 — First steps: how do I draw?

| Demo | Idea | Picture |
|------|------|---------|
| `01a_Dummy` | the gallery cycle, zero GPU resource | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01a_Dummy.png" alt="01a_Dummy" width="200"> |
| `01b_ClearScreen` | `clear` and two passes, no shader | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01b_ClearScreen.png" alt="01b_ClearScreen" width="200"> |
| `01c_Triangle` | `Drawable`, data by name | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01c_Triangle.png" alt="01c_Triangle" width="200"> |
| `01d_InterleavedTriangle` | a `Vertex` struct, `BufferUsage::Static` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01d_InterleavedTriangle.png" alt="01d_InterleavedTriangle" width="200"> |
| `01e_BorrowedTriangle` | a `Buffer` we own, borrowed by two drawables (`borrowVertices`) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01e_BorrowedTriangle.png" alt="01e_BorrowedTriangle" width="200"> |
| `01f_VertexSyntax` | every way of writing and changing vertices, one small triangle each | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/01f_VertexSyntax.png" alt="01f_VertexSyntax" width="200"> |

---

## 🟧 4. 02 — Changing data: how do I send what changes?

| Demo | Idea | Picture |
|------|------|---------|
| `02a_CpuGpuSync` | CPU copy; orange bars = not sent yet; a button prints both copies to the console | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/02a_CpuGpuSync.png" alt="02a_CpuGpuSync" width="200"> |
| `02b_DynamicGeometry` | `BufferUsage::Dynamic`: one edited corner follows the mouse | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/02b_DynamicGeometry.png" alt="02b_DynamicGeometry" width="200"> |
| `02c_HeightMap` | a hundred thousand heights computed on the CPU, all sent again | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/02c_HeightMap.png" alt="02c_HeightMap" width="200"> |
| `02d_Lorenz` | a growing curve; only the new part is sent | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/02d_Lorenz.png" alt="02d_Lorenz" width="200"> |

---

## 🟧 5. 03 — Images: textures and 3D on the raw GPU

| Demo | Idea | Picture |
|------|------|---------|
| `03a_TexturedQuad` | `quad["image"] = texture` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/03a_TexturedQuad.png" alt="03a_TexturedQuad" width="200"> |
| `03b_MultiTextureBlend` | five textures, a blend map painted with the mouse | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/03b_MultiTextureBlend.png" alt="03b_MultiTextureBlend" width="200"> |
| `03c_DepthAndTransforms` | indices, depth, culling, three matrices | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/03c_DepthAndTransforms.png" alt="03c_DepthAndTransforms" width="200"> |
| `03d_PointClouds` | a sphere drawn with `GL_POINTS` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/03d_PointClouds.png" alt="03d_PointClouds" width="200"> |
| `03e_Terrain3D` | an island colored by a 3D texture (water to snow) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/03e_Terrain3D.png" alt="03e_Terrain3D" width="200"> |

---

## 🟧 6. 04 — Passes: pixels computed by a shader, several passes

| Demo | Idea | Picture |
|------|------|---------|
| `04a_Mandelbrot` | no vertex buffer, only `gl_VertexID` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04a_Mandelbrot.png" alt="04a_Mandelbrot" width="200"> |
| `04b_ComplexShader` | a long shader on a quad | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04b_ComplexShader.png" alt="04b_ComplexShader" width="200"> |
| `04c_MultiPassMesh` | one `VertexBuffer<Vertex>`, borrowed by three drawables | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04c_MultiPassMesh.png" alt="04c_MultiPassMesh" width="200"> |
| `04d_RenderToTexture` | a cube drawn off-screen, the texture read twice | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04d_RenderToTexture.png" alt="04d_RenderToTexture" width="200"> |
| `04e_PostProcess` | a scene off-screen, then a full-screen pass | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/04e_PostProcess.png" alt="04e_PostProcess" width="200"> |

---

## 🧪 7. 05 — Simulation: make the GPU compute

| Demo | Idea | Picture |
|------|------|---------|
| `05a_Compute` | one computation; `download()` brings the numbers back; a button prints them to the console | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05a_Compute.png" alt="05a_Compute" width="200"> |
| `05b_GameOfLife` | one cell = one pixel, a `TexturePingPong` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05b_GameOfLife.png" alt="05b_GameOfLife" width="200"> |
| `05c_GrayScott` | the same ping-pong, with floats | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05c_GrayScott.png" alt="05c_GrayScott" width="200"> |
| `05d_ComputeParticles` | compute writes the buffer, the draw reads it | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05d_ComputeParticles.png" alt="05d_ComputeParticles" width="200"> |
| `05e_SpiralGalaxy` | a formula of the time, computed in the vertex shader: no CPU loop, no compute shader. Port of the [Glumpy galaxy](https://github.com/glumpy/glumpy/blob/master/examples/galaxy.py) (N. P. Rougier), after [Ingo Berg](https://github.com/beltoforion/Galaxy-Renderer) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05e_SpiralGalaxy.png" alt="05e_SpiralGalaxy" width="200"> |
| `05f_Galaxy` | a state that depends on the previous one: eight thousand stars, tiles in shared memory, `PingPong`; Save writes `galaxy.csv` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05f_Galaxy.png" alt="05f_Galaxy" width="200"> |
| `05g_NeuralNetwork` | a neural network learns on the GPU; the CPU gives the examples and reads the answers | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/05g_NeuralNetwork.png" alt="05g_NeuralNetwork" width="200"> |

`05f_Galaxy`: each star starts on a circular orbit, with the speed that balances the pull of what is inside its radius (the stars nearer the centre, and a dark matter halo). Slower, the disk falls into the centre; a cold, heavy disk breaks into clumps within one turn. The halo and a little disorder in the speeds keep it a disk. The arms wind up as it turns, as they do in a real galaxy (the stars near the centre go round faster): `05e_SpiralGalaxy` shows the density waves that keep arms in place. Save reads the stars back from the GPU and writes `galaxy.csv` (`x,y,vx,vy`, one line per star) in the current folder. To plot those positions in Python (scatter of `x` and `y`, comment lines ignored), run `python3 examples/05_Simulation/05f_Galaxy_plot_xy.py path/to/galaxy.csv` (`matplotlib` required; `-o out.png` saves instead of opening a window).

`05g_NeuralNetwork`: a multilayer perceptron, 2 → 16 → 16 → 1 (tanh, tanh, sigmoid), learns to tell two classes of points apart (circle, XOR or two spirals). The CPU makes the examples and the first weights, and reads the answers back; the GPU does all the computation in three compute shaders:

1. gradients: one invocation per example, the forward pass then the backward pass (chain rule); each writes its own gradient, one float per weight;
2. update: one invocation per weight, the mean of its gradients over the examples, then an Adam step (one more invocation averages the loss);
3. evaluate: one invocation per point, the forward pass only. It colours the plane, and answers the point chosen in the Try it panel.

A `barrier(Storage)` sits between the passes: the update reads every gradient, the next gradients read the new weights. A button prints every example and the answer of the network to the console.

---

## 🟩 8. 06 — Scene: compose a scene, like Three.js

| Demo | Idea | Picture |
|------|------|---------|
| `06a_FirstScene` | shape, camera, light, one draw per frame | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06a_FirstScene.png" alt="06a_FirstScene" width="200"> |
| `06b_Transforms` | `position`, `rotation`, `scale` chain; `rotate` adds a turn; helpers: grid, axes, bounds | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06b_Transforms.png" alt="06b_Transforms" width="200"> |
| `06c_Hierarchy` | a car pivot, body and wheels hung with `parent()`: move one, all follow; axes show the pivot | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06c_Hierarchy.png" alt="06c_Hierarchy" width="200"> |
| `06d_Lights` | ambient, sun, two lamps going round with their unlit bulbs; exposure and tone curve in Try it | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06d_Lights.png" alt="06d_Lights" width="200"> |
| `06e_TexturedSpheres` | color, photo, tinted photo, a see-through glass ball | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06e_TexturedSpheres.png" alt="06e_TexturedSpheres" width="200"> |
| `06f_GeometryShowcase` | primitives: lit, textured, depth, normals | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06f_GeometryShowcase.png" alt="06f_GeometryShowcase" width="200"> |
| `06g_TextureGallery` | nine images of the assets repository | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06g_TextureGallery.png" alt="06g_TextureGallery" width="200"> |
| `06h_Skybox` | six faces; the sky turns with the camera | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06h_Skybox.png" alt="06h_Skybox" width="200"> |
| `06i_CameraPick` | orbit or fly, click to select: the cube changes colour and shows its bounds | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06i_CameraPick.png" alt="06i_CameraPick" width="200"> |
| `06j_SplitViews` | one world, two cameras, one update | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06j_SplitViews.png" alt="06j_SplitViews" width="200"> |
| `06k_MiscLookAt` | a thousand cones look at a sphere | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06k_MiscLookAt.png" alt="06k_MiscLookAt" width="200"> |
| `06l_CanvasPaint` | live albedo: a CPU brush, `CanvasTexture` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06l_CanvasPaint.png" alt="06l_CanvasPaint" width="200"> |

---

## 🟦 9. 07 — World: things that move by themselves

| Demo | Idea | Picture |
|------|------|---------|
| `07a_HeadlessWorld` | ECS and routine, no drawing | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07a_HeadlessWorld.png" alt="07a_HeadlessWorld" width="200"> |
| `07b_Routines` | three cubes, three routines; Space for the green one | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07b_Routines.png" alt="07b_Routines" width="200"> |
| `07c_MovingRobot` | boxes linked by joints, a `Walk` routine | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07c_MovingRobot.png" alt="07c_MovingRobot" width="200"> |
| `07d_AnimatedModel` | hips and shoulders as joints | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07d_AnimatedModel.png" alt="07d_AnimatedModel" width="200"> |
| `07e_ExternalPhysics` | a small outside engine stepped in `fixedUpdate`; `PoseDriver::External`; shocks sent as `Contact` messages | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/07e_ExternalPhysics.png" alt="07e_ExternalPhysics" width="200"> |

---

## 🟩 10. 08 — Assets: load files

| Demo | Idea | Picture |
|------|------|---------|
| `08a_GltfModel` | `Duck.glb`, `frameAll` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/08a_GltfModel.png" alt="08a_GltfModel" width="200"> |
| `08b_GltfAnimation` | animation clips of `Soldier.glb` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/08b_GltfAnimation.png" alt="08b_GltfAnimation" width="200"> |
| `08c_PrefabAndSave` | three copies, then a JSON of asset names | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/08c_PrefabAndSave.png" alt="08c_PrefabAndSave" width="200"> |
| `08d_RobotArm` | URDF IRB 2400, forward kinematics; the frame of each joint (axes), a grid | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/08d_RobotArm.png" alt="08d_RobotArm" width="200"> |

---

## ⚡ 11. 09 — Performance: draw a lot

| Demo | Idea | Picture |
|------|------|---------|
| `09a_SpriteBatch` | a hundred thousand sprites, one draw, per-instance data | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/09a_SpriteBatch.png" alt="09a_SpriteBatch" width="200"> |
| `09b_ManyCubes` | 1,728 cubes, one mesh, sorted by look, culling | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/09b_ManyCubes.png" alt="09b_ManyCubes" width="200"> |
| `09c_IndirectDraw` | compute writes the survivors and the draw command; the CPU never reads the count | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/09c_IndirectDraw.png" alt="09c_IndirectDraw" width="200"> |

---

## 🎮 12. 10 — Complete: everything together

| Demo | Idea | Picture |
|------|------|---------|
| `10a_MvpDemo` | falling cubes, a lamp that casts shadows, debug boxes | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10a_MvpDemo.png" alt="10a_MvpDemo" width="200"> |
| `10b_DoomLike` | the game: glTF, particles, fog, shadows of the torches, two levels | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10b_DoomLike.png" alt="10b_DoomLike" width="200"> |
| `10c_SweatAndDecals` | a head that sweats, with decals: GPU passes in the UV map of a mesh | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10c_SweatAndDecals.png" alt="10c_SweatAndDecals" width="200"> |
| `10d_EarthMoon` | a rocket from Kourou to the Moon at the real scale: logarithmic zoom (Shift: faster), reversed depth, floating origin (untick it in Try it to see the 32 m steps); NASA textures | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10d_EarthMoon.png" alt="10d_EarthMoon" width="200"> |
| `10e_Archipelago` | a map of knowledge after Chréage: islands of Voronoi cells grown by uplift and river erosion, regions and parts made of cells, rivers that dig their valleys, finer tiles near the camera, loaded behind a progress bar; an animated sea, boats, gulls and fish; the web links in a tooltip | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10e_Archipelago.png" alt="10e_Archipelago" width="200"> |

`10b_DoomLike`: click to capture the mouse. WASD or arrows to move, Shift to run, Q and E to turn, left click to shoot, R to reload, F for the torch, L for the lantern. The green light leaves the level.

`10c_SweatAndDecals`: the head of the three.js `webgl_decals` demo (Lee Perry-Smith). Nothing is simulated in 3D. The mesh is drawn into its own UV map (`gl_Position = uv`), so one texel of a texture is one small piece of skin:

1. Force pass: gravity projected on the skin. `dFdx` and `dFdy` of the world position give the two tangents of the UV map, so the slope is written in texels per second. The pass also writes where the skin faces down (drops fall there) and where it sweats (the forehead).
2. Water pass (ping-pong): pores fill small beads. A texel holds one drop and the place of the drop inside the texel, so drops glide instead of jumping. A bead heavier than a threshold runs down the force field; when it crosses the border of its texel it goes to that neighbour (diagonals too) and leaves a thin wet film. It stops when it has left most of its water behind, at a threshold that changes from texel to texel (rough skin). Each texel computes what its eight neighbours send it, so water is never created on the way.
3. Decal pass: a box in front of the click is projected on the skin it contains, like `DecalGeometry` in three.js, and the picture is baked into a decal map.
4. Wash pass (ping-pong): sweat dilutes the decals. Where a drop runs, the paint is read from upstream, so it is carried down in streaks; wherever there is water, it fades (Washing in the Try it panel).

The skin material is the PBR shader of the Renderer (`shaders::pbr()`) with another `main()` given to its hook `main`: normal map, decals, darker wet skin. Each drop is drawn as a small round cap at its place (radius from the cube root of its water), so drops stay round and merge when they touch. Left click paints a decal or drops water (Try it panel), right drag turns. Pitch and Roll tilt the head, and the drops change their way. Water stops at the seams of the UV map: it does not cross from one island to the next.

---

`10e_Archipelago` is a new take on Chréage, the map of knowledge of I. Aberkane's thesis ("Mind Ergonomy for the Knowledge Economy", 2016, chapter 11). The contents come from [IslandedBrowser](https://github.com/Lecrapouille/IslandedBrowser): `organize.py` groups bookmarks of Firefox into islands and communities, and `IslandedBrowser -d DIR --layout` writes the place of every node, in `DIR/layout.json`, in a force directed layout (springs between similar pages). The demo reads `archipelago/bookmarks-layout.json` of Compages-data (the 118 public bookmarks of IslandedBrowser), or the file given by `COMPAGES_ARCHIPELAGO`; without it, a few pages of Wikipedia built in. The land is built around this graph and a seed (Try it: Seed, Erosion steps, Generate), on a map 16 km wide; every size is written with its unit (`toMetres(1.4_km)`). The code is in `examples/10_Complete/Archipelago/`, one file per part: `Knowledge` reads the layout, `Placement` fits it to the map, `Land` makes the land on the CPU in stages (`Shores`, `Relief` with `Geology`, `Strands` with `Rivers`, `Outlines`) and `Painting` its picture; the cells and the heights are `Field<T>` on a `Grid2D` ([Fields](07-Architecture.md#-1-overview)); on the GPU side, `Sea` (the hooks `deform` and `main` of the PBR shader), `FineTiles`, `Wildlife`, `Pins`, `Signs`, `Highlight`, `Panels` and `Browser`. The land is made in a thread (`Job`): `setUp()` returns at once, `Example::progress()` tells the gallery how far it has gone, and the gallery shows a progress bar (a check waits for the end before it counts its frames).

1. Cells: as the thesis asks, after A. Patel's polygon maps, the map is a Voronoi diagram of 96 x 96 cells of 167 m (one site per grid square, moved by up to 0.3 square, so the nearest site of a point is always among the 3 x 3 squares around it). Each cell is land or sea; water closed in by land is filled.
2. Islands around the graph: a force directed layout puts the islands far apart and their links close together. So each island gets a disc whose area is its share of the links, where the layout put it; the discs are pushed apart until a strait separates them, and the nodes of an island are scaled into its disc. The land is the union of round shapes on the graph (discs on the nodes, capsules along the edges island → region → link), on coordinates bent by noise for the coasts. Each land cell belongs to the region of the nearest link of its island: related links stay side by side.
3. Relief: the land cells grow by a simple geological model, on the graph of the cells (as Cordonnier et al. 2016). The plate rises under the islands (about 1 mm a year, more inland, 1.8 times more on the borders between regions, which become ridges) and the rivers cut it down by the stream power law, `dh/dt = U - K A^½ S`: faster where a river drains more land (A) and where it is steeper (S). A step is 40 000 years, solved exactly from the sea upward (Braun and Willett). The pits fill first (priority flood), the slopes creep. Between the sites, the ground is a smooth blend of the cells.
4. The hierarchy follows the thesis, which maps contents "as loci along a strand": a river is a strand of places, each place a strand of smaller places, down to the places that hold the contents. Here an island (a theme) is structured by its main river, the largest basin the erosion made; a region (a topic) is a basin between ridges, drained by its tributary, which runs down from its highest cell to a river or the sea; a link is one of the cells of the region nearest to the tributary, in the order of the layout along it, marked by a pin of the colour of the island (more links than cells: they share). Only these rivers are drawn: a smooth Catmull-Rom spline through their cells (the thesis tried Bézier, Catmull-Rom and B-splines for the river axis), meanders of about eleven widths, a width that grows with the square root of the area drained. They dig their valley (a smooth V) and their bed (with banks) in the mesh.
5. The edges of the cells are faint inside a region, firmer around a link, bold between regions. The region under the mouse is draped with a veil over its cells (their exact Voronoi polygons, cut by the bisectors of the sites around) and a band along its outline.
6. Levels of detail: the map is cut into 8 x 8 tiles of 2 km. From afar, one mesh of 31 m and one picture of 16 m a pixel. Near the camera, finer tiles (8 m, 2 m a pixel) are made by two threads and take the place of the coarse ones; a skirt along their edges hides the cracks. At most twelve fine tiles live at once. The heights come from one function of the position, so a coarse and a fine tile agree. Near the camera, the shader adds real pictures of grass, sand and rock (ambientCG, CC0), read at two scales and divided by their mean colour, so the colours of the map stay: sand on the beaches, rock on steep slopes, grass elsewhere.
7. The sea: near the camera, a surface of 160 x 160 quads follows the camera; its vertex shader (the PBR one, with a few lines added) moves each corner with Gerstner waves (GPU Gems 1, chapter 1), ten swells blown by one wind (the long ones close to it, the short chop spread wider), each with the speed of deep water waves (`omega² = g k`, a swell of 233 m has a period of 12 s and is 2.3 m high on average), the two shortest a chop in the light only. Waves come in groups: the height of each swell follows a smooth noise that drifts at the group speed (half the speed of the waves), the same on the GPU and for the boats. It is see-through: the sea bed shows in the shallows; foam on the steep crests and in the surf. Farther, the ground shader makes the waves in the normals only, and the two fade into each other. The boats ride the same swell, computed on the CPU from the same table.
8. Boats sail round the islands where the water is deep enough, gulls (white, grey wings, black tips) circle in flocks of four along the coast of each island, 30 to 80 m over the ground or the sea, flapping, fish swim in the lagoons and jump. Boats and fish are drawn at least as large as a part of their distance to the camera, like the icons of a map; a gull grows less than its distance (as its power 0.62), so it still gets larger as the camera comes closer, but never larger than a boat seen from as far.

The camera is a `Rig`: left drag moves the map, right drag turns it, the wheel zooms toward the mouse. Over the picture, the names are written as Chréage writes them (`Archipelago/Signs`): an island or a region on a rounded sign, a tab of the colour of its island on its left, raised on a thin stem over its place (higher, or left out, when it would cover another one; the islands from afar, the nearest regions closer); the links of the region under the mouse or chosen on a circle around it, each joined to its pin by a dotted line, in the order of the pins round the circle so that the lines hardly cross. Also over the picture, `Example::overlay()` lets the demo draw Dear ImGui windows (the pointer over them and the keys typed into them do not reach `draw()`); `Signs` draws with `ImGui::GetWindowDrawList()` on the picture, and `Archipelago/Panels` opens the windows of the interface of IslandedBrowser: a tooltip next to the pointer (the title, the URL, where it is, what a click does), the details of the choice on the right (its links: a click goes there, Ctrl + click opens), a search of the titles and URLs (`/` or Ctrl + F), a status bar with the keys of the moment, short notifications, and a help (`H`). A click selects, a double click opens a link or flies to a region (`RigController::flyTo`), a middle or Ctrl click opens in the web browser (`$BROWSER`, else firefox; a region: its first 20 links); Enter opens the selection, Z flies to it, Home to the whole map, N, B and Space show the names, the borders, the animals.

## 🔬 13. 11 — Science: fields, computed and shown

Values on a grid (`compages::Field<T>` of `Fields/`), computed on the CPU or on the GPU, and shown as colours and relief by `scene.show()` ([Renderer, show a field](06-Renderer-Layer.md#show-a-field-heat-maps-and-coloured-reliefs)).

| Demo | Idea | Picture |
|------|------|---------|
| `11a_HeatPlate` | a field in one call: heat on a `Field<float>`, `scene.show(field, look)` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11a_HeatPlate.png" alt="11a_HeatPlate" width="200"> |
| `11b_RippleTank` | waves computed and drawn on the GPU: a compute shader, `scene.show(grid, buffer, look)` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11b_RippleTank.png" alt="11b_RippleTank" width="200"> |
| `11c_FieldProbe` | the altitude under the mouse: `scene.probe(field, look, pixel)`, a `Tooltip` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11c_FieldProbe.png" alt="11c_FieldProbe" width="200"> |
| `11d_BimetalBeam` | a CPU model bends a mesh on the GPU: a bimetal strip heated by a flame, the hook `deform` reads a storage buffer | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11d_BimetalBeam.png" alt="11d_BimetalBeam" width="200"> |
| `11e_Smoke` | smoke rising from a burner: a 2D fluid on the GPU (Stable Fluids), five kernels from one GLSL text with `ShaderSource::define` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11e_Smoke.png" alt="11e_Smoke" width="200"> |
| `11f_Volume` | the warm air of a room: a `Field<float, Grid3D>` seen through (`scene.show(volume, look)`) and cut (`compages::slice()`) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11f_Volume.png" alt="11f_Volume" width="200"> |
| `11g_FlowAroundCylinder` | the wind around a turning column: a `Field<Vector2f>` as arrows (`scene.show(field, ArrowLook)`), streamlines (`compages::streamline()`), the pressure under them; the Magnus effect | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11g_FlowAroundCylinder.png" alt="11g_FlowAroundCylinder" width="200"> |
| `11h_Thermometers` | follow values over time: two thermometers on a heated plate, their curves on a board (`Chart`, a `CanvasTexture`); a click moves one (`scene.probe`) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11h_Thermometers.png" alt="11h_Thermometers" width="200"> |

---

## 🪟 14. Window, ImGui and physics

- Only `examples/Common/Window.cpp` talks to GLFW. The library receives a context that already exists.
- The panels use ImGui (the `docking` branch of the manifest). After them, `gpu::forgetRenderState()` keeps the ImGui state from being taken for the state of the demo.
- There is no physics engine in the library: `07e_ExternalPhysics` shows how to plug one in ([chapter 5](05-World-Layer.md#plug-in-a-physics-engine)).

Next: [10-Debugging.md](10-Debugging.md).
