# 🌡 11f_Volume: seeing through warm air

[⏫ Case studies](README.md)

![11f_Volume](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11f_Volume.png)

📁 `examples/11_Science/11f_Volume.cpp` · `Fields/Isosurface.hpp`, `Fields/Slice.hpp`

The temperature of a room is a `Field<float, Grid3D>`. `scene.show(volume, look)` sends it as a 3D texture and draws a box. For each pixel, the shader walks the ray through the box in $N$ steps of length $\Delta l$. Each step glows with the colour $c_i$ of its value and hides what is behind (**emission and absorption**):

```math
\alpha_i = 1 - e^{-k\,t_i\,\Delta l}, \qquad C = \sum_{i} c_i\,\alpha_i \prod_{j<i} (1 - \alpha_j)
```

with $t_i$ the value placed in the range (0 to 1), and $k = 3 / |\text{box}|$: at the top of the range, $e^{-3} \approx 5\,\%$ of the light crosses the box.

Two other ways to look at a volume:

- `compages::slice(volume, Axis::Y, layer)`: one layer as a 2D field, for `scene.show()`.
- `compages::isosurface(volume, level)`: the surface where the value equals a level (here the air at 30°), by **marching tetrahedra**. Each cube of 8 points is cut into 6 tetrahedra; on each edge whose ends are on both sides of the level, the surface crosses at

```math
\mathbf{p} = \mathbf{a} + \frac{\ell - f_a}{f_b - f_a}\,(\mathbf{b} - \mathbf{a})
```

Tetrahedra have no ambiguous cases (unlike the cubes of marching cubes). `renderer::meshOf(surface)` makes a mesh of it.

## 🚧 Limits

The volume emits and absorbs; it does not scatter light, and the sun does not light it. For fog that the light of the scene crosses, see [light the scene](../guides/light-the-scene.md#-fog-that-the-light-crosses).

## 🔗 See also

[Show a field: volumes](../guides/show-a-field.md#-volumes) · [Fields and grids](../concepts/fields-and-grids.md)
