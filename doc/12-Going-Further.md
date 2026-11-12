# 🎓 12. Going further: the calls that matter

[Chapter 3](03-Tutorial.md) drew a cube in ten lines. This chapter continues from there, one step at a time, up to what the large demos do. Each step adds one idea and uses the ones before it. Each one starts from real code of the gallery: open the demo, read its **Source** panel, change a number.

What you will learn:

| Step | Idea | Demo |
|------|------|------|
| 1 | Your own look: a shader of yours inside the PBR shader | `10f_FlowingWater` |
| 2 | A picture made by the program | `10f_FlowingWater` |
| 3 | A mesh made by hand | `10f_FlowingWater` |
| 4 | Things you can see through | `10f_FlowingWater` |
| 5 | Moving the vertices on the GPU | `10e_Archipelago` |
| 6 | Values on a grid: `Field` | `11a_HeatPlate`, `11g_FlowAroundCylinder` |
| 7 | A field sent to a shader | `10f_FlowingWater` |
| 8 | Drawing into a texture: passes and ping-pong | `10c_SweatAndDecals` |
| 9 | What is under the mouse | `06l_CanvasPaint`, `10f_FlowingWater` |

You need chapter 3 §3 (a `Scene` over a `World`). The words *shader*, *uniform*, *texture* and *mesh* are in the glossary of [chapter 1](01-Introduction.md).

---

## 🎨 1. Your own look: the hook `main`

