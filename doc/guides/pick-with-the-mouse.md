# Pick with the mouse

[Guides](README.md)

Know which shape is under the mouse and where the ray hits it; or where the mouse points on a plane.

![06i_CameraPick](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06i_CameraPick.png)

## A shape under a pixel

`scene.pick(pixel)` gives the nearest shape under a pixel and the world point it hits, as the **last render** saw it:

```cpp
if (p_frame.input.mouse_left)
{
    if (auto hit = m_scene.pick(p_frame.input.mouse))
    {
        if (hit->entity == m_canvas.id())
        {
            // hit->point is in world space
        }
    }
}
```

- The pixel starts at the **bottom left** (`Input::mouse`), like OpenGL.
- It uses the camera of the last render: before the first frame, there is nothing to pick.
- It uses the box round each mesh (`local_bounds`) to skip what the ray misses: wrong bounds make a shape impossible to pick.
- In the gallery the right button turns the camera (`Orbit`), so the left button is free for the demo.

`06i_CameraPick` shows the box of what you picked; `06l_CanvasPaint` paints where you click.

## A point on a plane

When you want a plane rather than a shape (the water of `10f`, the ground of a map), make the ray yourself from the camera of the last render, two points of the pixel unprojected:

```cpp
const compages::Vector4f eye = camera.inverse_projection * compages::Vector4f(x, y, p_depth, 1.0f);
const compages::Vector4f world = camera.inverse_view * compages::Vector4f(eye.x / eye.w, eye.y / eye.w, eye.z / eye.w, 1.0f);
```

`scene.lastCamera()` holds these matrices; it is empty before the first render (check `viewport_width`). For a field, `scene.probe()` does all of this and returns the value too ([show a field](show-a-field.md#the-value-under-the-mouse)).

## See also

[Paint a texture](paint-a-texture.md) · [Move the camera](move-the-camera.md)
