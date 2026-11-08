---
name: compages-example
description: Add, rename or change a demo of the Compages gallery (examples/, Compages-examples binary, ExampleManifest.hpp, Example interface, screenshots, --check). Use when writing a teaching example, fixing a demo, or regenerating doc screenshots.
---

# Compages gallery demos

One binary, `build/linux-gcc-debug/bin/Compages-examples`, runs every demo. The gallery is a
progressive tutorial: one folder = one chapter = one question, read in order; inside a
folder a letter gives the step (`05b_GameOfLife` = chapter 5, step b).
`01_FirstSteps` `02_ChangingData` `03_Images` `04_Passes` `05_Simulation` (GPU),
`06_Scene` (Renderer, Three.js-like), `07_World`, `08_Assets`, `09_Performance`, `10_Complete`.

## Add a demo

1. `examples/<NN>_<Chapter>/<NN><letter>_<Name>.hpp/.cpp` (SPDX header, `namespace examples`),
   a class deriving `Example` (`examples/Common/Example.hpp`):

   ```cpp
   std::string name() const override;          // returns "NNx_Name", as in the manifest
   std::string description() const override;   // the one idea this demo adds
   compages::Status setUp() override;          // everything that can fail; return prepare()
   void draw(compages::world::ViewFrame const& p_frame) override;
   // optional: controls() (ImGui panel), keyMap(), capturesMouse(), hud(),
   // progress() (a set-up made in a thread: progress bar, --check waits)
   ```

   Members hold the state (`compages::world::World m_world;`
   `compages::renderer::Scene m_scene{ m_world };`), as in `06a_FirstScene`.
2. Include it and add one `X(Type, "NNx_Name", "<Chapter folder>", "<Chapter folder>/NNx_Name.cpp", "<legacy>", <line budget>, "<scope>")`
   line to `COMPAGES_EXAMPLE_MANIFEST` in `examples/Common/ExampleManifest.hpp`
   (the manifest is the list of demos; budget 0 = none).
3. `make && ./build/linux-gcc-debug/bin/Compages-examples NNx_Name`, then
   `./build/linux-gcc-debug/bin/Compages-examples --check NNx_Name` (that demo only; fails on frame
   errors and leftover GPU handles), then `--check` alone before a commit (every demo).
4. Document it in doc/09-Examples.md (and the layer chapter if it introduces a call).

## Rules

- One new idea per demo, explained in `description()` and in comments; keep it
  short (it is shown in the Source panel).
- Report self-checks with `compages::gpu::reportError("...")` (makes `--check`
  fail), like `07a_HeadlessWorld`.
- Only `examples/Common/Window.cpp` talks to GLFW; demos never create windows.
- Assets come from `external/Compages-data/` (`Common/DataPath.hpp`).
- Screenshots: `./build/linux-gcc-debug/bin/Compages-examples --check --shots external/Compages-data/doc/examples`
  (`--shots-with-panels` keeps the panels; add a name for one demo only), in a git clone of Compages-data
  (`git clone https://github.com/Lecrapouille/Compages-data.git external/Compages-data`).
  The docs link the PNGs there by raw.githubusercontent URL; there is no doc/images/.
