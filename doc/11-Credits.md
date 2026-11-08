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

- `10e_Archipelago` is a new take on Chréage, the mindscape of Idriss J. Aberkane ("Mind Ergonomy for the Knowledge Economy", PhD thesis, 2016, chapter 11; the WebGL proof of concept by Gabriel Uribe, first made with Unity, then taken up by Stigmee). Its islands follow "Polygonal Map Generation for Games" (Amit Patel, Red Blob Games, 2010), as the thesis does. The relief follows the stream power law with uplift as in "Large Scale Terrain Generation from Tectonic Uplift and Fluvial Erosion" (Cordonnier, Braun, Cani, Benes, Galin, Peytavie, Guérin, 2016), solved by the implicit scheme of Braun and Willett (2013), with the pits filled by the priority flood of Barnes, Lehman and Mulla (2014). The waves of the sea are Gerstner waves as in "Effective Water Simulation from Physical Models" (Mark Finch, GPU Gems 1, 2004). The pictures of grass, sand and rock come from ambientCG (CC0). The rivers widen as Leopold and Maddock (1953) measured, and meander with the wavelength of Leopold and Wolman (1960). The flight of `RigController::flyTo` follows "Smooth and efficient zooming and panning" (Jarke J. van Wijk and Wim A. A. Nuij, 2003), as d3-zoom does.
- `10d_EarthMoon` wears NASA pictures (public domain): the Blue Marble of the Earth (Scientific Visualization Studio, [svs.gsfc.nasa.gov/2915](https://svs.gsfc.nasa.gov/2915)) and the colour map of the Moon from the Lunar Reconnaissance Orbiter Camera (CGI Moon Kit, [svs.gsfc.nasa.gov/4720](https://svs.gsfc.nasa.gov/4720)). The reversed depth follows the article "Depth Precision Visualized" (Nathan Reed) and the logarithmic depth example of three.js.

---

## 📦 3. Third-party libraries

The list and the install are in [chapter 2](02-Getting-Started.md#-2-build). In short: EnTT, units, cgltf, nlohmann/json, stb, pugixml, ImGui (gallery), glad (private to the backend), GLFW (gallery and tests).

Demo assets: [Compages-data](https://github.com/Lecrapouille/Compages-data), fetched by `make` with the other third parties.

---

## 📝 4. License

Compages is dual-licensed: GNU GPL v3 or later, or a commercial license for those who cannot use the GPL. See [LICENSING.md](../LICENSING.md). The scripts of `.makefile` carry their own header (MIT for the third-party download scripts).
