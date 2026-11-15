# 🎨 Techniques

[⏫ Documentation](../../README.md#-documentation)

A technique is a way to get one visual effect: a shadow, a halo round a lamp, water that flows. Each one costs GPU time or memory, and none of them is needed in every program. This page lists the techniques Compages offers, says when each one helps and when it does not, and links to the page that explains it in detail.

The detailed pages follow the shape of [3D Game Shaders For Beginners](https://github.com/lettier/3d-game-shaders-for-beginners) by David Lettier: one effect per page, a picture first, then the idea, the code, the cost and the limits. Their text, code and pictures are written for Compages.

## 💡 Light

### ☀ One sun

A directional light: only its direction counts. It gives shape to everything with one cost, and it is enough for an outdoor scene, a robot cell, most scientific views. Use `scene.sun(…)`. Demo `06d_Lights`.

### 🌑 Shadows of the sun

On by default. The Renderer draws the depth of the scene seen from the sun into a picture of 2048 × 2048 texels, once per frame, fitted to the objects that cast a shadow. Keep the floor from casting (`RenderFlags::NoShadowCaster`): a large floor stretches the picture and blurs every shadow. Turn shadows off (`Environment::shadows = false`) for a chart or a flat field, where they only add noise.

### 💡 A few lamps

A lamp lights round its position, up to a range. The four lamps nearest the camera are computed for every pixel they reach. Use them for a desk lamp, a torch, the light of a screen: `scene.lamp(…)`.

### 💡 Shadows of a lamp

Off by default. A lamp shines in all directions, so its shadow needs six more pictures of the nearby scene, one per face of a cube: six more draws of everything within its range. Turn it on for the one or two lamps that matter (`PointLight::shadows = true`), never for all of them. Demos `10a_MvpDemo`, `10b_DoomLike`.

### 🏮 Hundreds of lamps

Beyond the four nearest, lamps go into **clusters**: the view is cut into small boxes, and a pixel only adds the lamps of its box. A pixel lit by three lamps costs three lamps, whether the scene holds ten or two hundred. Use it for a city at night, the tubes of a corridor, sparks. These lamps cast no shadow, and up to 256 can be used. If your scene has one sun and two lamps, clusters bring nothing. Compare `06d_Lights` (a sun and a few lamps) with `06m_ManyLights` (two hundred lamps over a floor).

### 💡 Fog that light crosses (volumetric fog)

Shafts of sun between pillars, halos round lamps, shadows in the air. Two compute shaders fill a 3D grid of 128 × 72 × 64 cells that follows the camera, at every frame. It is the most expensive light effect of Compages; use it when the air itself is part of the picture (a dusty hall, a foggy street). For distance haze alone, the plain fog of `Environment::fog_density` costs nothing. Demo `06n_VolumetricFog`.

### 🎞 Exposure and tone curve

Light has no upper limit; a screen does. The tone curve maps one to the other at the end of every picture (`Environment::tone_mapping`, ACES by default) and costs nothing. Change `exposure` before you add lights to a dark scene.

## 🎨 Surfaces

### 🖼 A colour or a picture

`color()` and `texture()` cover most shapes, at the lowest cost.

### 📂 A model from a file

GlTF materials carry metallic, roughness and normal maps; the Renderer uses the metallic-roughness model of glTF for them. Use files for real objects; do not rebuild them by hand.

**A surface of your own (hook `surface`).** When no picture fits: worn paint, hot metal, a pattern computed from the position. A few lines of GLSL, no extra pass. [A look of your own](../tutorials/06-a-look-of-your-own.md), [write a material](../guides/write-a-material.md).

**Another model of light (hook `light`).** When the light itself must look different. [Cel shading and outlines](cel-shading.md): flat tones like a cartoon, and a black outline drawn as a second, slightly larger shape. The outline doubles the number of draws of the shapes that have one.

### 🫥 See-through

Glass and water are drawn after the solid shapes, sorted from far to near. Two see-through shapes that cross each other can be drawn in the wrong order. [See-through things](../guides/see-through-things.md).

## 🖼 The whole picture

### ✨ Filters of the screen

A filter is one function of GLSL run on every pixel of a view: sepia, outlines from the depth, a vignette. Each filter is one more pass over the whole view, and a view with filters is drawn into a floating-point picture first. Use them per camera: the camera of a robot can keep a clean picture. [Screen effects](../guides/screen-effects.md), demo `12a_ScreenFilters`.

### 💡 Bloom

Light brighter than white spills round its source. It needs bright sources (lamps seen directly, the sun on metal) to be visible; in a dim scene it only blurs. About twelve small passes. [Bloom](bloom.md), demo `12b_Bloom`.

## 🖼 Simulation in the picture

### 🏓 Ping-pong

A state that changes over time (cells, a fluid, wet skin) is kept in two textures or buffers: one is read, the other written, then they swap. Use it when the next state depends on the last one. When the motion is a formula of the time (an orbit, a wave), the vertex shader alone is enough and nothing is stored. [Compute on the GPU](../guides/compute-on-the-gpu.md).

### 🌊 Flow mapping

Water that flows round obstacles without a fluid simulation: a picture is moved along a map of directions. Cheap at run time; the map is made once. [Flow mapping](flow-mapping.md), demo `10f_FlowingWater`.

**Waves that move the surface (hook `deform`).** The corners of a flat grid are lifted by a formula of the time on the GPU. The CPU can compute the same formula to float boats on it. Demo `10e_Archipelago`.

## 📄 Not in Compages yet

Ambient occlusion in screen space (SSAO), depth of field, reflections in screen space (SSR). A rim light or a Fresnel edge can be written today with the hook `light`.

## 📄 Pages

| Technique | Demo | Page |
|-----------|------|------|
| Cel shading and outlines | `06o_CelShading` | [cel-shading.md](cel-shading.md) |
| Bloom | `12b_Bloom` | [bloom.md](bloom.md) |
| Flow mapping | `10f_FlowingWater` | [flow-mapping.md](flow-mapping.md) |
| Clustered lighting | `06m_ManyLights` | to write |
| Volumetric fog | `06n_VolumetricFog` | to write |
| Shadows of the sun and of lamps | `06d_Lights`, `10b_DoomLike` | to write |
| Filters of the screen | `12a_ScreenFilters` | to write |
| Ping-pong in a texture | `05b_GameOfLife`, `05c_GrayScott` | to write |
| Normal mapping | `08a_GltfModel` | to write |
| Sky box | `06h_Skybox` | to write |

The shaders, their pieces and their hooks are listed in the [shader library](../reference/shaders.md).
