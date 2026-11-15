# 🏝 11c_FieldProbe: what is under the mouse

[⏫ Case studies](README.md)

![11c_FieldProbe](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11c_FieldProbe.png)

📁 `examples/11_Science/11c_FieldProbe.cpp` · `Fields/HeightField.hpp`

The ray of a pixel is $\mathbf{r}(t) = \mathbf{o} + t\,\mathbf{d}$. It meets the relief where it goes below the ground:

```math
g(t) = r_y(t) - H\big(r_x(t),\ r_z(t)\big) \le 0
```

with $H$ the height drawn (the value times the relief of the look, `sample()` between points). `intersectHeight` walks along the ray by half a grid step until $g \le 0$, then halves the last step 8 times (**bisection**), so the point is found to $h/512$.

```cpp
if (const auto found = m_scene.probe(m_altitude, m_look, p_frame.input.mouse))
{
    // found->point: where; found->value: the altitude there
}
```

## 🚧 Limits

The walk steps by half a grid step: a peak thinner than that can be crossed without being seen. It reads the relief as drawn, so a look with no relief gives the flat ground.

## 🔗 See also

[Show a field: the value under the mouse](../guides/show-a-field.md#-the-value-under-the-mouse) · [Pick with the mouse](../guides/pick-with-the-mouse.md)
