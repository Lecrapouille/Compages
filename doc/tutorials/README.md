# 🎓 Tutorials

![The workshop at the last step](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13d_RobotEye.png)

You want to move a robot arm, draw it, and read what its camera sees. Start with one triangle. Then build the arm without a screen, before adding its workshop. The later steps keep that workshop and add heat, worn paint, and a camera.

You need basic C++: functions, structs, loops, inheritance, and a lambda. You do not need previous 3D or OpenGL experience. The commands use Linux and run from the `Compages` repository folder.

0. [Glossary](00-glossary.md): coordinates and units first; use the other entries when a word is new.
1. [Install and run](01-install.md): build the library, see a triangle, and compile your first program.
2. [The GPU](02-the-gpu.md): connect shader inputs to data, then compute twelve numbers.
3. [The World](03-the-world.md): move one entity, attach a child, then build the robot arm without graphics.
4. [A scene](04-a-scene.md): give the arm shapes and lights, then open your own window.
5. [Values on a grid](05-values-on-a-grid.md): store and inspect temperatures, then spread heat under the tool.
6. [A look of your own](06-a-look-of-your-own.md): replace a surface colour, then use UV coordinates to wear its edges.
7. [What the robot sees](07-what-the-robot-sees.md): show a second view, read its pixels, and measure a colour.

Run the demo near the start of each practical chapter. Observe its result before changing its code. Change one thing, rebuild with `make`, then run the same command again. Each “Try” gives an expected result so you can check your change. Restore the change before the next exercise unless the page says otherwise.

Complete programs say which file to create and give a build command. Other code blocks are excerpts: edit the linked source file rather than pasting them into an empty program. The gallery supplies the window and calls `setUp()` once, then `draw()` at each frame. Chapter 4 shows how to supply that loop yourself.

The layers solve separate problems. The GPU executes shaders; the World moves entities; the Renderer draws a World. Learning them in this order explains their boundaries. World-only programs need no graphics context, even though you learn the GPU first.
