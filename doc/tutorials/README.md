# 🎓 Tutorials: the robot workshop

[⏫ Documentation](../README.md)

![The workshop at its last step](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/00e_RobotEye.png)

These tutorials build one small program, page after page: a workshop with a table, then a robot arm that moves, then the heat its tool leaves on a steel plate, then a paint of its own, and last a camera that lets the robot see. Each page adds one idea of Compages and ends with something new on the screen.

Read them in order. Each step is a demo of the gallery (`00a_Workshop` to `00e_RobotEye`), so you can run it before you read it, and its code is a file of [examples/00_Workshop/Workshop/](../../examples/00_Workshop/Workshop/) that you can change and rebuild.

| # | Page | What you get | Demo |
|---|------|--------------|------|
| 1 | [Install and run](01-install.md) | Compages built, the gallery open | `00a_Workshop` |
| 2 | [A first scene](02-first-scene.md) | a table, a part, a lamp; then the same scene in your own window | `00a_Workshop` |
| 3 | [Make it move](03-make-it-move.md) | an arm of three joints, moved by a routine | `00b_RobotArm` |
| 4 | [Values on a grid](04-values-on-a-grid.md) | the heat of the tool on a plate, computed and shown | `00c_HotTable` |
| 5 | [A look of your own](05-your-own-look.md) | warning stripes painted by ten lines of GLSL | `00d_PaintedArm` |
| 6 | [What the robot sees](06-what-the-robot-sees.md) | a camera on the arm, its picture read by the program | `00e_RobotEye` |
| 7 | [The GPU alone](07-the-gpu-alone.md) | a triangle and a compute shader, with no scene at all | `01c_Triangle`, `05a_Compute` |

You need basic C++: variables, functions, structs, a loop, a lambda. You do not need to know 3D or OpenGL: each 3D word is linked to the [glossary](../concepts/glossary.md) the first time it appears.

After the tutorials, choose a path in the [documentation map](../README.md): the [guides](../guides/README.md) for one task, the [techniques](../techniques/README.md) for one effect, the [concepts](../concepts/README.md) to understand why Compages is built this way.
