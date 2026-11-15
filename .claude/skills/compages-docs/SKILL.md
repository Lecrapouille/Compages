---
name: compages-docs
description: Write, restructure or review the Compages documentation (README.md, doc/ in Diátaxis folders tutorials/ guides/ concepts/ reference/, plus techniques/ and case-studies/, examples/README.md, screenshots in Compages-data). Use when documenting a call, a feature, a demo, the build, or when asked to fix, rewrite or review docs.
---

# Compages documentation

Readers: C++ developers with no 3D or OpenGL background. Their English is
basic (many are French): write simple English, short sentences (about 20
words at most, one idea each), common words, no idioms, no phrasal verbs
when a plain verb exists.

The goal: pages that read as if an experienced developer explained what they
understood, not as if a tool described a repository. Few pages that are good
beat many pages that are thin: do not add a page, a section or a table that
does not teach something.

## Map: one page, one job (Diátaxis)

| Folder | Kind | The reader wants to… | A page is |
|--------|------|----------------------|-----------|
| `doc/tutorials/` | tutorial | learn, by doing, in order | the three layers from the bottom up: the GPU (`01c`, `05a`), the World (the arm of `examples/13_Workshop/Workshop/Arm.cpp`, headless), then the Renderer (`13a`…`13d`) |
| `doc/guides/` | how-to | do one task now | goal → steps → code → result picture → see also |
| `doc/concepts/` | explanation | understand why | the problem, the idea, the choice made and what it costs |
| `doc/reference/` | reference | look up a fact | tables of calls, the gallery list, credits; no story |
| `doc/techniques/` | technique | reproduce one effect | the fixed template below, one demo per page |
| `doc/case-studies/` | case study | see a whole demo built | 10c, 10e, 10f step by step; one fiche per `11_Science` demo |

There is no `doc/README.md`: the "Documentation" section of the root
`README.md` is the only map, a few lines long. Each folder has a `README.md`
that lists its pages in reading order, as a plain list with one line each.

Do not mix the kinds. A tutorial does not explain theory at length: it links
to a concept. A guide does not teach from zero: it links to the tutorial. A
reference page has no narrative. When a page starts doing two jobs, split it.

Put a fact in one place and link to it. Code comments point to pages
(`doc/concepts/three-layers.md`): update them when a page moves
(`grep -rn "doc/" include src examples tests .claude AGENTS.md README.md`).

## Writing charter

- **Start from the problem**, never from a list of features. First sentence:
  a situation the reader knows ("You want a river that flows round rocks").
- **From the general to the specific**: the tutorials follow the layers,
  GPU → World → Renderer. The running example (the robot workshop) starts
  in the World, headless, and the Renderer then draws it, one idea per page.
  It never comes before the layer it uses.
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
- **No signs of machine writing** (after Wikipedia, *Signs of AI writing*, and
  the *humanizer* skill):
  - Words: delve, tapestry, testament, pivotal, crucial, vital, key (as an
    adjective), underscore, highlight, showcase, leverage, robust (except its
    technical sense), seamless, powerful, comprehensive, intricate, landscape,
    vibrant, valuable, enhance, foster, align with, simply, just, easy,
    "serves as", "stands as", "boasts", "offers".
  - Patterns: "not X but Y", "not just X, it is Y"; groups of three for
    rhythm; run-ups ("Here is the thing", "Let's dive in"); closers that
    repeat the point ("That is the whole trick"); a first sentence that
    repeats the heading; sentences about the page itself ("This section
    explores…", "the table below shows"); vague links ("linked to",
    "associated with"); "-ing" tails ("…, ensuring that…"); stacked hedges;
    absolute claims ("the only library that…").
  - Layout: no signpost headings ("Quick answers", "Key takeaways", "The four
    kinds of page", "What happened", "Choose a path"); no table of "When you
    want to…" rows; bold only for the one word a reader must not miss; no
    "**Label:** text" lists; no horizontal rule between every section; no
    emoji in body text (emojis in headings are the house style: see Format).
  - Fix by rewriting the paragraph around its point, not by swapping words.
    Vary sentence length. Prose first; a list only for a real enumeration, a
    table only for a real comparison.
- **Keep the author's voice**: dry, concrete, physical comparisons (a shell
  drawn inside out, a picture moved along the water). Do not flatten the
  existing pages into a generic tone.
- **Depth by place**: README welcomes, tutorials hold the hand, concepts and
  architecture are rigorous.
- **Doxygen** follows the same rules, and the six questions of AGENTS.md
  ("Doxygen comments"): why, when, how, guarantees, limits, why this choice;
  an example for each public class.

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
  in `setUp()` (as `13b_HotPlate`, `11a_HeatPlate`).
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
`examples/13_Workshop/Workshop/*.cpp`), explained in order · "Run it"
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

- One emoji at the start of every heading (`# 🎓 Tutorials`, `## 🧪 Try`),
  as in the existing pages: it is the style of Compages, keep it. Pick one
  that says what the section is about; reuse the usual ones (🔗 See also,
  🧪 Try, ▶ Run it, 🚧 Limits, 📏 Cost, 📁 Source). 🟧 GPU, 🟦 World,
  🟩 Renderer mark the layers. Navigation lines use `[◀ Previous](…) ·
  [⏫ Folder](README.md) · [Next: … ▶](…)`.
- GitHub drops the emoji from a heading id and keeps the space as a hyphen:
  `## 🧪 Try` → `#-try`. Never use an emoji with U+FE0F (⚠️ 🖥️ 🛠️ ▶️ …) in a
  heading: the selector stays in the id and links break. Use ▶, not ▶️.
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
