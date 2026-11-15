# 🌍 Very large scenes

[⏫ Guides](README.md)

Show a rocket next to the Earth and Moon without losing its small movements or distant surface separation.

![10d_EarthMoon](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10d_EarthMoon.png)

Use [10d_EarthMoon.cpp](../../examples/10_Complete/10d_EarthMoon.cpp) as the working example. [Precision and scale](../concepts/precision-and-scale.md) explains why world coordinates and depth need separate solutions.

## 📍 Keep the simulation precise

Keep absolute positions in double precision outside the World. Choose an origin near the object being followed, subtract it, then convert the small offsets to float metres before `scene.update()`:

```cpp
m_origin = m_floating_origin ? focus(m_focus).centre : EARTH;
m_earth.position(fromOrigin(EARTH));
m_moon.position(fromOrigin(MOON));
// ...place the other bodies with the same origin.
m_rocket.position(fromOrigin(m_position));
```

These lines come from `EarthMoon::place()`. `focus(m_focus).centre` is the followed body's simulation position. The demo's `fromOrigin()` helper subtracts `m_origin` from a `compages::Vector3m`, then calls `compages::toMetres` to obtain float metres for the World. Use the same origin for every object in that frame. Add it back when converting picked World points to simulation positions.

## 🔍 Allow the camera to cover the scene

Set the `Orbit` controller's `min_distance` and `max_distance` for the required span. Wheel zoom changes distance by a proportion and glides toward the result. Use [move the camera](move-the-camera.md) to configure the controls and input.

Choose the camera's near and far planes to include the required objects. Keep the near plane as far from zero as your closest view permits.

## 🎯 Reversed depth

Use a floating-point `Depth32F` drawing target. The gallery and `Scene::renderToFile()` already provide one. For your own targets, check their depth attachment format. The Renderer uses reversed depth for the Scene.

When drawing a GPU overlay that reads the Scene's depth, preserve both buffers and select its reversed projection:

```cpp
compages::gpu::PassDesc over = compages::gpu::currentPass();
over.clear_color = false;          // keep the picture of the Scene
over.clear_depth = false;          // and its depth
over.reversed_depth = true;        // the same convention as the Scene
compages::gpu::RenderPass const pass(over);
drawable["projection"] = scene.lastCamera().reversed_projection;
```

This excerpt assumes `scene` was rendered into the current target and `drawable` is your overlay. Draw it before ending that target's pass. The CPU's `scene.lastCamera().projection` uses the usual convention; it is not the overlay projection for this pass.

## 🧯 Common problems

- If the rocket shakes, check that you subtract the origin before converting to float.
- If distant faces flicker, check the depth attachment and the near plane.
- If a GPU overlay appears through foreground objects, check its projection and `reversed_depth` together.
- If picking gives an unexpected absolute location, add the current drawing origin back to the returned World point.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 10d_EarthMoon
```

Follow the rocket, then change the focus to Earth or Moon. Small objects and planetary distances share the same scene.

## 🔗 See also

[Precision and scale](../concepts/precision-and-scale.md) · [Move the camera](move-the-camera.md) · [Several views](several-views.md)
