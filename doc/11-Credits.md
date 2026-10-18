# 🙏 11. Credits

## 📜 1. Where Compages comes from

Compages is the new name of **OpenGLCppWrapper**, a C++ wrapper around OpenGL. Its first version served scientific visualization, close to [SimTaDyn](https://github.com/Lecrapouille/SimTaDyn). The first idea came from the Python API of [Glumpy](https://github.com/glumpy/glumpy) (gloo): the shader names its inputs, uploads to the GPU wait until the draw, and the application never chains `glBind*` calls.

That design also had a scene node tree with a virtual `onDraw()` method. Handy for demos, it became painful to share a mesh between shaders, to move a node without breaking its buffers, and to run a simulation in a test without an OpenGL context. It was also slow: virtual calls everywhere, data not interleaved, memory not friendly to the CPU cache, materials hard to use. The project stopped for several years.

Two observations led to the three layers of [chapter 1](01-Introduction.md#-3-three-layers):

1. **One mesh, several shaders.** "One vertex array per program" copied the vertices for nothing. [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol) and [regl](https://github.com/regl-project/regl) declare the vertex layout in application code. Compages does the same with `Buffer<Vertex>` and `Pipeline`, and keeps `Drawable` for the one-shader case (`05a_MultiPassMesh`).
2. **Simulating and showing are two jobs.** Mixing GPU handles and the scene graph in the same nodes makes lifetimes hard. Splitting the world (what exists and moves), the renderer (catalog and snapshots) and the GPU (shaders and uploads) follows the generational ids of ECS engines such as [Bevy](https://github.com/bevyengine/bevy), and the short scene code of [Three.js](https://github.com/mrdoob/three.js).

LLM coding assistants made the revival possible, by helping fix all these issues in a reasonable time.

---

## 💡 2. Inspirations

**OpenGL**

- [Glumpy](https://github.com/glumpy/glumpy) — shaders, names, `draw`
- [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol), [regl](https://github.com/regl-project/regl) — vertex layout on the application side

**Scene graphs**

- [Three.js](https://github.com/mrdoob/three.js) — short scene building
- [Bevy](https://github.com/bevyengine/bevy) — ECS, extracting a view for drawing
- [scg3](https://github.com/vahlers/scg3) — a scene graph for teaching OpenGL

---

## 📦 3. Third-party libraries

The list and the install are in [chapter 2](02-Getting-Started.md#-2-build). In short: EnTT, units, cgltf, nlohmann/json, stb, pugixml, ImGui (gallery), glad (private to the backend), GLFW (gallery and tests).

Demo assets: [Compages-data](https://github.com/Lecrapouille/Compages-data), cloned into `external/Compages-data/` by `make download-external-libs`.

---

## 📝 4. License

Compages is dual-licensed: GNU GPL v3 or later, or a commercial license for those who cannot use the GPL. See [LICENSING.md](../LICENSING.md). The scripts of `.makefile` carry their own header (MIT for the third-party download scripts).
