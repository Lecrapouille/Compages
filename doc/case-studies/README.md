# Case studies

[Documentation](../../README.md#-documentation)

A case study follows one demo from its inputs to its final picture: the problem, the technique and where it comes from, the data and how it is made, the shaders, what Compages does and what the demo does, and the limits. The pages do not depend on each other. Run the demo first. Then follow the data from one step to the next, and read the equations only when you know what each step is for.

The snippets are parts of the real implementations, not files to paste. Helpers such as `Land`, `Patch` or the flow-map baker belong to the demos, not to Compages; the source paths say where they live.

## Complete demos

| Demo | Page | What it adds |
|------|------|--------------|
| `10c_SweatAndDecals` | [Sweat and decals](sweat-and-decals.md) | a simulation on the GPU, in the UV map of a mesh: several passes, ping-pong |
| `10e_Archipelago` | [Archipelago](archipelago.md) | a whole world made on the CPU, in a thread: cells, erosion, rivers, levels of detail, a sea |
| `10f_FlowingWater` | [Flowing water](flowing-water.md) | a technique of a game studio: a flow map baked from the scene, read by the GPU and the CPU |

## Science with fields

The `11_Science` demos compute physics on a grid and show it. Each page gives the physical problem, the equation, the loop on the grid, where it runs (CPU or GPU), the picture and the limits. Start with the heat plate.

| Demo | Page | The physics | The Compages idea |
|------|------|-------------|-------------------|
| `11a_HeatPlate` | [Heat plate](heat-plate.md) | heat equation | a `Field<float>` updated on the CPU, `scene.show()` |
| `11b_RippleTank` | [Ripple tank](ripple-tank.md) | wave equation | the same on the GPU: a compute shader, three buffers |
| `11c_FieldProbe` | [Field probe](field-probe.md) | a ray meets a relief | `scene.probe()` |
| `11d_BimetalBeam` | [Bimetal beam](bimetal-beam.md) | heat + thermal bending | a CPU model bends a mesh on the GPU (hook `deform`) |
| `11e_Smoke` | [Smoke](smoke.md) | incompressible fluid (Stable Fluids) | five compute kernels, ping-pong |
| `11f_Volume` | [Volume](volume.md) | light through warm air | `Field<float, Grid3D>`, a 3D texture, an isosurface |
| `11g_FlowAroundCylinder` | [Flow around a cylinder](flow-around-cylinder.md) | ideal flow round a column | vectors, streamlines, a grid fitted round a shape |
| `11h_Thermometers` | [Thermometers](thermometers.md) | following values in time | `Chart` |

### How to read the equations

| Symbol | Read it as |
|--------|------------|
| $i, j$ | column and row of a point, not a distance |
| $h$ | distance between neighbouring grid points |
| $n$, $n+1$ | current step and next step |
| $\Delta t$ | duration of one step, in seconds |
| $\partial T / \partial t$ | how fast the temperature changes with time |
| $\nabla^2 T$ | how the value differs from the values around it |

The physical models and solvers belong to the demos. Compages holds the data, gives the GPU tools and shows the result; creating a `Field` does not make heat or fluid move by itself ([fields and grids](../concepts/fields-and-grids.md)).

### The equations at a glance

| Equation | Discrete form | Page |
|----------|---------------|------|
| $\partial_t T = \alpha \nabla^2 T$ | explicit Euler, five-point Laplacian, $\alpha \Delta t / h^2 \le \tfrac14$ | [heat plate](heat-plate.md), [bimetal beam](bimetal-beam.md) |
| $\partial_t^2 u = c^2 \nabla^2 u$ | leapfrog, three buffers, $(c \Delta t / h)^2 \le \tfrac12$ | [ripple tank](ripple-tank.md) |
| $\nabla\cdot\mathbf{u} = 0$ | advect back, Jacobi on $\nabla^2 p = \nabla\cdot\mathbf{u}$, $\mathbf{u} - \nabla p$ | [smoke](smoke.md) |
| light through a medium | $\alpha_i = 1 - e^{-k t_i \Delta l}$, front to back | [volume](volume.md) |
| ideal flow | uniform + doublet + vortex | [flow around a cylinder](flow-around-cylinder.md) |

## Practice: keep calculation and display apart

In `11a_HeatPlate`, change one control at a time: the colour map, then the relief, then the speed of time. The first two change only the picture; the third changes how fast heat spreads, and the solver still keeps its step under the limit. Then follow one value from the CPU update to `scene.show`, and find where the old and new fields swap.

Before you adapt a solver, write down its units, its boundary conditions and its stability limit. A plausible picture alone does not validate a numerical result.
