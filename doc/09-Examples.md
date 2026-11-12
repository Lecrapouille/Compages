# 🖼 9. Examples: the gallery

One binary, `Compages-examples`, holds every demo: viewport, list, inspector, source, console, GPU leak checks. It is the living manual of the project. The sources are written to be read in the order of the folders; each `description()` explains the idea the demo adds.

```sh
make -j"$(nproc)"
./build/linux-gcc-debug/bin/Compages-examples
./build/linux-gcc-debug/bin/Compages-examples 01c_Triangle
./build/linux-gcc-debug/bin/Compages-examples --check
```

For a first visit, start with `06a_FirstScene`: predict what changing the colour or rotation speed will do, read the Source panel, then edit the file and rebuild with `make`. The panel displays source; changes are made in your editor. **F5 restarts a demo; it does not recompile C++.**

Choose a short route before exploring the whole catalog:

| Goal | Try these demos | What to watch |
|------|-----------------|---------------|
| Build a visible scene | `06a_FirstScene` → `06b_Transforms` → `06c_Hierarchy` → `08a_GltfModel` | Shapes, placement, parent-child movement, then files |
| Simulate objects | `07a_HeadlessWorld` → `07b_Routines` → `07c_MovingRobot` | Data loops, per-object code, then joints |
| Learn GPU drawing | `01c_Triangle` → `02b_DynamicGeometry` → `03c_DepthAndTransforms` | Named inputs, changed vertices, then 3D |
| Show scientific data | `11a_HeatPlate` → `11c_FieldProbe` → `11h_Thermometers` | A field, a value under the mouse, then a chart |

Some demos deliberately show the CPU and GPU copies separately. Pause with F6 and advance with F7 to see one step at a time. Use [chapter 12](12-Going-Further.md) before the large complete demos.

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
| `06m_ManyLights` | two hundred coloured lamps in one picture: the Renderer sorts them into clusters (tiles × depth slices), each pixel reads the lamps of its cluster | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06m_ManyLights.png" alt="06m_ManyLights" width="200"> |
| `06n_VolumetricFog` | fog that the light crosses: sun shafts through the slats of a roof, halos and shadows of lamps in the air (froxels: a 3D picture that follows the camera, filled by two compute shaders) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06n_VolumetricFog.png" alt="06n_VolumetricFog" width="200"> |

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
| `10b_DoomLike` | the game: photographed materials and props of Poly Haven (CC0), a bolt-action rifle, glTF soldiers and robots, particles, fog, shadows of the wall lamps, two levels | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10b_DoomLike.png" alt="10b_DoomLike" width="200"> |
| `10c_SweatAndDecals` | a head that sweats, with decals: GPU passes in the UV map of a mesh | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10c_SweatAndDecals.png" alt="10c_SweatAndDecals" width="200"> |
| `10d_EarthMoon` | a rocket from Kourou to the Moon at the real scale: logarithmic zoom (Shift: faster), reversed depth, floating origin (untick it in Try it to see the 32 m steps); NASA textures | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10d_EarthMoon.png" alt="10d_EarthMoon" width="200"> |
| `10e_Archipelago` | a map of knowledge after Chréage: islands of Voronoi cells grown by uplift and river erosion, regions and parts made of cells, rivers that dig their valleys, finer tiles near the camera, loaded behind a progress bar; an animated sea, boats, gulls and fish; the web links in a tooltip | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10e_Archipelago.png" alt="10e_Archipelago" width="200"> |
| `10f_FlowingWater` | water flowing round what stands in it, by a flow map (Portal 2, GDC 2011): baked on the CPU from the scene, a `Field<Vector4f>` sent as it is to the GPU; ripples and foam moved in two phases blended by a noise; crates carried by the same `Field`; a pipe that spits water | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10f_FlowingWater.png" alt="10f_FlowingWater" width="200"> |

`10b_DoomLike`: click to capture the mouse. WASD or arrows to move, Shift to run, Q and E to turn, left click to shoot, R to reload, F for the wall lamp aimed at, L for the lantern. Green boxes are rounds, white kits heal. The green light leaves the level. The surfaces are `DoomLike/Materials` (a hook `main`: colour, normal and roughness maps projected along the three axes, the lamps glinting where the roughness map says smooth); the textures and models are in `polyhaven/` of Compages-data.

