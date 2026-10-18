---
name: compages-example
description: Add, rename or change a demo of the Compages gallery (examples/, Compages-examples binary, ExampleManifest.hpp, Example interface, screenshots, --check). Use when writing a teaching example, fixing a demo, or regenerating doc screenshots.
---

# Compages gallery demos

One binary, `build/Compages-examples`, runs every demo. Folders by layer:
`00_GettingStarted/`, `10_ScientificAndCompute/` (GPU), `20_Performance/`,
`30_WorldAndAssets/` (World, then World + Renderer), `50_Complete/` (all three).

## Add a demo

1. `examples/<Folder>/<NN>_<Name>.hpp/.cpp` (SPDX header, `namespace examples`),
   a class deriving `Example` (`examples/Common/Example.hpp`):

   ```cpp
   std::string name() const override;          // returns "NN_Name", as in the manifest
   std::string description() const override;   // the one idea this demo adds
   compages::Status setUp() override;          // everything that can fail; return prepare()
   void draw(compages::world::ViewFrame const& p_frame) override;
   // optional: controls() (ImGui panel), keyMap(), capturesMouse(), hud()
   ```

   Members hold the state (`compages::world::World m_world;`
   `compages::renderer::Scene m_scene{ m_world };`), as in `50_ThreeJsLike`.
2. Include it and add one `X(Type, "NN_Name", "<target>", "<Folder>/NN_Name.cpp", "<legacy>", <line budget>, "<scope>")`
   line to `COMPAGES_EXAMPLE_MANIFEST` in `examples/Common/ExampleManifest.hpp`
   (the manifest is the list of demos; budget 0 = none).
3. `make -j"$(nproc)" && ./build/Compages-examples NN_Name`, then
   `./build/Compages-examples --check` (fails on frame errors and leftover GPU handles).
4. Document it in doc/09-Examples.md (and the layer chapter if it introduces a call).

## Rules

- One new idea per demo, explained in `description()` and in comments; keep it
  short (it is shown in the Source panel).
- Report self-checks with `compages::gpu::reportError("...")` (makes `--check`
  fail), like `30_HeadlessWorld`.
- Only `examples/Common/Window.cpp` talks to GLFW; demos never create windows.
- Assets come from `external/Compages-data/` (`Common/DataPath.hpp`).
- Screenshots: `./build/Compages-examples --check --shots external/Compages-data/doc/examples`
  (`--shots-with-panels` keeps the panels). Only five images are copied into
  `doc/images/`.
