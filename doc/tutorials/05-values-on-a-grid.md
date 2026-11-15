# 🗺 5. Values on a grid

[◀ A scene](04-a-scene.md) · [⏫ Tutorials](README.md) · [Next: a look of your own ▶](06-a-look-of-your-own.md)

![13b_HotPlate](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13b_HotPlate.png)

The tip of the arm is hot: 300 degrees. Where it touches the steel plate on the table, the plate heats; the heat spreads around and fades. You will store the temperature of the plate as one number per point of a grid, compute it at each frame, and show it in colours with one call.

## 🗺 A grid and a field

A **grid** says where the points are. A **field** holds one value per point. From [Workshop/HotPlate.cpp](../../examples/13_Workshop/Workshop/HotPlate.cpp):

```cpp
const compages::Grid2D grid{ .columns = 41, .rows = 61, .origin = { -0.25f, -0.45f }, .spacing = 0.015f };
m_temperature = compages::Field<float>(grid, ROOM);
m_next = m_temperature;
```

There are 41 columns along x and 61 rows along z, with one point every 1.5 cm. The first point is at (−0.25, −0.45). This makes a plate of 60 cm by 90 cm under the reach of the arm. `Field<float>(grid, ROOM)` makes 41 × 61 = 2501 temperatures, all at 20 degrees. `m_temperature(i, j)` reads or writes the value of column `i`, row `j`; `grid.point(i, j)` says where that point is, in metres.

A `Grid2D` and a `Field` are plain C++ data in the `compages` namespace, in `<Compages/Fields/…>`. They do not need the World, the GPU or a screen: a unit test or a solver uses them as they are. The concept page [fields and grids](../concepts/fields-and-grids.md) has the whole picture.

## ♨ One step of heat

At each frame, each point moves toward the mean of its four neighbours: this is the heat equation, in its simplest discrete form. It also loses a little heat to the air, and the points under the tip take its temperature:

```cpp
const float rate = DIFFUSIVITY * p_seconds / (grid.spacing * grid.spacing); // < 0.25 to stay stable
const bool touches = (p_tool.y < TABLE_TOP + 0.01f);
for (int j = 1; j < grid.rows - 1; ++j)
{
    for (int i = 1; i < grid.columns - 1; ++i)
    {
        const compages::Vector2f p = grid.point(i, j);
        const float t = m_temperature(i, j);
        const float spread = m_temperature(i - 1, j) + m_temperature(i + 1, j) + m_temperature(i, j - 1) +
                             m_temperature(i, j + 1) - (4.0f * t);
        const bool under = touches && (std::hypot(p.x - p_tool.x, p.y - p_tool.z) < TIP_RADIUS);
        m_next(i, j) = under ? TIP : t + (rate * spread) - (COOLING * p_seconds * (t - ROOM));
    }
}
std::swap(m_temperature, m_next);
```

`spread` is the sum of the four neighbours minus four times the point itself: zero where the temperature is flat, positive in a cold point between warm ones. The new values go into a second field, `m_next`, and the two are swapped at the end: a point must not read a neighbour that was already updated in the same step. The edge points are never written; they stay at room temperature.

The step needs to know where the tip is. The demo asks the World, after it moved the arm:

```cpp
m_scene.update(p_frame);                                                     // the arm moves
m_plate.step(std::min(p_frame.elapsed, 0.05f), m_arm.tool.worldPosition()); // then it heats
```

`worldPosition()` is the place of the tip in the room, through the whole chain of joints of page 3. `Scene::draw()` is `update()` then `render()`; the demo calls the two halves, with its own work in between.

## 🎨 Show it

```cpp
m_shown = p_scene.show(m_temperature, m_look, "Plate");   // once
m_shown.position(0.0f, TABLE_TOP + 0.003f, 0.0f);         // on the table
// each frame, after step():
p_scene.show(m_shown, m_temperature, m_look);
```

The look says how a value becomes a colour:

```cpp
compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Inferno,
                                      .low = 20.0f,
                                      .high = 300.0f,
                                      .relief = 0.0f,
                                      .lit = true };
```

Inferno goes from black (20 degrees, `low`) through purple and orange to pale yellow (300 degrees, `high`): the colours of hot metal. `relief` would raise each point by its value (zero: a flat plate). `lit` lets the sun and the lamp light the plate, like any surface.

The first `show()` makes an entity that draws the field, lying flat on the ground of its grid; `position()` lifts it 3 mm above the table, so that the two surfaces do not fight for the same pixels. The next calls send the new values only. The GPU turns them into colours; the mesh is never rebuilt.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 13b_HotPlate
```

A glowing trail follows the tip and fades to purple, then black. The demo runs twelve seconds of work before its first frame (`warmUp()`), so that the trail is there at once.

## 🧪 Try

- Set `.colors` to `ColorMap::Viridis`: the same values in colours a colour-blind eye reads well.
- Set `.relief = 0.0005f`: the hot points rise, half a millimetre per degree.
- Multiply `COOLING` by 10: the trail is short, only the tip glows.
- Set `DIFFUSIVITY` to `1.0e-2f`: `rate` goes above 0.25 and the step explodes into stripes. An explicit step is only stable for small steps of time; [the heat plate](../case-studies/heat-plate.md) explains the limit.

Most scientific data has this shape: one value per point of a space. In Compages the grid says where the points are, the field holds the values, and the look says how to show them. You compute on the field with plain loops, and the Renderer shows it with one call. The same field can also be sent to your own shader ([show a field](../guides/show-a-field.md)), read under the mouse, or computed on the GPU ([the ripple tank](../case-studies/ripple-tank.md)).

---

[◀ A scene](04-a-scene.md) · [⏫ Tutorials](README.md) · [Next: a look of your own ▶](06-a-look-of-your-own.md)
