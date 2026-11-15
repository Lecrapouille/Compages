# Show a field

[Guides](README.md)

Show values on a grid as colours, as a relief, as arrows, as a see-through volume, as curves over time; read the value under the mouse; give the values to a shader of your own.

![11g_FlowAroundCylinder](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11g_FlowAroundCylinder.png)

What a grid and a field are: [fields and grids](../concepts/fields-and-grids.md). The first steps: [values on a grid](../tutorials/05-values-on-a-grid.md).

## Colours and relief

```cpp
compages::renderer::FieldLook look{ .colors = compages::renderer::ColorMap::Inferno,
                                    .low = 0.0f, .high = 300.0f,   // first and last colour
                                    .relief = 0.0015f,             // metres per unit; 0 = flat
                                    .lit = true };                 // false: exact colours, like a chart
compages::world::Entity plate = scene.show(temperature, look, "Plate");
// later, new values or another look, same grid size:
COMPAGES_TRY(scene.show(plate, temperature, look));
```

The field lies on the ground of its grid (x, z); move its entity to lift it. The maps are `Viridis` (readable in grey and by colour-blind eyes), `Inferno` (heat), `Coolwarm` (below and above a middle), `Terrain` and `Grey`. The values go to the GPU as a storage buffer and the shader makes the colours and the heights: a new range or new values never rebuild the mesh. A lit field casts the shadow of its relief in the sun. `11a_HeatPlate` solves the heat equation on a plate and shows it each frame.

**Values computed on the GPU**: give the storage buffer instead of a `Field`, one float per point, row after row; nothing comes back to the CPU. The buffer must live as long as the entity draws.

```cpp
m_surface = m_scene.show(m_grid, m_heights[m_now], m_look, "Water");   // setUp()
// each frame, after the compute steps and their barrier(Barrier::Storage):
COMPAGES_TRY(m_scene.show(m_surface, m_heights[m_now], m_look));
```

`11b_RippleTank` steps the wave equation this way.

## The value under the mouse

`scene.probe(field, look, pixel)` follows the ray of a pixel of the last picture down to the field as drawn (with its relief), and returns the point and the value there, or nothing:

```cpp
if (const auto found = m_scene.probe(m_altitude, m_look, p_frame.input.mouse))
{
    m_probe.position(found->point).set(Tooltip{ .title = std::format("{:.0f} m", found->value), … });
}
```

`11c_FieldProbe` writes the altitude of an island next to the mouse.

## Vectors and streamlines

A `Field<Vector2f>` (a wind, a speed: x and y of each vector along x and z) is shown as arrows coloured by their length, one point in `every`:

```cpp
m_arrows = m_scene.show(m_wind, compages::renderer::ArrowLook{ .colors = compages::renderer::ColorMap::Viridis,
    .low = 0.0f, .high = 2.0f, .scale = 0.12f, .every = 4 }, "Wind");
```

`compages::streamline(field, start, step, steps)` (`<Compages/Fields/Streamline.hpp>`) gives the line a speck would follow; draw it with `scene.debug().line()`. `show()` also takes fields on a `CurvilinearGrid`: the mesh follows the points of the grid, so the edge of a column is round, not a staircase. `11g_FlowAroundCylinder` compares the two ([its case study](../case-studies/flow-around-cylinder.md)).

## Volumes

A `Field<float, Grid3D>` is shown as a see-through box: along each ray, the colours of the values add up. It goes to the GPU as a 3D texture. `compages::slice(volume, Axis::Y, layer)` cuts one layer as a 2D field; `compages::isosurface(volume, level)` gives the triangles where the volume crosses a level, and `meshOf(surface)` a mesh of them.

```cpp
m_room = m_scene.show(m_heat, m_look, "Room");
m_cut = m_scene.show(compages::slice(m_heat, compages::Axis::Y, layer), m_cut_look, "Cut");
m_cut.position(0.0f, m_heat.grid().point(0, layer, 0).y, 0.0f);
```

`11f_Volume` shows the warm air of a room this way, with the surface of the air at 30 degrees.

## Curves over time

A `Chart` (`<Compages/Renderer/Visualization/Chart.hpp>`) draws curves of values over time into a `CanvasTexture`, to put on any mesh:

```cpp
COMPAGES_TRY(m_chart.create(m_scene, { .width = 1024u, .height = 512u, .seconds = 120.0f,
                                       .low = 0.0f, .high = 100.0f,
                                       .scale = ChartScale::FollowValues }, "Thermometers"));
const std::size_t curve = m_chart.series(CanvasTexture::Color::rgb(255, 140, 0));
m_chart.applyLook(m_scene, board.id());
// each frame:
m_chart.add(curve, time, temperature.sample(spot));
COMPAGES_TRY(m_chart.draw());
```

A grid on round numbers and the numbers of both axes are drawn with it. `ChartScale::Fixed` keeps `low` to `high`; `FollowValues` follows the values. `11h_Thermometers` follows two points of a heated plate.

## The values in your own shader

When your shader needs the field, send it as a texture. A `Field` numbers its points like the texels of a texture, so the values go as they are:

```cpp
compages::renderer::TextureAsset flow;
flow.name = "Flow map";
COMPAGES_TRY_ASSIGN(flow.texture, compages::gpu::Texture::fromField(m_bake.packed()));
COMPAGES_TRY_ASSIGN(m_flow_map, m_scene.assets().addTexture(flow.name, std::move(flow)));
// later, after a change of the values:
flow->texture.write(std::as_bytes(m_bake.packed().values()));
```

`float` gives R32F, `Vector2f` RG32F, `Vector4f` RGBA32F; a `Field<T, Grid3D>` a 3D texture. The shader reads a point at the centre of its texel: `uv = ((p - origin) / spacing + 0.5) / cells`. The texture is a copy: after a change of the field, write it again. The CPU still reads the field with `sample()`, with no upload. [Flow mapping](../techniques/flow-mapping.md) does this.

## See also

[Fields and grids](../concepts/fields-and-grids.md) · [Case studies](../case-studies/README.md) (the `11_Science` demos) · [Reference: fields](../reference/api.md#6-fields)