`10c_SweatAndDecals`: the head of the three.js `webgl_decals` demo (Lee Perry-Smith). The mesh is drawn into its own UV map, and four passes make the sweat and the decals there: [chapter 13 §1](13-Complete-Demos.md#-1-10c_sweatanddecals-a-simulation-on-the-skin) rebuilds it step by step. Left click paints a decal or drops water (Try it panel), right drag turns. Pitch and Roll tilt the head, and the drops change their way.

---

`10e_Archipelago` is a new take on Chréage, the map of knowledge of I. Aberkane's thesis: islands of Voronoi cells grown by uplift and river erosion around the graph of your bookmarks ([IslandedBrowser](https://github.com/Lecrapouille/IslandedBrowser)), made in a thread behind a progress bar. [Chapter 13 §2](13-Complete-Demos.md#-2-10e_archipelago-a-world-made-from-a-graph) rebuilds it step by step. Try it: Seed, Erosion steps, Generate; `COMPAGES_ARCHIPELAGO` names another layout file.

The camera is a `Rig`: left drag moves the map, right drag turns it, the wheel zooms toward the mouse. Over the picture, the names are written as Chréage writes them (`Archipelago/Signs`): an island or a region on a rounded sign, a tab of the colour of its island on its left, raised on a thin stem over its place (higher, or left out, when it would cover another one; the islands from afar, the nearest regions closer); the links of the region under the mouse or chosen on a circle around it, each joined to its pin by a dotted line, in the order of the pins round the circle so that the lines hardly cross. Also over the picture, `Example::overlay()` lets the demo draw Dear ImGui windows (the pointer over them and the keys typed into them do not reach `draw()`); `Signs` draws with `ImGui::GetWindowDrawList()` on the picture, and `Archipelago/Panels` opens the windows of the interface of IslandedBrowser: a tooltip next to the pointer (the title, the URL, where it is, what a click does), the details of the choice on the right (its links: a click goes there, Ctrl + click opens), a search of the titles and URLs (`/` or Ctrl + F), a status bar with the keys of the moment, short notifications, and a help (`H`). A click selects, a double click opens a link or flies to a region (`RigController::flyTo`), a middle or Ctrl click opens in the web browser (`$BROWSER`, else firefox; a region: its first 20 links); Enter opens the selection, Z flies to it, Home to the whole map, N, B and Space show the names, the borders, the animals.

`10f_FlowingWater`: the flow maps of Portal 2 (A. Grimes, GDC 2011; A. Vlachos, SIGGRAPH 2010) in a concrete pool under sodium lamps, with a pipe spitting water. [Chapter 13 §3](13-Complete-Demos.md#-3-10f_flowingwater-water-that-flows-round-things) rebuilds it step by step. Try it → Show goes through each step of the paper (the flow map, the pop, the crawling edges, two phases, the masks); Arrows and Features show the flow and its upwellings and vortices; the Bake sliders make the flow map again. A left click on the water adds an upwelling (or a vortex), right drag turns the camera.

## 🔬 13. 11 — Science: fields, computed and shown

Values on a grid (`compages::Field<T>` of `Fields/`), computed on the CPU or on the GPU, and shown as colours and relief by `scene.show()` ([Renderer, show a field](06-Renderer-Layer.md#show-a-field-heat-maps-and-coloured-reliefs)). [Chapter 14](14-Science-With-Fields.md) explains each demo: its equation, its loop on the grid, its calls.

| Demo | Idea | Picture |
|------|------|---------|
| `11a_HeatPlate` | a field in one call: heat on a `Field<float>`, `scene.show(field, look)` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11a_HeatPlate.png" alt="11a_HeatPlate" width="200"> |
| `11b_RippleTank` | waves computed and drawn on the GPU: a compute shader, `scene.show(grid, buffer, look)` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11b_RippleTank.png" alt="11b_RippleTank" width="200"> |
| `11c_FieldProbe` | the altitude under the mouse: `scene.probe(field, look, pixel)`, a `Tooltip` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11c_FieldProbe.png" alt="11c_FieldProbe" width="200"> |
| `11d_BimetalBeam` | a CPU model bends a mesh on the GPU: a bimetal strip heated by a flame, the hook `deform` reads a storage buffer | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11d_BimetalBeam.png" alt="11d_BimetalBeam" width="200"> |
| `11e_Smoke` | smoke rising from a burner: a 2D fluid on the GPU (Stable Fluids), five kernels from one GLSL text with `ShaderSource::define` | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11e_Smoke.png" alt="11e_Smoke" width="200"> |
| `11f_Volume` | the warm air of a room: a `Field<float, Grid3D>` seen through (`scene.show(volume, look)`), cut (`compages::slice()`) and its surface at 30 degrees (`compages::isosurface()`) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11f_Volume.png" alt="11f_Volume" width="200"> |
| `11g_FlowAroundCylinder` | the wind around a turning column: a `Field<Vector2f>` as arrows (`scene.show(field, ArrowLook)`), streamlines (`compages::streamline()`), the pressure under them; the Magnus effect; a regular `Grid2D` or a `CurvilinearGrid` fitted round the column (an O-grid) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11g_FlowAroundCylinder.png" alt="11g_FlowAroundCylinder" width="200"> |
| `11h_Thermometers` | follow values over time: two thermometers on a heated plate, their curves on a board (`Chart`, a `CanvasTexture`); a click moves one (`scene.probe`) | <img src="https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11h_Thermometers.png" alt="11h_Thermometers" width="200"> |

---

## 🪟 14. Window, ImGui and physics

- Only `examples/Common/Window.cpp` talks to GLFW. The library receives a context that already exists.
- The panels use ImGui (the `docking` branch of the manifest). After them, `gpu::forgetRenderState()` keeps the ImGui state from being taken for the state of the demo.
- There is no physics engine in the library: `07e_ExternalPhysics` shows how to plug one in ([chapter 5](05-World-Layer.md#plug-in-a-physics-engine)).

Next: [10-Debugging.md](10-Debugging.md).
