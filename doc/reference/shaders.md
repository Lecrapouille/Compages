# 🔍 Shader library

[⏫ Reference](README.md)

The Renderer draws with GLSL that Compages writes for you, in [src/Renderer/Procedural/ShaderLib.cpp](../../src/Renderer/Procedural/ShaderLib.cpp). This page shows how this GLSL is cut into pieces, which pieces include which, where you can put your own code, and which uniforms the Renderer fills. The calls are in [ShaderLib.hpp](../../include/Compages/Renderer/Procedural/ShaderLib.hpp); how to use the hooks is in [write a material](../guides/write-a-material.md).

## 🔍 The shaders

`compages::renderer::shaders` returns each shader as a `ShaderPair`: its vertex stage and its fragment stage, as `gpu::ShaderSource` text that you may change before it is compiled.

| Call | Family | Draws |
|------|--------|-------|
| `pbr()` | `PbrMinimal` | lit surfaces: the colour or picture of an instance, the metallic-roughness model of glTF, four hooks |
| `lit()` | `Lit` | lit surfaces with one flat colour |
| `unlit()` | `Unlit` | a colour no light changes: bulbs, markers, labels |
| `depth()` | `Depth` | the distance to the camera, as grey |
| `normals()` | `Normals` | the direction of each surface, as a colour |
| `shadow()` | — | the depth seen from the sun (the shadow picture) |
| `lampShadow()` | — | the distance to a lamp (one face of its shadow cube) |
| `compute(name, text)` | — | a compute shader that may include the pieces below |

The **family** of a material says which uniforms the Renderer sets for it. A material made with `createMaterial()` takes the family of the shader it starts from.

## 🧩 The pieces

A piece is a named block of GLSL that a shader includes with `#include <compages/….glsl>`. Your own shaders, and the compute shaders made with `compute()`, can include the same pieces. In a compute shader, `COMPAGES_COMPUTE` is defined, and the parts that need a pixel are left out.

```text
pbr() vertex stage
├── <compages/skinning.glsl>     joints, weights, skinMatrix()
└── <hook/deform>                deform(world, normal): yours, or no change

pbr() fragment stage
├── <compages/fog.glsl>          applyFog(colour, worldPos): plain fog, or the volumetric fog
│   └── <compages/scene.glsl>    lightDir, lightColor, ambient, cameraPos, fogColor, fogDensity
├── <compages/shade.glsl>        shade(base, n, worldPos), shadePbr(albedo, n, worldPos, metallic, roughness),
│   │                            bumpNormal(n, worldPos, uv, tangentNormal)
│   ├── <compages/scene.glsl>
│   ├── <compages/sun.glsl>      sunVisibility(n, toLight, worldPos): the shadow of the sun
│   └── <compages/lamps.glsl>    pointLightsContribution(n, worldPos): the four nearest lamps
│                                and the clustered ones (storage buffers 9, 10, 11)
├── <compages/present.glsl>      present(linear): tone curve and sRGB; screen(colour); opacity
│   └── <compages/srgb.glsl>     toSrgb(), fromSrgb()
├── struct Surface, instanceSurface(), sceneLight(s), toScreen(s, lit)
├── <hook/surface>               surface(s): yours, or no change
├── <hook/light>                 light(s): yours, or sceneLight(s)
└── <hook/main>                  main(): yours, or the four steps below
```

`lit()` includes `fog`, `shade` and `present` the same way, with no hooks. Each piece is included once, even when several pieces ask for it.

## 🔍 The four hooks of the PBR shader

The default `main()` of the fragment stage is these steps:

```glsl
void main()
{
    Surface s = instanceSurface();   // the colour, picture, normal map of the instance
    surface(s);                      // hook: change the surface
    oColor = toScreen(s, light(s));  // hook light, then emission, fog, tone curve, sRGB
}
```

| Hook | Stage | Signature | Default | Write it to… |
|------|-------|-----------|---------|--------------|
| `deform` | vertex | `void deform(inout vec4 world, inout vec3 normal)` | no change | move corners on the GPU: waves, a bent beam |
| `surface` | fragment | `void surface(inout Surface s)` | no change | a pattern, worn paint, hot metal |
| `light` | fragment | `vec3 light(Surface s)` | `sceneLight(s)` | another model of light: cartoon tones, a rim |
| `main` | fragment | `void main()` | the three lines above | what is not a lit surface: a picture shown as it is, mixed lightings |

A `Surface` holds `albedo`, `normal`, `metallic`, `roughness`, `emission` and `opacity`. `metallic` and `roughness` count only when the instance uses the metallic-roughness model of glTF (`MaterialInstance::metal_roughness`, turned on by the glTF loader, or by hand). A shape made with `color()` has it off and is lit by `shade()`, which uses the albedo and the normal only.

A hook may read what the PBR shader declares: `vWorldPos`, `vNormal`, `vUv`, `baseColorFactor`, `baseColorMap`, `opacity`, `cameraPos`, and everything the pieces above give. A hook name that the shader does not declare is an error that lists the known names.

You add your own pieces with `include(name, text)` on a stage, before the hook that uses them:

```cpp
compages::renderer::shaders::ShaderPair water = compages::renderer::shaders::pbr();
water.vertex()
    .include("archipelago/swells.glsl", swells)   // GLSL written from C++ data
    .hook("deform", SURFACE_WAVES);
```

## 🟩 What the Renderer sets

For every draw, if the shader declares them:

| Uniform | Holds |
|---------|-------|
| `model`, `view`, `projection` | the placement of the shape, the camera, the lens (depth reversed) |
| `cameraPos` | the camera, in world space |
| `lightDir`, `lightColor`, `ambient` | the sun and the sky |
| `shadowMap`, `lightViewProj`, `shadowEnabled`, `shadowBias`, `shadowTexel` | the shadow of the sun |
| `pointLightCount`, `pointLightPos0…3`, `pointLightColor0…3`, `pointLightRange0…3` | the four nearest lamps |
| `lampShadowMap`, `lampFaces`, `lampShadows` | the shadows of these lamps |
| `clusterView`, `clusterTiles`, `clusterSlices`, `clusterDepth`, `clusterViewport` | the clustered lamps |
| `fogColor`, `fogDensity`, `fogVolume…` | the plain and the volumetric fog |
| `exposure`, `toneMapping`, `linearTarget` | the tone curve, and whether the view has screen effects |
| `baseColorFactor`, `baseColorMap`, `hasBaseColorMap`, `opacity` | the instance |
| `pbrUseMetalRoughness`, `pbrMetallic`, `pbrRoughness`, `pbrMetalRoughnessMap`, `pbrNormalMap`, `pbrNormalScale` | the metallic-roughness model of an instance |

The textures and storage buffers listed in `MaterialDesc::textures` and `::buffers` are bound by name. The units 2 to 6 are free for your textures; the bindings 9, 10 and 11 hold the clustered lamps.

A uniform that the shader declares but never uses is removed by the GLSL compiler. Setting it then is a frame error: use what you declare.

## ✨ Screen filters

A [screen filter](../guides/screen-effects.md) is not a hook of the PBR shader. It is one function, `vec3 effect(vec2 uv)`, that Compages wraps in a full-screen pass. It reads `screenColor`, `screenDepth`, `screenSize` and its own `uniform float` values.
