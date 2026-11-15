# 11h_Thermometers: values over time

[Case studies](README.md)

![11h_Thermometers](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11h_Thermometers.png)

📁 `examples/11_Science/11h_Thermometers.cpp` · `Renderer/Visualization/Chart.hpp`

Two thermometers read a heated plate (as in [the heat plate](heat-plate.md)) with `field.sample(spot)`, and a `Chart` draws their curves on a board. With `ChartScale::FollowValues`, the axis takes **round numbers** round the values shown. The step of the grid is a quarter of the range, rounded to the nearest of $1, 2, 5$ or $10$ times a power of ten:

```math
\text{step} = m \cdot 10^{n}, \qquad n = \left\lfloor \log_{10} \tfrac{v_{\max} - v_{\min}}{4} \right\rfloor,\quad m \in \{1, 2, 5, 10\}
```

Then the bottom and the top go out to multiples of the step (one step more where a curve would touch the frame).

```cpp
COMPAGES_TRY(m_chart.create(m_scene,
                            { .width = 1024u, .height = 512u, .seconds = 120.0f,
                              .low = 0.0f, .high = 100.0f,
                              .scale = compages::renderer::ChartScale::FollowValues },
                            "Thermometers"));
// each frame
m_chart.add(m_curves[k], m_time, m_temperature.sample(m_spots[k]));
COMPAGES_TRY(m_chart.draw());
```

The board shows the picture as it is, like a screen: a hook `main` that only reads the picture, so the sun does not light it.

## See also

[Show a field: curves over time](../guides/show-a-field.md#curves-over-time) · [Paint a texture](../guides/paint-a-texture.md)
