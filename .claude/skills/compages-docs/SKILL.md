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
| README.md | what it is for (no jargon), the story, why Compages (comparison), code at a glance, quick start, doc table |
| doc/README.md | index: Learn (1–3, 12–14), layers (4–6), Reference (7–11), quick paths |
| 01-Introduction.md | glossary, three layers and what each leaves out, error moments, OpenGL versions |
| 02-Getting-Started.md | tools per OS, build, third parties, gallery, tests/CI, install |
| 03-Tutorial.md | one program per layer (§2 is compiled and run headless) |
| 04/05/06-*-Layer.md | one layer each: concepts → calls → demos |
| 07-Architecture.md | includes, folders, frame flow, entry points |
| 08-API-Reference.md | calls of the three layers |
| 09-Examples.md | gallery window, options, every demo |
| 10-Debugging.md | library/driver messages, apitrace, RenderDoc, CI, common problems |
| 11-Credits.md | technical lessons, inspirations, third parties, license (the story is in README.md) |
| 12-Going-Further.md | the tutorial continued: one important call per step (own shader, pictures, meshes, see-through, deform, Field, fromField, passes, picking), from real demo code |
| 14-Science-With-Fields.md | the 11_Science demos: equations (LaTeX ```math blocks), discrete loops, mermaid diagrams, the Fields calls |
| techniques/README.md | the list of techniques (lettier order, status ✅ 🕓 🚧) and how a page reads |
| techniques/<slug>.md | one effect per page (cel-shading, bloom, flow-mapping…), the template below |
| 13-Complete-Demos.md | 10c, 10e, 10f rebuilt step by step (the technique lives here; 09 keeps the idea, the keys and a link) |

Put a fact in one place and link to it. README.md and doc/README.md hold the
chapter tables: keep them in sync. Code comments point to chapters
(`doc/01-Introduction.md`, `doc/06-Renderer-Layer.md §5`): update them on rename.

## Technique pages (doc/techniques/)

One effect, one page, one demo. Inspired by lettier's *3D Game Shaders For
Beginners* for the structure only: never copy its text or pictures. Order:
title, nav line (`[◀ Prev](x.md) · [⏫ Techniques](README.md) · [Next ▶](y.md)`),
screenshot, card table (Demo | Layer | Your C++ | Your GLSL | Passes added),
then 🎯 The problem · 💡 The idea (maths + one mermaid) · 🧩 In Compages (real
lines of the demo) · 🔍 The shader · 📏 Cost (passes, textures, memory) ·
🚧 Limits · 🧪 Try it (the demo's own controls) · 📁 Source, and the nav line
again. "Your C++" counts only the lines that switch the effect on and set it.
A new page: add it to techniques/README.md (status ✅), relink its neighbours'
nav lines, link it from the demo's row in 09-Examples.md.

## Rules

- The README sells before it teaches: what the library is for in plain words
  (no jargon, no code), the story (why it exists, what was wrong with the
  others), why it is better (comparison table), and only then code, quick
  start and the doc table. Never open a README with API details.
- Code excerpts come from real files (`examples/...`) or are compiled: World
  snippets build headless with
  `g++ -std=c++20 x.cpp $(PKG_CONFIG_PATH=build/linux-gcc-debug/lib/pkgconfig pkg-config --cflags Compages) build/linux-gcc-debug/lib/libCompages.a`;
  paste the real output.
- Namespaces in snippets: `compages::Vector3f`, `compages::Frame`,
  `compages::world::...`, `compages::renderer::...`, `compages::gpu::...`.
- Each chapter: what you will learn → content/tutorial → reference tables → next.
- Maths: display equations in ```` ```math ```` blocks, inline as `$...$` (GitHub renders both);
  in Python string edits, beware `\f`, `\t`, `\r` in LaTeX (`\frac`, `\text`, `\right`): use raw strings.
  Diagrams: ```` ```mermaid ```` (flowchart, classDiagram), short labels, `<br/>` for new lines.
- Do not document what does not exist (check the header first); mark future
  work as such.
- Link anchors: GitHub drops emojis from heading ids (`## 🚀 2. Build` →
  `#-2-build`). Do not use emojis with U+FE0F (⚠️ 🖥️ 🛠️ 🏗️ …) in headings:
  the selector stays in the id.
- Logos in `doc/logos/` (`Compages.png` plain, `compages-layers.png` with the
  three layer captions); screenshots only in Compages-data `doc/examples/<demo>.png`, linked as
  `https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/<demo>.png`.
