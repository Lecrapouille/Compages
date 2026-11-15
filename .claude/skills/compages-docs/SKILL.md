---
name: compages-docs
description: Write, restructure or review the Compages documentation (README.md, doc/ in Diátaxis folders tutorials/ guides/ concepts/ reference/, plus techniques/ and case-studies/, examples/README.md, screenshots in Compages-data). Use when documenting a call, a feature, a demo, the build, or when asked to fix, rewrite or review docs.
---

# Compages documentation

Readers: C++ developers with no 3D or OpenGL background, often French.
The goal: pages that read as if an experienced developer explained what he
understood, not as if a tool described a repository.

## Map: one page, one job (Diátaxis)

| Folder | Kind | The reader wants to… | A page is |
|--------|------|----------------------|-----------|
| `doc/tutorials/` | tutorial | learn, by doing, in order | one step of the **robot workshop** (`examples/00_Workshop/`): a result on screen, then the next step |
| `doc/guides/` | how-to | do one task now | goal → steps → code → result picture → see also |
| `doc/concepts/` | explanation | understand why | the problem, the idea, the choice made and what it costs |
| `doc/reference/` | reference | look up a fact | tables of calls, the gallery list, credits; no story |
| `doc/techniques/` | technique | reproduce one effect | the fixed template below, one demo per page |
| `doc/case-studies/` | case study | see a whole demo built | 10c, 10e, 10f step by step; one fiche per `11_Science` demo |

`doc/README.md` is the map: three paths (use Compages · make effects and
simulations · understand and extend the engine) over these folders. Each
folder has a `README.md` listing its pages in reading order. The root
`README.md` links only to `doc/README.md` and the first tutorial.

Do not mix the kinds. A tutorial does not explain theory at length: it links
to a concept. A guide does not teach from zero: it links to the tutorial. A
reference page has no narrative. When a page starts doing two jobs, split it.

Put a fact in one place and link to it. Code comments point to pages
(`doc/concepts/three-layers.md`): update them when a page moves
(`grep -rn "doc/" include src examples tests .claude AGENTS.md README.md`).

## Writing charter

- **Start from the problem**, never from a list of features. First sentence:
  a situation the reader knows ("You want a river that flows round rocks").
- **One running example** for the tutorials: the robot workshop grows by one
  idea per page (table → arm → heat → look → camera). Guides and concepts
  reuse it when they can.
- **Real code only**: excerpts from `examples/` or compiled (see Checks).
  Cut with `// ...`, never invent a call. Check the header before naming a
  call; mark future work as future.
- **Explain why**: each choice of architecture says what it buys and what it
  costs ("the world never sees a GPU type, so it runs in a test without a
  screen").
- **Write plainly**: short sentences, present tense, common words, no idioms
  (French readers). Define a 3D word once, in `concepts/glossary.md`, and
  link to it. Prose first; a list only for real enumerations, a table only
  for real comparisons.
- **Never write**: "This section explores…", "In this chapter we will…",
  "Let's dive in", "powerful", "seamless", "robust", "blazing", "simply",
  "just", "easy", "comprehensive", "leverage", absolute claims ("the only
  library that…"). Say what Compages chooses, what it leaves out, when it fits.
- **Keep the author's voice**: dry, concrete, physical comparisons (a shell
  drawn inside out, a picture moved along the water). Do not flatten the
  existing pages into a generic tone.
- **Depth by place**: README welcomes, tutorials hold the hand, concepts and
  architecture are rigorous.

## Pictures

Every tutorial, guide, technique and case study shows at least one picture,
near the top: what the reader will get. More pictures where a step changes
what is seen.

- Screenshots of demos live in Compages-data `doc/examples/<demo>.png`, made by
  `./build/linux-gcc-debug/bin/Compages-examples --check --shots external/Compages-data/doc/examples [demo]`
  and linked as
  `https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/<demo>.png`.
  Look at a new screenshot (Read the PNG) before linking it: framing, nothing
  cut, the idea visible. A demo whose interesting state needs time warms up
  in `setUp()` (as `00c_HotTable`, `11a_HeatPlate`).
- Commit new PNGs in the Compages-data clone too, and say that it must be
  pushed: until then the links are broken on GitHub.
- Diagrams: ```` ```mermaid ```` (flowchart, sequence, classDiagram), short
  labels, `<br/>` for new lines. One diagram per idea, not per section.
- Maths: ```` ```math ```` blocks, inline `$...$`. In Python string edits,
  beware `\f`, `\t`, `\r` in LaTeX (`\frac`, `\text`, `\right`): raw strings.
- Logos in `doc/logos/` (`Compages.png`, `compages-layers.png`).

## Templates

**Tutorial** (`tutorials/NN-name.md`): title · nav line · screenshot of the
step · "You will" (one sentence) · the code of the step (from
`examples/00_Workshop/Workshop/*.cpp`), explained in order · "Run it"
(`Compages-examples 00x_Name`) and what you should see · "Try" (two or three
changes and their effect) · "What happened" (link to the concepts) · nav line.

**Guide** (`guides/name.md`): title · one line goal · picture · steps with
code · common mistakes · see also (concept, reference, demo).

**Concept** (`concepts/name.md`): the problem · the idea (one diagram) ·
how Compages does it · what it costs and what it leaves out · see also.

**Technique** (`techniques/slug.md`): title, nav line
(`[◀ Prev](x.md) · [⏫ Techniques](README.md) · [Next ▶](y.md)`),
screenshot, card table (Demo | Layer | Your C++ | Your GLSL | Passes added),
🎯 The problem · 💡 The idea (maths + one mermaid) · 🧩 In Compages (real lines
of the demo) · 🔍 The shader · 📏 Cost · 🚧 Limits · 🧪 Try it · 📁 Source, nav
line again. "Your C++" counts only the lines that switch the effect on. Lettier's
*3D Game Shaders For Beginners* inspires the list and the shape only: never copy
its text or pictures. A new page: status ✅ in techniques/README.md, relink the
neighbours, link it from the demo's row in reference/gallery.md.

**Case study** (`case-studies/name.md`): the problem → the technique (with its
paper) → the data and how it is made → the shaders and hooks → what Compages
does for you → cost → other ways → extensions. A science fiche: the physical
problem → the equation → the discrete loop → the data → CPU or GPU → the
picture → limits.

## Format

- Emojis in headings, as in the other pages; 🟧 GPU, 🟦 World, 🟩 Renderer.
  GitHub drops emojis from heading ids (`## 🚀 2. Build` → `#-2-build`); never
  use emojis with U+FE0F (⚠️ 🖥️ 🛠️ 🏗️ …) in headings: the selector stays in the id.
- Namespaces in snippets: `compages::Vector3f`, `compages::world::...`,
  `compages::renderer::...`, `compages::gpu::...`.
- Relative links between pages; check that every target exists.

## Checks before a commit

1. World snippets that are not from a demo compile headless:
   `g++ -std=c++20 x.cpp $(PKG_CONFIG_PATH=build/linux-gcc-debug/lib/pkgconfig pkg-config --cflags Compages) build/linux-gcc-debug/lib/libCompages.a`;
   paste the real output.
2. Every relative link resolves (a loop over `](…)` targets with `test -e`).
3. **Fresh reader pass**: reread each new page as a C++ developer who opened
   the repository an hour ago. Is the first paragraph a problem they have? Is
   every 3D word defined or linked? Can they run what the page shows, with the
   commands given? Does any sentence only fill space? Fix what fails; for a
   large rewrite, have a subagent do this pass with no other context and
   report the places where it got lost.
