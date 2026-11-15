# 11g_FlowAroundCylinder: ideal flow and a grid fitted round the column

[Case studies](README.md)

![11g_FlowAroundCylinder](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/11g_FlowAroundCylinder.png)

📁 `examples/11_Science/11g_FlowAroundCylinder.cpp`

## The flow

A uniform wind $U$ round a column of radius $R$ that turns (circulation $\Gamma$). In polar coordinates $(r, \theta)$ round the column, the **ideal flow** (no friction) is a uniform flow plus a doublet plus a vortex:

```math
u_r = U\left(1 - \frac{R^2}{r^2}\right)\cos\theta, \qquad
u_\theta = -U\left(1 + \frac{R^2}{r^2}\right)\sin\theta + \frac{\Gamma}{2\pi r}
```

The pressure follows from Bernoulli (faster is lower), as a coefficient:

```math
C_p = 1 - \frac{|\mathbf{u}|^2}{U^2}
```

With a turn, the wind is faster on one side: the pressure is lower there and the column is pushed sideways (the **Magnus effect**; the force per metre is $\rho\,U\,\Gamma$, Kutta-Joukowski).

## Streamlines

`compages::streamline(field, start, step, steps)` follows the field with the midpoint method (Runge-Kutta 2), $\hat{\mathbf{v}} = \mathbf{v} / |\mathbf{v}|$:

```math
\mathbf{x}_{n+1} = \mathbf{x}_n + \Delta s\;\hat{\mathbf{v}}\!\left(\mathbf{x}_n + \tfrac{\Delta s}{2}\,\hat{\mathbf{v}}(\mathbf{x}_n)\right)
```

## A grid fitted round the column

On a regular `Grid2D`, the round column is a staircase of square cells: the colours step along its wall. A **body-fitted** grid follows the shape: here an **O-grid**, $i$ round the column and $j$ away from it. Point $(i, j)$ lies on the straight line from the wall $\mathbf{w}_i$ to the edge of the channel $\mathbf{e}_i$ (equal steps along the edge, so a point falls on each corner), with steps that grow away from the wall:

```math
\mathbf{p}_{ij} = \mathbf{w}_i + \big(\mathbf{e}_i - \mathbf{w}_i\big)\,\frac{e^{b\,u} - 1}{e^{b} - 1}, \qquad u = \frac{j}{n_r - 1},\quad b = 3
```

```cpp
compages::CurvilinearGrid ogrid;
COMPAGES_TRY_ASSIGN(ogrid, compages::CurvilinearGrid::create(AROUND + 1, OUT, fitted));
m_fitted_pressure = compages::Field<float, compages::CurvilinearGrid>(ogrid, 0.0f);
m_fitted_floor = m_scene.show(m_fitted_pressure, m_pressure_look, "Pressure, fitted grid");
```

The ring closes on itself: its first column is repeated as its last one. Same calls as on a `Grid2D` (`field(i, j)`, `sample()`, `streamline()`, `show()`); only `locate()` differs. Its points are not regular, so finding the cell of a place is a **search**: the cells are sorted into buckets once; then, in the few cells of the bucket, the inverse of the bilinear map of a cell

```math
\mathbf{p}(s, t) = \mathbf{a} + (\mathbf{b} - \mathbf{a})\,s + (\mathbf{c} - \mathbf{a})\,t + (\mathbf{a} - \mathbf{b} - \mathbf{c} + \mathbf{d})\,s\,t
```

is found by Newton's method (a 2 × 2 system per step). A place is in the cell when $0 \le s, t \le 1$.

Try it: Grid (regular or fitted), Grid lines, the speed, the turn.

## Limits

An ideal flow has no friction: no boundary layer, no wake, no vortices shed behind the column. Real air separates behind it. The formula is exact for this model, not for real air.

## See also

[Show a field: vectors and streamlines](../guides/show-a-field.md#vectors-and-streamlines) · [Fields and grids](../concepts/fields-and-grids.md)
