# 🙏 11. Credits

## 📜 1. Where Compages comes from

The story (SimTaDyn maps, OpenGL 3.3 core, Glumpy, OpenGLCppWrapper, then Compages) is told in the [README](../README.md#-the-story). Two technical lessons of OpenGLCppWrapper shaped the three layers of [chapter 1](01-Introduction.md#-3-three-layers):

1. **One mesh, several shaders.** "One vertex array per program" copied the vertices for nothing. [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol) and [regl](https://github.com/regl-project/regl) declare the vertex layout in application code. Compages does the same with `VertexBuffer<Vertex>`: several `Drawable` borrow one buffer (`04c_MultiPassMesh`), and `Pipeline` does the same one level lower.
2. **Simulating and showing are two jobs.** OpenGLCppWrapper had a node tree with a virtual `onDraw()`: handy for demos, but GPU buffers lived in the nodes, a node could not move without breaking them, and nothing ran in a test without an OpenGL context. Splitting the world (what exists and moves), the renderer (catalog and snapshots) and the GPU (shaders and uploads) follows the generational ids of ECS engines such as [Bevy](https://github.com/bevyengine/bevy), and the short scene code of [Three.js](https://github.com/mrdoob/three.js).

---

## 💡 2. Inspirations

**OpenGL**

- [Glumpy](https://github.com/glumpy/glumpy) — shaders, names, `draw`
- [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol), [regl](https://github.com/regl-project/regl) — vertex layout on the application side

**Scene graphs**

- [Three.js](https://github.com/mrdoob/three.js) — short scene building
- [Bevy](https://github.com/bevyengine/bevy) — ECS, extracting a view for drawing
- [scg3](https://github.com/vahlers/scg3) — a scene graph for teaching OpenGL

**Demos ported from other projects**

- `05e_SpiralGalaxy` is a port of the [galaxy example of Glumpy](https://github.com/glumpy/glumpy/blob/master/examples/galaxy.py), by Nicolas P. Rougier (BSD license). The model, its constants, the four kinds of bodies (stars, dust, hydrogen clouds and their cores) and the look come from it. Glumpy took the model from the [Galaxy Renderer](https://github.com/beltoforion/Galaxy-Renderer) of Ingo Berg, a simulation of the density wave theory of spiral arms. The colours of the stars are the black-body spectrum of John Walker (public domain), as Glumpy uses them. Compages only changes how it is drawn: the vertex shader computes the orbits from the time, where Glumpy moved them with numpy on the CPU.

- `10e_Archipelago` is a new take on Chréage, the mindscape of Idriss J. Aberkane ("Mind Ergonomy for the Knowledge Economy", PhD thesis, 2016, chapter 11; the WebGL proof of concept by Gabriel Uribe, first made with Unity, then taken up by Stigmee). Its contents and their graph come from [IslandedBrowser](https://github.com/Lecrapouille/IslandedBrowser) (bookmarks of Firefox grouped by similarity, then a force directed layout). Its islands follow "Polygonal Map Generation for Games" (Amit Patel, Red Blob Games, 2010), as the thesis does. The relief follows the stream power law with uplift as in "Large Scale Terrain Generation from Tectonic Uplift and Fluvial Erosion" (Cordonnier, Braun, Cani, Benes, Galin, Peytavie, Guérin, 2016), solved by the implicit scheme of Braun and Willett (2013), with the pits filled by the priority flood of Barnes, Lehman and Mulla (2014). The waves of the sea are Gerstner waves as in "Effective Water Simulation from Physical Models" (Mark Finch, GPU Gems 1, 2004). The pictures of grass, sand and rock come from ambientCG (CC0). The rivers widen as Leopold and Maddock (1953) measured, and meander with the wavelength of Leopold and Wolman (1960). The flight of `RigController::flyTo` follows "Smooth and efficient zooming and panning" (Jarke J. van Wijk and Wim A. A. Nuij, 2003), as d3-zoom does.
- `10d_EarthMoon` wears NASA pictures (public domain): the Blue Marble of the Earth (Scientific Visualization Studio, [svs.gsfc.nasa.gov/2915](https://svs.gsfc.nasa.gov/2915)) and the colour map of the Moon from the Lunar Reconnaissance Orbiter Camera (CGI Moon Kit, [svs.gsfc.nasa.gov/4720](https://svs.gsfc.nasa.gov/4720)). The reversed depth follows the article "Depth Precision Visualized" (Nathan Reed) and the logarithmic depth example of three.js.

**Techniques of games and papers, rebuilt in the demos**

The demos rebuild published techniques from the papers and talks below, with code, pictures and scenes of their own: no asset, level or look of the games is copied.

- `10b_DoomLike` — textures and models of [Poly Haven](https://polyhaven.com) (CC0): the textures `concrete_panels`, `metal_plate`, `rusty_metal_grid`, `rusty_corrugated_iron`, `concrete_floor_worn_001`, `concrete_slab_wall`; the models `bolt_action_rifle_7_62`, `ammo_box`, `medical_box`, `barrel_03`, `old_military_crate`, `industrial_caged_sconce`, `mounted_fluorescent_lights`. Triplanar normal mapping: B. Golus, "Normal Mapping for a Triplanar Shader" (2017). The soldier (`Soldier.glb`) and the robot (`RobotExpressive.glb`, by Tomás Laulhé, CC0) come from the examples of three.js.
- `10f_FlowingWater` — the flow maps of Portal 2, by Valve:
  - Alex Grimes, "Making and Using Non-Standard Textures", GDC 2011 ([PDF](https://cdn.fastly.steamstatic.com/apps/valve/2011/gdc_2011_grimes_nonstandard_textures.pdf)): baking a flow map from the level (footprint, upwellings, vortices, deflection, slowing at the edges, foam), the noise that offsets the phase, the two phases, the foam distorted by the ripples, a flow that reacts to the game;
  - Alex Vlachos, "Water Flow in Portal 2", SIGGRAPH 2010 ([PDF](http://www.valvesoftware.com/publications/2010/siggraph2010_vlachos_waterflow.pdf)): the shader of the flowing water;
  - every publication of Valve: [valvesoftware.com/publications](https://www.valvesoftware.com/fr/publications).
- `10e_Archipelago` — Gerstner waves: M. Finch, "Effective Water Simulation from Physical Models", GPU Gems 1, chapter 1 (NVIDIA, 2004); erosion: G. Cordonnier et al., "Large Scale Terrain Generation from Tectonic Uplift and Fluvial Erosion" (Eurographics 2016), J. Braun and S. Willett (Geomorphology 2013), R. Barnes et al., "Priority-Flood" (Computers & Geosciences 2014); polygon maps: A. Patel ([Red Blob Games](https://www.redblobgames.com/maps/terrain-from-noise/)).
- `10c_SweatAndDecals` — the head and the decals of the three.js `webgl_decals` example (Lee Perry-Smith head, CC BY 3.0); normal mapping without tangents: C. Schüler, "Followup: Normal Mapping Without Precomputed Tangents" (2013).
- `06m_ManyLights` and every lit scene — clustered lighting: O. Olsson, M. Billeter and U. Assarsson, "Clustered Deferred and Forward Shading", HPG 2012. The metallic-roughness model of glTF: B. Karis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013.
- `11e_Smoke` — J. Stam, "Stable Fluids", SIGGRAPH 1999.
- `11d_BimetalBeam` — S. Timoshenko, "Analysis of Bi-Metal Thermostats", J. Opt. Soc. Am. 11, 1925.
- `11f_Volume` — marching tetrahedra (A. Doi and A. Koide, IEICE 1991); emission and absorption: N. Max, "Optical Models for Direct Volume Rendering" (IEEE TVCG 1995).

---

## 📦 3. Third-party libraries

The list and the install are in [chapter 2](02-Getting-Started.md#-2-build). In short: EnTT, units, cgltf, nlohmann/json, stb, pugixml, ImGui (gallery), glad (private to the backend), GLFW (gallery and tests).

Demo assets: [Compages-data](https://github.com/Lecrapouille/Compages-data), fetched by `make` with the other third parties.

---

## 📝 4. License

Compages is dual-licensed: GNU GPL v3 or later, or a commercial license for those who cannot use the GPL. See [LICENSING.md](../LICENSING.md). The scripts of `.makefile` carry their own header (MIT for the third-party download scripts).
