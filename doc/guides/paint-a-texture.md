# Paint a texture

[Guides](README.md)

Change a texture while the program runs: a robot that paints a canvas, a height-field editor, a CPU simulation shown on a quad.

![06l_CanvasPaint](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06l_CanvasPaint.png)

A `compages::renderer::CanvasTexture` is an RGBA8 picture in CPU memory, registered as a texture of the catalog, and sent to the GPU in part only. Header `<Compages/Renderer/Procedural/CanvasTexture.hpp>`, demo `06l_CanvasPaint`.

```cpp
compages::renderer::CanvasTexture canvas;
COMPAGES_TRY(canvas.create(512u, 512u));  // white

// matches the mesh: a 1 x 1 plane in local space, before scale()
canvas.setMapping(compages::AABB::fromCorners({ -0.5f, -0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }));

COMPAGES_TRY(canvas.bind(scene.assets(), "my_canvas"));
canvas.applyLook(scene, easel_entity);   // a material with this texture as base colour

// in the loop:
canvas.paintWorld(hit.point, easel.worldMatrix(), color, radius_metres);
COMPAGES_TRY(canvas.sync());             // sends the dirty part
scene.draw(frame);
```

| Step | Role |
|------|------|
| `create` | the pixels in CPU memory |
| `setMapping` | local coordinates of the mesh → coordinates of the texture (a box, plus optional U/V flips) |
| `bind` | creates or updates the texture in the catalog |
| `paintLocal`, `paintWorld`, `paintLineWorld` | a round stamp, or a line of stamps |
| `sync` | sends what changed to the GPU |
| `applyLook` | `scene.look(entity, texture_id)` |

- Pixels are stored **row by row from the bottom**, like OpenGL.
- `paintWorld` turns the world point into a local one with the inverse of the canvas's world matrix. The radius of the brush is in world units.
- Mirrored or upside down? Flip U or V in `setMapping`.
- After local stamps, `sync()` sends only the **dirty rectangle**; `clear()` marks everything. The texture uses nearest filtering, with no mipmaps to rebuild each frame. For a 4K canvas, sync less often.

The same picture can be drawn into with lines and filled pixels (`paintPixelLine`, `fillPixels`): a `Chart` draws its curves this way ([show a field](show-a-field.md#curves-over-time)).

## See also

[Pick with the mouse](pick-with-the-mouse.md) · [Make pictures and meshes](make-pictures-and-meshes.md) (a texture written once)
