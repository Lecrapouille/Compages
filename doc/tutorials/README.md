# 🎓 Tutorials

![The workshop at the last step](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13d_RobotEye.png)

Compages has three layers, and the tutorials follow them from the bottom up. First the GPU layer alone: a shader and some numbers. Then the World: a robot arm that moves, with no screen at all. Then the Renderer, which draws that arm in a small workshop and adds one idea per page: values on a grid, a shader of your own, a second camera.

1. [Install and run](01-install.md)
2. [The GPU](02-the-gpu.md): a triangle and a compute shader
3. [The World](03-the-world.md): a robot arm without a screen
4. [A scene](04-a-scene.md): the arm drawn in a workshop, then in your own window
5. [Values on a grid](05-values-on-a-grid.md): the heat the arm leaves on a plate
6. [A look of your own](06-a-look-of-your-own.md): worn paint, from a shader hook
7. [What the robot sees](07-what-the-robot-sees.md): a camera on the arm, read by the program

Each page has a demo in the gallery. Run it before you read the page. The code of the workshop is in [examples/13_Workshop/Workshop/](../../examples/13_Workshop/Workshop/): change it and rebuild with `make`.

You need basic C++: variables, functions, structs, a loop, a lambda. You do not need to know 3D or OpenGL. The words are in the [glossary](../concepts/glossary.md).
