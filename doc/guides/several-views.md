# Several views of one world

[Guides](README.md)

Draw the same world from several cameras in one frame: a split screen, a minimap, the camera of a robot, a picture read by your program.

![06j_SplitViews](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06j_SplitViews.png)

## Update once, render many times

```cpp
scene.update(frame);   // the world moves once
scene.render();        // the active camera (the first one made)
scene.render(top);     // another camera of the same Scene
```

`draw(frame)` is `update(frame)` plus `render()`. With several views, call the two halves: one update, then one render per view. Calling `update` twice would move the world twice.

## Where each view goes

**A part of the window**: the viewport of the camera, in fractions of the target. A camera whose viewport covers part of the target clears and draws only that part:

```cpp
m_eye.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 0.5f, 1.0f };   // left half
m_top.set(compages::world::Camera{ .projection = compages::world::Camera::Projection::Orthographic,
                                   .ortho_half_height = 80.0f,
                                   .near_plane = 1.0f,
                                   .far_plane = 300.0f,
                                   .viewport = { 0.5f, 0.0f, 0.5f, 1.0f } });   // right half, from above
```

`06j_SplitViews`: an orbiting camera on the left, an orthographic map on the right.

**A picture of its own**: a `RenderTarget`, a colour picture and a depth picture kept from frame to frame.

```cpp
COMPAGES_TRY_ASSIGN(m_eye, compages::renderer::RenderTarget::create(640u, 480u));
// each frame, or at the rate of the sensor
scene.render(robot_camera, m_eye);               // opens its pass, clears, draws
COMPAGES_TRY_ASSIGN(pixels, m_eye.readColor());  // RGBA8, the top row first
COMPAGES_TRY_ASSIGN(metres, m_eye.readDepth(robot_camera.get<compages::world::Camera>()));
```

`color()` shows it (an ImGui window); `resize(width, height)` does nothing when the size has not changed, so a panel may call it each frame. `readColor()` and `readDepth()` wait for the GPU: read at the rate of a sensor, not at every frame. `readDepth` gives metres along the axis of the camera, +infinity where nothing was drawn: what an RGB-D camera gives. `saveColor(path)` writes a PNG; `renderToFile` is this, in one call. A camera with [screen effects](screen-effects.md) gives the filtered colour to `readColor()`; the depth stays the one of the geometry.

**Another Scene on the same world**: a second `Scene` shares the World and the catalog of the first:

```cpp
compages::renderer::Scene map(world, scene.assets());
compages::world::EntityId top = map.camera().id();
scene.update(frame);
scene.render();
map.render(top);
```

## What a sensor must not see

Set `sees_helpers = false` on the camera of a sensor: it leaves out the grids, the axes, the debug lines and the meshes marked `RenderFlags::Helper` ([draw helpers](draw-helpers.md)). [What the robot sees](../tutorials/07-what-the-robot-sees.md) does all of this in the workshop.

## See also

[One frame](../concepts/one-frame.md) · [Run without a screen](run-without-a-screen.md) (a picture into a file) · [Screen effects](screen-effects.md)
