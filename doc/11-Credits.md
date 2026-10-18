# 🙏 11. Credits

## 📜 1. Where Compages comes from

The story (SimTaDyn maps, OpenGL 3.3 core, Glumpy, OpenGLCppWrapper, then Compages) is told in the [README](../README.md#-the-story). Two technical lessons of OpenGLCppWrapper shaped the three layers of [chapter 1](01-Introduction.md#-3-three-layers):

1. **One mesh, several shaders.** "One vertex array per program" copied the vertices for nothing. [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol) and [regl](https://github.com/regl-project/regl) declare the vertex layout in application code. Compages does the same with `Buffer<Vertex>` and `Pipeline`, and keeps `Drawable` for the one-shader case (`05a_MultiPassMesh`).
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

---

## 📦 3. Third-party libraries

The list and the install are in [chapter 2](02-Getting-Started.md#-2-build). In short: EnTT, units, cgltf, nlohmann/json, stb, pugixml, ImGui (gallery), glad (private to the backend), GLFW (gallery and tests).

Demo assets: [Compages-data](https://github.com/Lecrapouille/Compages-data), cloned into `external/Compages-data/` by `make download-external-libs`.

---

## 📝 4. License

Compages is dual-licensed: GNU GPL v3 or later, or a commercial license for those who cannot use the GPL. See [LICENSING.md](../LICENSING.md). The scripts of `.makefile` carry their own header (MIT for the third-party download scripts).
