# 🎨 6. A look of your own

[◀ Values on a grid](05-values-on-a-grid.md) · [⏫ Tutorials](README.md) · [Next: what the robot sees ▶](07-what-the-robot-sees.md)

![13c_PaintedArm](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13c_PaintedArm.png)

The paint of a machine wears away first on the edges, where tools and hands rub it, and the steel shows under it. No built-in look does that.

## 🎯 Goal

Write a GLSL surface hook that gives the arm grey edges and orange face centres. You will share the material between its links while keeping their lighting and shadows.

## ▶ Compare the two surfaces

```sh
./build/linux-gcc-debug/bin/Compages-examples 13b_HotPlate
./build/linux-gcc-debug/bin/Compages-examples 13c_PaintedArm
```

Observe the links' grey edges in the second demo. Their motion and the plate's heat use the previous code. Open [Workshop/Paint.cpp](../../examples/13_Workshop/Workshop/Paint.cpp). All shader edits in this chapter go in its `WORN_PAINT` string. Quit the gallery, rebuild with `make`, and run `13c_PaintedArm` after each edit.

## 🖌 Step 1: replace one surface colour

The Scene draws its shapes with a lit shader of its own, the **PBR** shader (physically based rendering). Writing a whole lit shader is long: the sun, the lamps, the shadows, the fog, the colours of the screen. So the PBR shader leaves named functions that you may replace: its **hooks**. The hook `surface` receives the surface as the shape describes it, with its colour and its normal, and may change it before the light is computed. From [Workshop/Paint.cpp](../../examples/13_Workshop/Workshop/Paint.cpp):

First replace the body of the existing `surface` function with one assignment:

```glsl
void surface(inout Surface s)
{
    s.albedo = vec3(0.55, 0.56, 0.58);
}
```

Both links become grey across their whole surface. They still shade under the lights. `albedo` is the surface's base colour before lighting. `inout` lets the hook read the incoming `Surface` and change it for the lighting stage. Other surface values retain their incoming values.

The path is now: the shape's colour → your surface hook → lighting → the displayed pixel. Unlike the fragment shader of chapter 2, your function does not write the final pixel colour.

## 📐 Step 2: choose where paint survives

Restore the original function from `Paint.cpp`:

```glsl
void surface(inout Surface s)
{
    float edge = min(min(v2f.uv.x, 1.0 - v2f.uv.x), min(v2f.uv.y, 1.0 - v2f.uv.y));
    float paint = smoothstep(0.02, 0.08, edge);
    s.albedo = mix(vec3(0.55, 0.56, 0.58), s.albedo, paint);
}
```

`v2f.uv` gives where the pixel lies on its face of the box, from 0 to 1 in each direction (its **UV coordinates**). The distance to the nearest edge of the face is the smallest of four numbers: `v2f.uv.x`, `1 - v2f.uv.x`, `v2f.uv.y` and `1 - v2f.uv.y`. `smoothstep(0.02, 0.08, edge)` is 0 on the edge, 1 a little inside, with a soft step between. `mix()` then keeps the colour of the link (`s.albedo`) inside the face and puts a light steel grey on the edge.

The shader never mentions the sun or the lamp. They come after the hook, unchanged.

At the centre of a face, `v2f.uv = (0.5, 0.5)`, so `edge = 0.5` and `paint = 1`. `mix(a, b, 1)` returns `b`: the link keeps its original orange. On the left edge, `v2f.uv.x = 0`, so `edge = 0` and `paint = 0`. `mix(a, b, 0)` returns `a`: the steel colour appears. The range 0.02 to 0.08 controls the transition in UV space, not in metres. A scaled box stretches that band with its face.

## 🎨 Step 3: share the shader between links

The rest of `paintArm()` creates a material and assigns it to the two link entities:

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

In [13c_PaintedArm.cpp](../../examples/13_Workshop/13c_PaintedArm.cpp), setup replaces the previous call to `dressArm()` with:

```cpp
COMPAGES_TRY(workshop::paintArm(m_scene, workshop::dressArm(m_scene, m_arm)));
```

`dressArm()` still creates the shapes. It also returns the two links to paint. `paintArm()` assigns the shared material before `m_scene.prepare()`. Do not call `dressArm()` a second time: that would create duplicate shapes. The plate and the other room objects keep their existing materials.

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
- Use `fract(v2f.worldPos.y * 10.0)` instead of `edge`. You get bands every 10 cm, fixed in the room: the arm moves through them. Real paint stays on the object, which is why the hook uses `v2f.uv`.

A `Surface` also has `metallic` and `roughness`. They count only when the material instance of the shape uses the metallic-roughness model of glTF (`MaterialInstance::metal_roughness`). The glTF loader turns it on for a model loaded from a file. A shape made with `color()` has it off: it is lit by a simpler model, which ignores these two values. Turn it on by hand to use them. The PBR shader has three more hooks: `deform` moves the corners of a shape, `light` changes how light falls on it, and `main` replaces everything. [Write a material](../guides/write-a-material.md) shows each one, and the [shader library](../reference/shaders.md) shows the pieces that a hook may call.

---

[◀ Values on a grid](05-values-on-a-grid.md) · [⏫ Tutorials](README.md) · [Next: what the robot sees ▶](07-what-the-robot-sees.md)
