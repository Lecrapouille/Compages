# 🎨 6. A look of your own

[◀ Values on a grid](05-values-on-a-grid.md) · [⏫ Tutorials](README.md) · [Next: what the robot sees ▶](07-what-the-robot-sees.md)

![13c_PaintedArm](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13c_PaintedArm.png)

The paint of a machine wears away first on the edges, where tools and hands rub it, and the steel shows under it. No built-in look does that. You will write it in a few lines of GLSL, and keep everything else the Renderer gives: the light of the sun and of the lamp, the shadows, the colour of each link.

## 🖌 Change the surface, keep the light

The Scene draws its shapes with a lit shader of its own, the **PBR** shader (physically based rendering). Writing a whole lit shader is long: the sun, the lamps, the shadows, the fog, the colours of the screen. So the PBR shader leaves named functions that you may replace: its **hooks**. The hook `surface` receives the surface as the shape describes it, with its colour and its normal, and may change it before the light is computed. From [Workshop/Paint.cpp](../../examples/13_Workshop/Workshop/Paint.cpp):

```glsl
void surface(inout Surface s)
{
    float edge = min(min(vUv.x, 1.0 - vUv.x), min(vUv.y, 1.0 - vUv.y));
    float paint = smoothstep(0.02, 0.08, edge);
    s.albedo = mix(vec3(0.55, 0.56, 0.58), s.albedo, paint);
}
```

`vUv` gives where the pixel lies on its face of the box, from 0 to 1 in each direction (its **UV coordinates**). The distance to the nearest edge of the face is the smallest of four numbers: `vUv.x`, `1 - vUv.x`, `vUv.y` and `1 - vUv.y`. `smoothstep(0.02, 0.08, edge)` is 0 on the edge, 1 a little inside, with a soft step between. `mix()` then keeps the colour of the link (`s.albedo`) inside the face and puts a light steel grey on the edge.

The shader never mentions the sun or the lamp. They come after the hook, unchanged.

## 🎨 A material for the links

```cpp
compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
shaders.fragment().hook("surface", WORN_PAINT);
compages::renderer::MaterialId paint;
COMPAGES_TRY_ASSIGN(paint,
                    p_scene.assets().createMaterial(
                        "Worn paint", { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                        .shaders = std::move(shaders),
                                        .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                        .see_through = std::nullopt }));
for (compages::world::Entity const& link : p_links)
{
    p_scene.look(link, paint);
}
```

`shaders::pbr()` gives the two stages of the PBR shader as text. `hook("surface", WORN_PAINT)` puts your function in place of the default one, which changes nothing. `createMaterial()` compiles the shader once and keeps it in the catalog of the Scene, under a name. Its four fields have no default value, so nothing is chosen for you without your knowledge:

| Field | Here | Meaning |
|-------|------|---------|
| `family` | `PbrMinimal` | which uniforms the Renderer gives the shader |
| `shaders` | the PBR shader with your hook | the program |
| `state` | depth test, back faces hidden | a solid, closed shape |
| `see_through` | none | the shape is drawn with the solid ones |

`look(entity, material)` gives the material to a link. Each link keeps its own colour, which arrives in `s.albedo`. The same material would show a blue link worn grey on its edges.

If the GLSL has an error, `createMaterial()` returns the message of the compiler, with the line. `paintArm()` returns it, then `setUp()` of the demo, and the console of the gallery shows it before the first frame.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 13c_PaintedArm
```

The edges of the two links show light grey steel. The links still shade from light to dark as they turn under the lamp.

## 🧪 Try

- Change `0.08` to `0.25`: the paint is worn on a wider band.
- Replace `vec3(0.55, 0.56, 0.58)` by `vec3(0.05)`: black edges, like a rubber seal.
- Remove the closing parenthesis of `mix(`. The demo stops at its setup, and the console shows the line of the error.
- Use `fract(vWorldPos.y * 10.0)` instead of `edge`. You get bands every 10 cm, fixed in the room: the arm moves through them. Real paint stays on the object, which is why the hook uses `vUv`.

A `Surface` also has `metallic` and `roughness`. They count only when the material instance of the shape uses the metallic-roughness model of glTF (`MaterialInstance::metal_roughness`). The glTF loader turns it on for a model loaded from a file. A shape made with `color()` has it off: it is lit by a simpler model, which ignores these two values. Turn it on by hand to use them. The PBR shader has three more hooks: `deform` moves the corners of a shape, `light` changes how light falls on it, and `main` replaces everything. [Write a material](../guides/write-a-material.md) shows each one, and the [shader library](../reference/shaders.md) shows the pieces that a hook may call.

---

[◀ Values on a grid](05-values-on-a-grid.md) · [⏫ Tutorials](README.md) · [Next: what the robot sees ▶](07-what-the-robot-sees.md)