📁 `examples/10_Complete/10f_FlowingWater.cpp`, `FlowingWater/Shaders.hpp` · more in [chapter 6 §1](06-Renderer-Layer.md#-1-build-a-scene)

`color(...)` and `texture(...)` cover the simple cases. For anything else, write the `main()` of the fragment shader yourself, and keep the rest of the PBR shader of the Renderer: its sun, its lamps, its shadow, its fog and its tone curve. The PBR shader leaves a hole called a **hook** for it:

```cpp
compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
shaders.fragment().hook("main", flowing::SOLID_MAIN);
compages::renderer::Material material;
COMPAGES_TRY_ASSIGN(material,
                    compages::renderer::Material::create(
                        p_name, { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                  .shaders = std::move(shaders),
                                  .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                  .see_through = std::nullopt }));
material.textures = { { .sampler = "surfaceMap", .texture = p_picture } };
return m_scene.assets().addMaterial(material.name, std::move(material));
```

Your `main()` sees what the PBR shader declares: `vWorldPos`, `vNormal`, `vUv`, `baseColorFactor`, and the functions `shade(base, normal, position)`, `applyFog(colour, position)` and `present(colour)`. It writes `oColor`. Here, a picture projected along the three axes and blended by the normal (**triplanar mapping**): long walls need no texture coordinates.

```glsl
uniform sampler2D surfaceMap;
uniform float tileMetres;
uniform float waterLevel;

void main()
{
    vec3 n = normalize(vNormal);
    vec3 w = pow(abs(n), vec3(4.0));
    w /= (w.x + w.y + w.z);
    vec3 p = vWorldPos / tileMetres;
    vec3 c = texture(surfaceMap, p.zy).rgb * w.x + texture(surfaceMap, p.xz).rgb * w.y
           + texture(surfaceMap, p.xy).rgb * w.z;
    vec3 base = c * baseColorFactor;
    // ... darker and slimy near the water ...
    oColor = vec4(present(applyFog(shade(base, n, vWorldPos), vWorldPos)), opacity);
}
```

Then a shape wears it, and each frame you give its uniforms:

```cpp
m_scene.look(p_entity, p_material);
// each frame
material->program.set("tileMetres", 4.0f);
material->program.set("waterLevel", LEVEL);
```

- `material.textures` binds pictures to the samplers by name: the Renderer does it at each draw (at most five, units 2 to 6).
- One shader text, several materials: the concrete, the painted metal and the stripes of `10f` are three `Material`s of the same `SOLID_MAIN`, each with its own picture.
- Setting a uniform the shader does not use (or with the wrong type) is a **frame error**: `--check` fails. A GLSL compiler drops a uniform it never reads, so a uniform you set must really count in the result.
- A tint per shape: each shape has its own `MaterialInstance` (`base_color_factor`, in linear colours: `srgbToLinear(...)`).

---

## 🖼 2. A picture made by the program

📁 `examples/10_Complete/FlowingWater/Pictures.cpp` · more in [chapter 4](04-GPU-Layer.md)

A texture is an image in GPU memory. You can fill it from bytes you computed: noise, a normal map, concrete. Describe it, write the pixels, build the smaller copies used from afar (**mipmaps**), and give it to the assets of the scene:

```cpp
compages::gpu::TextureDesc desc =
    compages::gpu::TextureDesc::image(std::uint32_t(p_picture.size), std::uint32_t(p_picture.size),
                                      p_srgb ? compages::gpu::PixelFormat::SRGB8A8
                                             : compages::gpu::PixelFormat::RGBA8)
        .wrap(compages::gpu::Wrap::Repeat);
desc.levels = 0u;                       // as many mipmaps as the size allows
compages::renderer::TextureAsset asset;
asset.name = p_name;
COMPAGES_TRY_ASSIGN(asset.texture, compages::gpu::Texture::create(desc));
COMPAGES_TRY(asset.texture.write(std::as_bytes(std::span(p_picture.rgba))));
COMPAGES_TRY(asset.texture.generateMipmaps());
asset.description = desc;
return p_scene.assets().addTexture(p_name, std::move(asset));
```

- **sRGB** for colours painted for the eye (concrete, foam); plain `RGBA8` for numbers (noise, normals). The GPU turns sRGB into linear light when it reads, so the filtering stays right.
- `Wrap::Repeat` and a picture that **tiles** (its right edge meets its left edge): `tiledNoise()` wraps its lattice every `period` cells, so the noise has no seam.
- Rows start from the bottom.

---

## 🧱 3. A mesh made by hand

📁 `examples/10_Complete/FlowingWater/Spout.cpp` · more in [chapter 6 §1](06-Renderer-Layer.md#-1-build-a-scene)

A mesh is a list of corners (**vertices**: position, normal, texture coordinates) and a list of triangles (three indices into the corners). The jet of water of `10f` is a tube along the path of a thrown body, one ring of corners after another:

```cpp
MeshVertex vertex;
vertex.position = centre + (out * radius);
vertex.normal = out;
vertex.uv = Vector2f(float(k) / float(AROUND), length);   // round, along
mesh.source_vertices.push_back(vertex);
// ...two triangles between ring a and ring b:
p_mesh.source_indices.insert(p_mesh.source_indices.end(),
                             { a + k, b + k, a + k + 1u, a + k + 1u, b + k, b + k + 1u });
// ...at the end
p_mesh.index_count = p_mesh.source_indices.size();
p_mesh.local_bounds = compages::AABB::fromCorners(low, high);
```

```cpp
m_jet_body = m_scene.mesh(flowing::jetMesh(m_jet, LEVEL), "Water jet");
```

- `scene.mesh(mesh, name)` returns an entity, like `box()`. `scene.reshape(entity, mesh)` replaces its mesh later (`10e` does it for its tiles).
- `local_bounds` is the box around the corners: the Renderer skips a mesh outside the view, and `pick()` tests it.
- The corners stay on the CPU in `source_vertices`: `10f` reads them back to find where the scene meets the water (step 1 of [chapter 13 §3](13-Complete-Demos.md#-3-10f_flowingwater-water-that-flows-round-things)).

---

## 🫥 4. Things you can see through

📁 `examples/10_Complete/10f_FlowingWater.cpp` (`makeJet`) · more in [chapter 6 §1](06-Renderer-Layer.md#looks)

A see-through material gives a second state to `Material::create`. The Renderer uses it for a shape whose opacity is under one: such shapes are drawn after the solid ones, from the farthest, blended, without writing the depth.

```cpp
.state = { .depth_test = true, .cull = compages::gpu::CullMode::None },
.see_through = compages::gpu::RenderState{ .depth_test = true,
                                           .depth_write = false,
                                           .blend = compages::gpu::Blend::alpha(),
                                           .cull = compages::gpu::CullMode::None } }
// ...
renderer.flags = compages::renderer::RenderFlags::NoShadowCaster;
instance->opacity = 0.99f;
```

- `CullMode::None` draws both faces: the far side of the jet shows through the near one.
- Water casts no shadow: `RenderFlags::NoShadowCaster`.
- The shader writes its alpha into `oColor.a`, times `opacity`.

---

## 🌊 5. Moving the vertices on the GPU: the hook `deform`

📁 `examples/10_Complete/Archipelago/Shaders.hpp` · more in [chapter 6 §1](06-Renderer-Layer.md#your-own-material-hooks-of-the-pbr-shader)

The vertex shader of the PBR shader has a hook too. It receives each corner in world space and may move it. The sea of `10e` is a flat grid that the hook lifts into waves:

```cpp
water.vertex()
    .include("archipelago/swells.glsl", swells)
    .hook("deform", SURFACE_WAVES);
```

```glsl
void deform(inout vec4 world, inout vec3 normal)
{
    world = displace(world);   // Gerstner waves, from the table of swells
    normal = gWaveNormal;
}
```

- `include(name, text)` gives GLSL text a name: here the table of swells, written by C++ (`swellsGlsl()`). The boats read the same table on the CPU (`swellHeight()`): they ride the waves that are drawn.
- When the vertex shader replaces `deform`, `Material::create` also makes the program of the sun shadow with it: the shadow follows the waves.

---

## 🗺 6. Values on a grid: `Field`

📁 `examples/11_Science/11a_HeatPlate.cpp`, `11g_FlowAroundCylinder.cpp` · more in [chapter 6 §1](06-Renderer-Layer.md#show-a-field-heat-maps-and-coloured-reliefs)

A `Grid2D` says where points are: columns, rows, the first point, the step. A `Field<T>` holds one value per point, in one array. A field of a million points is one object, never a million entities.

```cpp
const compages::Grid2D grid{ .columns = POINTS,
                             .rows = POINTS,
                             .origin = { -0.5f * WIDTH, -0.5f * WIDTH },
                             .spacing = spacing };
m_temperature = compages::Field<float>(grid, ROOM);
// ...
m_plate = m_scene.show(m_temperature, m_look, "Plate");          // once
COMPAGES_TRY(m_scene.show(m_plate, m_temperature, m_look));     // each frame, new values
```

| Call | Does |
|------|------|
| `field(i, j)`, `field[k]` | the value of a point |
| `field.sample({ x, z })` | the value at any place, mixed from the four points around (bilinear) |
| `grid.point(k)`, `grid.nearest(p)`, `grid.neighbours8(k)` | from a point to its place, and back; its neighbours |
| `scene.show(Field<float>, FieldLook)` | a heat map, with relief or not |
| `scene.show(Field<Vector2f>, ArrowLook)` | arrows |
| `scene.probe(field, look, pixel)` | the point and the value under the mouse |

`10f` shows its flow as arrows with the same call (Try it → Arrows), and its floating crates read the flow where they are with `sample()`.

---

## 🚚 7. A field sent to a shader

📁 `examples/10_Complete/10f_FlowingWater.cpp` · more in [chapter 4](04-GPU-Layer.md)

`show()` draws a field in a way chosen for you. When your own shader needs the values, send the field as a texture. A `Field` numbers its points like the texels of a texture, so the values go as they are:

```cpp
compages::renderer::TextureAsset flow;
flow.name = "Flow map";
COMPAGES_TRY_ASSIGN(flow.texture, compages::gpu::Texture::fromField(m_bake.packed()));
COMPAGES_TRY_ASSIGN(m_flow_map, m_scene.assets().addTexture(flow.name, std::move(flow)));
// later, after a change of the values:
flow->texture.write(std::as_bytes(m_bake.packed().values()));
```

- `float` → R32F, `Vector2f` → RG32F, `Vector4f` → RGBA32F; a `Field<T, Grid3D>` gives a 3D texture (`11f_Volume`).
- The shader reads it at the centre of the texel of a point: `uv = ((p - origin) / spacing + 0.5) / cells`.
- The same `Field` serves the CPU (`sample()`) and the GPU (the texture): the crates float on the water that is drawn.

---

## 🖌 8. Drawing into a texture: passes and ping-pong

📁 `examples/10_Complete/10c_SweatAndDecals.cpp` · more in [chapter 4 §5 and §7](04-GPU-Layer.md#-5-compute)

The GPU layer draws into textures as well as onto the screen. A **pass** chooses where to draw (`RenderPass` on a `Framebuffer`). A simulation reads its state and writes the next one: two textures, read one, write the other, swap (**ping-pong**):

```cpp
for (int i = 0; i < steps; ++i)
{
    m_fluid_pass["previous"] = m_fluid.current();
    {
        compages::gpu::RenderPass const pass(
            m_fluid.next(), { .clear_color = false, .clear_depth = false });
        m_fluid_pass.draw(3u);          // one triangle that covers the texture
    }
    m_fluid.swap();
}
```

- `drawable["name"] = value` sets a uniform or binds a texture by the name of its sampler.
- `draw(3u)` with no vertices: the vertex shader makes the three corners from `gl_VertexID`, and one triangle covers the whole texture. Each pixel is one cell of the simulation.
- `10c` also draws the head **into its own UV map** (`gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0)`): one texel of the texture is then one piece of skin ([chapter 13 §1](13-Complete-Demos.md#-1-10c_sweatanddecals-a-simulation-on-the-skin)).

---

## 🖱 9. What is under the mouse

📁 `examples/06_Scene/06l_CanvasPaint.cpp`, `examples/10_Complete/10f_FlowingWater.cpp` (`click`)

`scene.pick(pixel)` gives the nearest shape under a pixel and the point it hits, as the last render saw it:

```cpp
if (p_frame.input.mouse_left)
{
    if (auto hit = m_scene.pick(p_frame.input.mouse))
    {
        if (hit->entity == m_canvas.id())
        {
            // hit->point is in world space
        }
    }
}
```

When you want a plane rather than a shape, make the ray yourself from the camera of the last render: two points of the pixel, unprojected.

```cpp
const Vector4f eye = camera.inverse_projection * Vector4f(x, y, p_depth, 1.0f);
const Vector4f world = camera.inverse_view * Vector4f(eye.x / eye.w, eye.y / eye.w, eye.z / eye.w, 1.0f);
```

- The pixel starts at the bottom left (`Input::mouse`), like OpenGL.
- `lastCamera()` is empty before the first render: check `viewport_width`.
- In the gallery the right button turns the camera (`Orbit`), so the left button is free for the demo.

---

## 📋 Summary

| I want to… | Call | Step |
|------------|------|------|
| write my own fragment shader and keep the lights | `shaders::pbr()`, `.hook("main", …)`, `Material::create` | 1 |
| give a uniform each frame | `material->program.set(name, value)` | 1 |
| make a texture from bytes | `TextureDesc::image`, `Texture::create`, `write`, `generateMipmaps`, `addTexture` | 2 |
| make a mesh from corners | `MeshAsset`, `scene.mesh`, `scene.reshape` | 3 |
| draw something see-through | `see_through` state, `opacity < 1` | 4 |
| move vertices on the GPU | `.hook("deform", …)`, `.include(name, text)` | 5 |
| keep values on a grid | `Grid2D`, `Field<T>`, `sample`, `scene.show` | 6 |
| give a field to my shader | `Texture::fromField`, `write(values())` | 7 |
| simulate on the GPU | `RenderPass`, `TexturePingPong`, `draw(3u)` | 8 |
| know what is under the mouse | `scene.pick`, `lastCamera()` | 9 |

Next: [13. Three complete demos, step by step](13-Complete-Demos.md), where these calls work together.
