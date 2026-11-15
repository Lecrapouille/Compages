# 🧱 Make pictures and meshes

[⏫ Guides](README.md)

Build a texture from pixels your program computes (noise, a normal map, concrete), and a shape from triangles your program builds (a tube, a terrain, a tile).

![10f_FlowingWater](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10f_FlowingWater.png)

Every picture and the jet of water of `10f_FlowingWater` are made this way: no file is read.

## 🖼 A picture made by the program

From [FlowingWater/Pictures.cpp](../../examples/10_Complete/FlowingWater/Pictures.cpp): describe the texture, write the pixels, build the smaller copies used from afar (**mipmaps**), and give it to the catalog of the Scene:

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

- **sRGB** for colours painted for the eye (concrete, foam); plain `RGBA8` for numbers (noise, normals). The GPU turns sRGB into linear light when it reads, so filtering stays right.
- `Wrap::Repeat` and a picture that **tiles**: its right edge meets its left edge. The noise of `10f` wraps its lattice every `period` cells, so it has no seam.
- Rows start from the bottom, as in OpenGL.

## 🔺 A mesh made by hand

A mesh is a list of corners (**vertices**: position, normal, texture coordinates) and a list of triangles (three indices into the corners). The jet of water of `10f` is a tube along the path of a thrown body, one ring of corners after another, from [FlowingWater/Spout.cpp](../../examples/10_Complete/FlowingWater/Spout.cpp):

```cpp
compages::renderer::MeshVertex vertex;
vertex.position = centre + (out * radius);
vertex.normal = out;
vertex.uv = compages::Vector2f(float(k) / float(AROUND), length);   // round, along
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

- `scene.mesh(mesh, name)` returns an entity, like `box()`. `scene.reshape(entity, mesh)` replaces its mesh later; its copies change with it (`10e_Archipelago` does it for its tiles).
- `local_bounds` is the box round the corners: the Renderer skips a mesh outside the view with it, and `pick()` tests it. Wrong bounds make a shape vanish at the edge of the view.
- The corners stay on the CPU in `source_vertices`: `10f` reads them back to find where the scene meets the water.
- A field becomes a mesh with `compages::renderer::meshOf(…)`: an isosurface of a volume, for instance ([show a field](show-a-field.md)).

## 🔗 See also

[Write a material](write-a-material.md) (read the picture in your shader) · [Paint a texture](paint-a-texture.md) (a picture that changes) · [Flowing water](../case-studies/flowing-water.md)
