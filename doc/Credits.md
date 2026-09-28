# Credits

## Project Origin

The repository is named **OpenGLCppWrapper**. The documented product is called **Compages**: a C++20 library built around OpenGL 4.5.

The code’s first incarnation focused on scientific visualization, inspired by projects like [SimTaDyn](https://github.com/Lecrapouille/SimTaDyn). The initial impulse came from the Python API [Glumpy](https://github.com/glumpy/glumpy) (gloo): the shader names its input, GPU uploads are deferred, and application code doesn’t chain together `glBind*` calls.

That initial design also included a scene node tree, with a virtual `onDraw()` method. This was convenient for demos, but became unwieldy when reusing a mesh between multiple shaders, reparenting without breaking buffers, or running simulations in tests without a GL context.

Two main observations led to the three-layer architecture described in [Philosophy.md](Philosophy.md):

1. **One mesh, multiple shaders.** The “one VAO per program” model duplicated vertices unnecessarily. Other frameworks, such as [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol), and [regl](https://github.com/regl-project/regl), handle vertex layout in application code. Compages follows a similar approach with `Buffer<Vertex>` and `Pipeline`, and keeps `Drawable` for the single-shader case. Example: `05a_MultiPassMesh`.

2. **Simulation and visualization are separate concerns.** Mixing GPU handles and scene graph in the same node structure complicates resource lifetimes. Separating the world (things that exist and move), the renderer (asset catalog and snapshots), and the GPU (shaders and flush operations) follows the generational model of ECS engines like [Bevy](https://github.com/bevyengine/bevy) and the declarative structure of [Three.js](https://github.com/mrdoob/three.js).

The folder layout is described in [Architecture.md](Architecture.md).

## Inspirations

**Related to OpenGL**

- [Glumpy](https://github.com/glumpy/glumpy) — shaders, naming, and `draw`
- [bgfx](https://github.com/bkaradzic/bgfx), [sokol_gfx](https://github.com/floooh/sokol), [regl](https://github.com/regl-project/regl) — vertex layout handled on the application side

**Related to Scene Graphs**

- [Three.js](https://github.com/mrdoob/three.js) — concise scene construction
- [Bevy](https://github.com/bevyengine/bevy) — ECS, extracting a view for rendering
- [scg3](https://github.com/vahlers/scg3) — scene graph for OpenGL teaching

## Third-party Libraries

A full list of dependencies and installation instructions is in [Install.md](Install.md). In summary: EnTT, units, cgltf, nlohmann/json, stb, pugixml, ImGui (gallery), glad (backend private), GLFW (examples and tests).

Demo assets: [Compages-data](https://github.com/Lecrapouille/Compages-data), cloned to `external/Compages-data/` via `make download-external-libs`.

## License

GNU General Public License v3. The license text appears in the headers of the source files. Scripts in `.makefile` carry their own header (MIT license for third-party download scripts).
