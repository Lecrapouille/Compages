# Draw helpers and debug lines

[Guides](README.md)

See sizes, axes, bounds, paths, forces and what a camera sees, while you build and debug a scene; and keep them out of the picture of a robot camera.

![06j_SplitViews](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06j_SplitViews.png)

## Helpers: entities like any other

```cpp
scene.grid(10.0f, 20u);               // a grid on the ground, 20 cells across
scene.axes(0.5f).parent(arm);         // X red, Y green, Z blue, turning with the arm
scene.showBounds(cube.id(), true);    // the box round the mesh of the cube
```

They are entities or components: move them, hang them from a parent, remove them. `06b_Transforms` uses the three; `08d_RobotArm` shows the frame of every joint of a robot.

## Debug lines: one frame only

`scene.debug()` queues lines for the next render, then forgets them: call it each frame for what must stay.

| Call | Draws |
|------|-------|
| `line(a, b, colour)`, `ray(…)`, `box(…)` | a segment, a ray, a box |
| `polyline(points, colour)` | a path |
| `arrow(from, to, colour)` | a force, a speed |
| `frame(pose, length)` | the axes of a pose |
| `frustum(pose, lens, aspect, length, colour)` | what a camera sees |

`06j_SplitViews` draws on its map, with `frustum`, what the left camera sees; `10a_MvpDemo` draws debug boxes.

## Hidden from a sensor

A camera that stands for a sensor must not see what is drawn for the person at the screen:

```cpp
robot_eye.get<compages::world::Camera>().sees_helpers = false;
```

It then draws neither the helpers, nor `scene.debug()`, nor the meshes you mark yourself as helps (`RenderFlags::Helper` on their `MeshRenderer`: a marker, a gizmo made of boxes). The other views still draw them. [What the robot sees](../tutorials/07-what-the-robot-sees.md) shows the grid vanish from the robot's view.

## See also

[Several views](several-views.md) · [Move the camera](move-the-camera.md)
