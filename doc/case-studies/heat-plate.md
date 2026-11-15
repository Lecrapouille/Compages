# 🔥 11a_HeatPlate: the heat equation

[⏫ Case studies](README.md)

![11a_HeatPlate](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11a_HeatPlate.png)

📁 `examples/11_Science/11a_HeatPlate.cpp`

Heat spreads from warm places to cold ones. The temperature $T$ follows the **heat equation**, with $\alpha$ the diffusivity of the metal (m²/s):

```math
\frac{\partial T}{\partial t} = \alpha\,\nabla^2 T = \alpha\left(\frac{\partial^2 T}{\partial x^2} + \frac{\partial^2 T}{\partial z^2}\right)
```

On a grid of step $h$, the **Laplacian** $\nabla^2 T$ is the sum of the four neighbours minus four times the point (the five-point stencil):

```math
\nabla^2 T_{ij} \approx \frac{T_{i-1,j} + T_{i+1,j} + T_{i,j-1} + T_{i,j+1} - 4\,T_{ij}}{h^2}
```

One small step of time $\Delta t$ forward (explicit Euler):

```math
T_{ij}^{n+1} = T_{ij}^{n} + r\left(T_{i-1,j} + T_{i+1,j} + T_{i,j-1} + T_{i,j+1} - 4\,T_{ij}\right)^{n}, \qquad r = \frac{\alpha\,\Delta t}{h^2}
```

It stays stable only while $r \le \tfrac14$: a longer frame is cut into several steps. The code keeps $r \le 0.2$:

```cpp
const float limit = 0.2f * h2 / DIFFUSIVITY;
const int steps = std::max(1, int(std::ceil(p_seconds / limit)));
const float rate = DIFFUSIVITY * (p_seconds / float(steps)) / h2;
// ...
m_next(i, j) = edge ? ROOM
               : heated ? HEATER
               : t + (rate * (m_temperature(i - 1, j) + m_temperature(i + 1, j) +
                              m_temperature(i, j - 1) + m_temperature(i, j + 1) - (4.0f * t)));
```

**One step with real numbers:** take the centre value 100, four neighbours at 20 and $r = 0.1$. The update gives $100 + 0.1(20 + 20 + 20 + 20 - 4 \times 100) = 68$. A hotter point cools toward its neighbours. This is one local update; heaters and edge rules may hold some other points fixed.

- Two fields, `m_temperature` and `m_next`: read one, write the other, `std::swap`. Writing in place would mix old and new values.
- The edges stay at room temperature (a **boundary condition**); the heaters are held hot.
- `scene.show(m_plate, m_temperature, m_look)` each frame sends the new values; the mesh never changes.

Try it: the colour map, the relief (the temperature as a height), the speed of time.

## 🚧 Limits

- The explicit step is stable only while $r = \alpha\,\Delta t / h^2 \le \tfrac14$. Halving the step $h$ of the grid gives four times the points and four times the steps: sixteen times the work. An implicit solver would take large steps; it is not in Compages.
- The CPU loop is fine for 96 × 96 points. For a much larger grid at every frame, compute on the GPU: [the ripple tank](ripple-tank.md).

## 🔗 See also

[Values on a grid](../tutorials/04-values-on-a-grid.md) (the same loop, on the workshop table) · [Show a field](../guides/show-a-field.md) · [Fields and grids](../concepts/fields-and-grids.md)
