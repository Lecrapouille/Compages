# 🎨 Techniques, one page each

[⏫ Documentation](../README.md)

Each page explains **one technique** of real-time 3D: what problem it solves, the idea behind it, the few lines of Compages that switch it on, the GLSL that does the work, its cost and its limits. One demo of the gallery shows it; open the demo next to the page:

```sh
./build/linux-gcc-debug/bin/Compages-examples 06o_CelShading
```

The pages follow the order of difficulty: each one may use a word or a call from a page above it, never from a page below. You can also jump straight to the effect you need.

The list is inspired by [3D Game Shaders For Beginners](https://github.com/lettier/3d-game-shaders-for-beginners) by David Lettier, a guide we recommend reading. Its text and pictures are not ours: these pages, their code and their screenshots are written for Compages.

## 🗺 The list

✅ written · 🕓 the demo exists, the page is to write · 🚧 not in Compages yet

| # | Technique | Demo | Page |
|---|-----------|------|------|
| 1 | Reference frames: model, view, projection | `06b_Transforms`, `06c_Hierarchy` | 🕓 |
| 2 | A shader named by its data | `01c_Triangle`, `01f_VertexSyntax` | 🕓 |
| 3 | Textures | `03a_TexturedQuad`, `06g_TextureGallery` | 🕓 |
| 4 | Render to a texture | `04d_RenderToTexture` | 🕓 |
| 5 | Light: sun, lamps, shadows | `06d_Lights` | 🕓 |
| 6 | Normal mapping | `08a_GltfModel` | 🕓 |
| 7 | A sky round the scene | `06h_Skybox` | 🕓 |
| 8 | [Cel shading and outlines](cel-shading.md) | `06o_CelShading` | ✅ |
| 9 | Hundreds of lamps: clustered lighting | `06m_ManyLights` | 🕓 |
| 10 | Fog in a volume: shafts of light | `06n_VolumetricFog` | 🕓 |
| 11 | Filters of the screen | `04e_PostProcess`, `12a_ScreenFilters` | 🕓 |
| 12 | [Bloom](bloom.md) | `12b_Bloom` | ✅ |
| 13 | A simulation in a texture: ping-pong | `05b_GameOfLife`, `05c_GrayScott` | 🕓 |
| 14 | [Flow mapping](flow-mapping.md) | `10f_FlowingWater` | ✅ |
| 15 | Fresnel and rim lighting | — | 🚧 |
| 16 | Ambient occlusion in screen space (SSAO) | — | 🚧 |
| 17 | Depth of field | — | 🚧 |
| 18 | Reflections in screen space (SSR) | — | 🚧 |

## 📄 How a page reads

Every page has the same parts, in the same order, so that you know where to look:

| Part | It answers |
|------|-----------|
| Picture and card | What does it look like? Which demo, which layer, how many lines? |
| 🎯 The problem | What is wrong without it? |
| 💡 The idea | How does it work, in words and one picture? |
| 🧩 In Compages | Which calls switch it on? (the real lines of the demo) |
| 🔍 The shader | Which GLSL does the work? |
| 📏 Cost | How many passes, textures, how much memory? |
| 🚧 Limits | What it does not do, and where to go next |
| 🧪 Try it | What to change in the demo, and what you should see |
| 📁 Source | The files to open |

The card at the top counts **your** lines only: the C++ that switches the effect on and sets it, and the GLSL you write. Building the scene round it does not count.
