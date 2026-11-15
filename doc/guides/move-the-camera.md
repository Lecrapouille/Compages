# 🎥 Move the camera

[⏫ Guides](README.md)

**Goal:** let the user turn round a point, fly through a scene, or drag a map; or move the camera from your code.

![10e_Archipelago](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/10e_Archipelago.png)

A camera is an entity: `position()`, `lookAt()`, `parent()` move it like anything else. The ready-made controls are **routines** that read the mouse and the keys from the `ViewFrame`. Place the camera first: a control starts from where it is.

| Control | For | Input |
|---------|-----|-------|
| `Orbit` | turning round an object | right button turns, wheel zooms (logarithmic, glides), Shift faster |
| `Fly` | moving through a scene | right button looks, W A S D, Q E |
| `Rig` | a map, a strategy game | left button drags the ground, right or middle turns, wheel zooms toward the mouse; W A S D or arrows, Q E turn, R F zoom, Shift runs |
| `FPSController` | walking on the ground | applied directly, not a routine |

```cpp
scene.camera().position(1.3f, 1.55f, 1.55f).add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.85f, 0.0f));
scene.camera().position(1.6f, 1.2f, 1.6f).add<compages::world::Rig>(compages::Vector3f(0.35f, 0.0f, 0.0f));
```

The point given to `Orbit` and `Rig` is what they look at. In your own window, fill `frame.input` from your window library; the gallery does it in `examples/Common/Window.cpp`.

## 🛫 Fly to a place

`Rig` glides to the wanted pose: each frame covers the part `1 - exp(-sharpness × dt)` of the way, so the glide is the same at any frame rate and never overshoots. To jump far, call `flyTo`: the camera goes up, across and down along the path of van Wijk and Nuij (the path of a map viewer), starting and stopping with no jolt; the farther the place, the higher it goes. Any input of the user stops the flight where it is.

```cpp
compages::world::RigController& rig = camera.get<compages::world::Rig>().controller;
rig.flight_pace = 0.8f;                                            // seconds per unit of path
rig.flyTo(compages::Vector3f(4200.0f, 0.0f, -3100.0f), 3200.0f);   // target, distance (m)
```

`10e_Archipelago` flies between regions this way. The logic is in `RigController`, tested without a window.

## 🔗 See also

[Several views](several-views.md) (more than one camera) · [Very large scenes](very-large-scenes.md) (zooming from a metre to the Moon) · [Pick with the mouse](pick-with-the-mouse.md)
