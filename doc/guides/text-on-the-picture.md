# 🔤 Text on the picture

[⏫ Guides](README.md)

**Goal:** write the name of a thing next to it, a box of lines that points at it, or a caption at a fixed place of a view.

![10d_EarthMoon](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10d_EarthMoon.png)

Three components write on the picture (`Renderer/Components/Text.hpp`). Load a font once (TrueType or OpenType; a wrong file is reported by `prepare()`), then set the components:

| Component | On | Writes |
|-----------|----|--------|
| `Label` | any entity | a dot on its point, the text on its right |
| `Tooltip` | any entity | a box of lines that points at it, like the tooltip of a web page |
| `Caption` | a camera | text at a fixed place of what this camera sees |

```cpp
scene.font(dataPath("fonts/DejaVuSans.ttf"));
camera.set(compages::renderer::Caption{ .text = "Altitude 0 km",  // '\n' for more lines
                                        .color = { 1.0f, 0.96f, 0.88f },
                                        .pixels = 16.0f,
                                        .at = { 12.0f, 10.0f } });   // pixels from the top left
mars.set(compages::renderer::Label{ .text = "Mars",              // UTF-8, Latin-1 letters
                                    .color = { 1.0f, 0.95f, 0.85f },  // sRGB
                                    .pixels = 15.0f,                  // height of the letters
                                    .hide_closer_than = 0.0f });      // metres; 0 = always
region.set(compages::renderer::Tooltip{ .title = "Astronomie",
                                        .text = "fr.wikipedia.org/wiki/Astronomie\n"
                                                "fr.wikipedia.org/wiki/Galaxie",
                                        .color = { 0.90f, 0.94f, 0.98f },       // the lines
                                        .accent = { 1.0f, 0.82f, 0.48f },       // title, stem, dot
                                        .background = { 0.05f, 0.10f, 0.17f },  // the frame
                                        .opacity = 0.86f,
                                        .pixels = 14.0f });
```

The letters keep their size in pixels at any distance. Change the `text` of a caption each frame for numbers that move; set a tooltip to show it and remove it to hide it (`10e_Archipelago` shows the links of the region under the mouse). Each view of a split screen has its own caption.

**Rules.** A label that would cover text already written (a caption, a tooltip, another label) is not written, so names stay readable where things crowd. A label is not hidden by what is in front of its point. `hide_closer_than` removes the name when the thing is near enough to be seen: `10d_EarthMoon` gives each planet 50 times its radius. A tooltip faces the camera, sits above its point or below when there is no room, and slides sideways to stay inside the picture.

**How.** The letters are drawn the way of Glumpy: the font is baked once into a picture of **distance fields** (stb_truetype), where each pixel says how far it is from the edge of the letter. The shader cuts at the edge with a smooth step of one pixel, so a letter is sharp at any size; a second cut a little outside gives a dark halo, readable on a bright planet. All the text is drawn in one call, after everything else, and after the [screen effects](screen-effects.md).

In the gallery, `dataPath()` comes from `examples/Common/DataPath.hpp`; in your program, give a path that exists.

## 🔗 See also

[Draw helpers](draw-helpers.md) · [Very large scenes](very-large-scenes.md)
