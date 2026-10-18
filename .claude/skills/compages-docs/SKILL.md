---
name: compages-docs
description: Edit or extend the Compages documentation (README.md, doc/ numbered chapters 01–11, doc/README.md index, examples/README.md, logos and images). Use when documenting a call, a layer feature, a demo, the build, or when asked to fix or rewrite docs.
---

# Compages documentation

Audience: developers with no 3D or OpenGL background, often French. Simple
English: short sentences, present tense, common words, no idioms; define each
3D word once (01-Introduction.md §2 is the glossary). Emojis in headings, as
in the other chapters; 🟧 GPU, 🟦 World, 🟩 Renderer.

## Map (general → specific)

| File | Holds |
|------|-------|
| README.md | pitch, three-layer table, code at a glance, quick start, doc table |
| doc/README.md | index: Learn (1–3), layers (4–6), Reference (7–11), quick paths |
| 01-Introduction.md | glossary, three layers and what each leaves out, error moments, OpenGL versions |
| 02-Getting-Started.md | tools per OS, build, third parties, gallery, tests/CI, install |
| 03-Tutorial.md | one program per layer (§2 is compiled and run headless) |
| 04/05/06-*-Layer.md | one layer each: concepts → calls → demos |
| 07-Architecture.md | includes, folders, frame flow, entry points |
| 08-API-Reference.md | calls of the three layers |
| 09-Examples.md | gallery window, options, every demo |
| 10-Debugging.md | library/driver messages, apitrace, RenderDoc, CI, common problems |
| 11-Credits.md | history, inspirations, third parties, license |

Put a fact in one place and link to it. README.md and doc/README.md hold the
chapter tables: keep them in sync. Code comments point to chapters
(`doc/01-Introduction.md`, `doc/06-Renderer-Layer.md §5`): update them on rename.

## Rules

- Code excerpts come from real files (`examples/...`) or are compiled: World
  snippets build headless with
  `g++ -std=c++20 x.cpp -I include -isystem external/units/include -isystem external/entt/src build/libCompages.a`;
  paste the real output.
- Namespaces in snippets: `compages::core::Vector3f`, `compages::core::Frame`,
  `compages::world::...`, `compages::renderer::...`, `compages::gpu::...`.
- Each chapter: what you will learn → content/tutorial → reference tables → next.
- Do not document what does not exist (check the header first); mark future
  work as such.
- Link anchors: GitHub drops emojis from heading ids (`## 🚀 2. Build` →
  `#-2-build`). Do not use emojis with U+FE0F (⚠️ 🖥️ 🛠️ 🏗️ …) in headings:
  the selector stays in the id.
- Logos in `doc/logos/` (`Compages.png` plain, `compages-layers.png` with the
  three layer captions); screenshots in `doc/images/`.
