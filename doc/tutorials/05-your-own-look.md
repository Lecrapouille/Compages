# 🎨 5. A look of your own

[◀ Values on a grid](04-values-on-a-grid.md) · [⏫ Tutorials](README.md) · [Next: What the robot sees ▶](06-what-the-robot-sees.md)

![00d_PaintedArm](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/00d_PaintedArm.png)

Machines that move are painted with black and yellow warning stripes. No built-in look does that. You will write the stripes yourself, in a few lines of GLSL, and keep everything else the Renderer gives: the light of the sun and of the lamp, the shadows, the colour of each link.

## 🖌 Change the surface, keep the light

A **shader** is a small program the GPU runs for each pixel of a shape. Writing a whole lit shader is long: the sun, the lamps, the shadows, the fog, the colours of the screen. Compages lets you replace only one part of its own lit shader, called **PBR** (physically based rendering): its parts with a name are **hooks**. The hook `surface` receives the surface as the shape describes it (its colour, its normal, how metallic and how rough it is) and may change it, before the light is computed. From [Workshop/Paint.cpp](../../examples/00_Workshop/Workshop/Paint.cpp):

```glsl
void surface(inout Surface s)
{
    float stripe = step(0.5, fract((vUv.x + vUv.y) * 4.0));
    s.albedo = mix(s.albedo, vec3(0.03), stripe);
    s.metallic = 0.2;
    s.roughness = 0.35;
}
```

`vUv` is where the pixel lies on the face of the box, from 0 to 1 in each direction (its **UV coordinates**). `vUv.x + vUv.y` grows along the diagonal; `fract(… * 4.0)` makes it go from 0 to 1 four times; `step(0.5, …)` turns that into 0 for the first half and 1 for the second: diagonal stripes. `mix(a, b, stripe)` keeps the colour of the link (`s.albedo`, orange) where `stripe` is 0, and paints near-black where it is 1. The last two lines make the paint a little shiny.

The shader never speaks of the sun or the lamp: they come after the hook, unchanged.

## 🧩 A material, given to the links

```cpp
compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
shaders.fragment().hook("surface", STRIPES);
compages::renderer::MaterialId paint;
COMPAGES_TRY_ASSIGN(paint,
                    p_scene.assets().createMaterial(
                        "Warning paint", { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                           .shaders = std::move(shaders),
                                           .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                           .see_through = std::nullopt }));
for (std::size_t k = 1u; k < p_arm.links.size(); ++k)
{
    p_scene.look(p_arm.links[k], paint);
}
```

`shaders::pbr()` gives the two stages of the PBR shader as text; `hook("surface", STRIPES)` puts your function in place of the default one, which changes nothing. `createMaterial()` compiles it once and keeps it in the catalog of the Scene under a name. These four fields have no default, so nothing is chosen for you without your knowing (the pictures and buffers a shader reads, `.textures` and `.buffers`, are empty by default):

| Field | Here | Means |
|-------|------|-------|
| `family` | `PbrMinimal` | which uniforms the Renderer gives the shader |
| `shaders` | the PBR with your hook | the program |
| `state` | depth test, back faces culled | a solid, closed shape |
| `see_through` | none | drawn with the solid shapes, not after them |

`look(entity, material)` gives the material to a link. Each link keeps its own colour, which arrives in `s.albedo`: the same paint would make blue stripes on a blue link.

Compiling can fail (a typo in the GLSL): `COMPAGES_TRY_ASSIGN` then returns the error of the compiler, with the line, from `paintArm()`, and `setUp()` of the demo returns it. You see it in the console of the gallery before the first frame.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 00d_PaintedArm
```

The two links are striped; they still shade from light to dark as they turn under the lamp.

## 🧪 Try

- Write `* 10.0` instead of `* 4.0`: thinner stripes.
- Replace `vec3(0.03)` by `vec3(1.0, 0.85, 0.0)`: yellow on orange.
- Write `s.albedo = vec3(1.0, 0.0, 0.0` (no closing parenthesis): the demo stops at its setup, and the console shows the line of the error.
- Use `vWorldPos.y` instead of `vUv.x + vUv.y`: stripes fixed in the room, through which the arm moves. Not what paint does.

## 💡 What happened

A hook replaces one named function of a shader that Compages owns. The PBR shader has four: `deform` moves the vertices, `surface` changes what a surface is, `light` changes how light falls on it, `main` replaces everything. The [guide to writing a material](../guides/write-a-material.md) shows each, and the techniques use them for a [cartoon look](../techniques/cel-shading.md) or [flowing water](../techniques/flow-mapping.md). Why shaders are glued by names, and not by numbers: [the GPU layer](../concepts/the-gpu-layer.md).

---

[◀ Values on a grid](04-values-on-a-grid.md) · [⏫ Tutorials](README.md) · [Next: What the robot sees ▶](06-what-the-robot-sees.md)
