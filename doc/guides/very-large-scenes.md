# Very large scenes

[Guides](README.md)

Show a metre and 384,000 km in one picture, a rocket next to the Earth and the Moon, without flicker or shaking.

![10d_EarthMoon](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10d_EarthMoon.png)

Three tools, used together by `10d_EarthMoon`.

## Logarithmic zoom

Each notch of the wheel moves the `Orbit` camera the same *part* of the way (`zoom_step`, about 11 %), and the move glides; Shift makes a notch six. Set `controller.min_distance` and `controller.max_distance` for the span of your scene.

## Reversed depth

### What the depth buffer does

Two faces can fall on the same pixel. To know which hides the other, the GPU keeps a second picture, the **depth buffer**: for each pixel, a number between 0 and 1 for the distance of the nearest face so far. A new face is kept only if it is nearer.

### Why far faces flicker

The projection does not write the distance `d` itself. With the usual convention (0 near, 1 far) it writes about `1 - near / d`, which grows fast near the camera and almost stops far away. With `near = 1 m`, the values from 0 to 0.5 are spent between 1 m and 2 m. The buffer has a fixed number of steps, so far away one step covers a long distance:

| Distance | Usual depth: two faces must be at least … apart |
|----------|-------------------------------------------------|
| 10 m | 6 µm |
| 10 km | 6 m |
| Moon (384,000 km) | 9 million km |

Closer than that, the GPU cannot choose, and the two faces flicker from one frame to the next: **z-fighting**.

### Reversed depth with floats

Store the depth the other way round: 1 at the near plane, 0 at the far one, about `near / d`. With a **floating-point** buffer (`Depth32F`), this is where the numbers are dense: a `float` has as many values between 0.001 and 0.002 as between 0.5 and 1. The two effects cancel, and the precision stays the same *in proportion* at every distance, about one ten-millionth: two faces at 10 km can be 1 mm apart, at the Moon about 40 m. A near plane of half a metre works with a far plane of two million kilometres.

The Scene does this for you; three conditions make it work:

1. A `Depth32F` target (the gallery window and `renderToFile` have one). An integer buffer has even steps: reversing it gains nothing.
2. A clip depth in [0, 1]: the backend calls `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)` for a pass with `PassDesc::reversed_depth`, so small numbers are not erased by OpenGL's `(z + 1) / 2`.
3. A reversed projection: the Renderer gives its materials `CameraFrame::reversed_projection` as `projection`.

`reversed_depth = true` also mirrors each comparison (`Less` becomes `Greater`), so your drawables keep "nearer wins", and clears the depth to "far". **You must care only** when you draw over a picture of a Scene and test against its depth (the particles of `10b_DoomLike`):

```cpp
compages::gpu::PassDesc over = compages::gpu::currentPass();
over.clear_color = false;          // keep the picture of the Scene
over.clear_depth = false;          // and its depth
over.reversed_depth = true;        // the same convention as the Scene
compages::gpu::RenderPass const pass(over);
drawable["projection"] = scene.lastCamera().reversed_projection;
```

With the usual `projection` in this pass, far faces win over near ones. `CameraFrame::projection` stays the usual one, for the CPU (culling, picking). The idea comes from "Depth Precision Visualized" by Nathan Reed.

## Floating origin

A `float` has 24 bits of precision: near the Moon, 3.8 × 10⁸ m from the origin, two neighbouring floats are 32 m apart, and the rocket would shake by 32 m. Changing the unit does not help. Keep the simulation in `double`, outside Compages, and each frame write into the World the positions **relative to the rocket**, which stay small; far things get the large numbers, where 32 m is less than a pixel.

```cpp
const compages::Vector3m origin = rocket;              // metres in double, from the simulation
m_earth.position(compages::toMetres(EARTH - origin));  // float metres for the World
m_moon.position(compages::toMetres(MOON - origin));
m_rocket.position(0.0f, 0.0f, 0.0f);
```

`compages::Vector3m` keeps the unit in the type: `Vector3g::UNIT_Y * 6371.0_km` is a position, `vector::distance` gives a `meter_t`. `compages::toMetres` turns lengths into the floats of the World; prefer it to `.to<float>()`, which gives the number in the unit of the type (`(1.0_au).to<float>()` is 1).

A sphere wears an **equirectangular** picture (a planet, a 360° sky) as a globe does: the top row on the north pole (+Y), east to the right seen from outside.

## See also

[Move the camera](move-the-camera.md) · [Text on the picture](text-on-the-picture.md) (names that hide when you come close)
