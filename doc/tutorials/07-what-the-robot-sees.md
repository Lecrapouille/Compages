# 📷 7. What the robot sees

[◀ A look of your own](06-a-look-of-your-own.md) · [⏫ Tutorials](README.md)

![13d_RobotEye](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/13d_RobotEye.png)

A robot that works on a part must see it. Look at the corner of the picture above: the robot sees the plate, the part and its own arm, but not the grid on the floor.

## 🎯 Goal

Attach a camera to the arm and display its view in a corner of the window. You will also render into a small target, read its pixels, and measure the share occupied by blue pixels.

## ▶ Observe the camera before reading pixels

```sh
./build/linux-gcc-debug/bin/Compages-examples 13d_RobotEye
```

Watch the upper-right picture as the base turns, then pause with **F6**. Both views stop at the same arm pose. The text reports the share of blue pixels in the sensor picture. You will follow three operations: place the camera, render its view, then read its target.

Open [Workshop/Eye.cpp](../../examples/13_Workshop/Workshop/Eye.cpp) and [13d_RobotEye.cpp](../../examples/13_Workshop/13d_RobotEye.cpp). `RobotEye` and its members are declared in [Workshop.hpp](../../examples/13_Workshop/Workshop/Workshop.hpp). The following snippets are parts of that helper, not separate programs.

## 🎥 Step 1: attach the second camera

A camera is an entity like the others. It has a place, and it can be the child of a joint. From [Workshop/Eye.cpp](../../examples/13_Workshop/Workshop/Eye.cpp):

```cpp
m_camera = p_scene.camera("Robot eye");
m_camera.parent(p_arm.base).position(-0.04f, 0.77f, 0.0f).lookAt(0.7f, 0.0f, 0.0f);
compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
lens.sees_helpers = false;
lens.near_plane = 0.02f;
lens.far_plane = 20.0f;
```

The camera is a child of the base. When the base turns, the camera turns with it and keeps looking at the area in front of the arm. `lookAt()` takes a point in the axes of the parent: 70 cm in front of the base, at the height of the base. A small box on a mast gives the camera a body.

The `Camera` component holds the lens. `near_plane` and `far_plane` say which distances it sees, here from 2 cm to 20 m. `sees_helpers = false` matters most for a robot. The grid on the floor, the debug lines and the gizmos are drawn for you, the person at the screen. A real camera would not see them, and a vision program must not be confused by them.

The first camera made in a Scene, the one of `buildRoom()`, stays the camera that `render()` uses. This one is drawn only when the program asks.

## 🖼 Step 2: render the view in the window

In the window, the camera draws into a corner. Its **viewport** is a rectangle of the target, given in fractions of the target: from 66 % to 98 % of the width, and from 64 % to 98 % of the height.

```cpp
m_camera.get<compages::world::Camera>().viewport = { 0.66f, 0.64f, 0.32f, 0.34f };
p_scene.render(m_camera);
```

The viewport values are `{ x, y, width, height }` with their origin at the lower-left of the target. Here x + width and y + height are both 0.98, so the view leaves a small border. `RobotEye::show()` sets this rectangle every time before drawing the corner view. Rendering that view does not advance time or run the arm routine again.

## 📥 Step 3: render into a readable picture

For the program, the camera also draws into a `RenderTarget` of 160 × 120 pixels. The target is made once, then filled and read four times a second:

```cpp
// once
COMPAGES_TRY_ASSIGN(m_picture, compages::renderer::RenderTarget::create(160u, 120u));

// four times a second
m_camera.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 1.0f, 1.0f }; // the whole picture
p_scene.render(m_camera, m_picture);
std::vector<std::byte> pixels;
COMPAGES_TRY_ASSIGN(pixels, m_picture.readColor());
```

`render(camera, target)` draws into the target instead of the window. `readColor()` brings the picture back to the CPU: four bytes per pixel (red, green, blue, alpha), top row first, as an image file or OpenCV expect. It waits until the GPU has finished drawing. This is why a sensor reads at its own rate and not at every frame.

The target holds 160 × 120 × 4 = 76,800 bytes of colour data. The viewport must cover the whole target here; otherwise only the corner rectangle would be rendered. Later, `show()` restores the corner viewport before drawing into the window. Keep both assignments when changing the viewport.

`RobotEye::look()` subtracts elapsed time from a countdown and resets it to `0.25_s` after a read. This limits readback to roughly four times per second when frames arrive often enough. It returns a `compages::Status`; setup and frame code check failures from target creation and readback.

## 🔵 Step 4: measure a colour

Then the program looks at the pixels. Here it counts the blue ones, the colour of the part:

```cpp
std::size_t blue = 0u;
for (std::size_t k = 0u; k + 3u < pixels.size(); k += 4u)
{
    const int r = int(pixels[k]);
    const int g = int(pixels[k + 1u]);
    const int b = int(pixels[k + 2u]);
    blue += ((b > 100) && (b > r + 60) && (b > g + 40)) ? 1u : 0u;
}
m_seen = float(blue) / float(m_picture.width() * m_picture.height());
```

The demo writes the share over the picture: "The robot sees the part: … % of its picture". The number changes as the arm turns, and when the forearm hides the part.

A pixel counts only if blue is above 100 and sufficiently above both red and green. Alpha is skipped. The division converts the count into a fraction between 0 and 1. For example, 192 blue pixels out of 19,200 give `m_seen = 0.01`, displayed as 1%.

This measures a screen area, not the part's distance or physical size. Lighting, shadows, and other blue objects can change the count. It is a colour-threshold exercise, not reliable object recognition.

## 🔁 The frame, in order

```cpp
m_scene.update(p_frame);
m_plate.step(std::min(p_frame.elapsed, 0.05_s), m_arm.tool.worldPosition());
if (const compages::Status shown = m_plate.show(m_scene); !shown)
{
    compages::gpu::reportError(shown.error());
}
if (const compages::Status seen = m_eye.look(m_scene, p_frame.elapsed); !seen)
{
    compages::gpu::reportError(seen.error());
}
m_scene.render();
m_eye.show(m_scene);
```

Setup calls `m_eye.create(m_scene, m_arm)` after building the arm and plate, before `prepare()`. Use the complete setup in `13d_RobotEye.cpp` when following the call order above. The constants and `0.05_s` use the SI literals from the earlier chapters.

The World moves once. Two window views are drawn each frame. On sensor frames, a third picture is drawn into the offscreen target. Drawing never moves anything: it only reads where things are.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 13d_RobotEye
```

## 🧪 Try

Edit `Eye.cpp` or `Room.cpp`, then rebuild with `make` and rerun `13d_RobotEye`.

- In `RobotEye::look()`, change `m_wait = 0.25_s` to `m_wait = 1.0_s`.
  The percentage changes less often while the corner view still moves at the frame rate.

- Set `sees_helpers` to `true`. The grid appears in the corner.
- Paint the part red in `buildRoom()`. The share falls to zero, because the robot looks for blue.
- Read the depth too: `m_picture.readDepth(m_camera.get<compages::world::Camera>())` gives depth along the camera axis in metres. Background pixels return positive infinity; ignore them when processing depth.

A view of a world is a camera plus a place to draw: the window, a corner of it, or a picture of its own. Several cameras, and even several Scenes, can read the same World without changing it. RobotIK draws the cameras of its robots this way. The guide [several views](../guides/several-views.md) has the other cases, and [one frame](../concepts/one-frame.md) explains what happens between `update()` and the pixels.

This is the end of the tutorials. The [guides](../guides/README.md) answer one task each, the [techniques](../techniques/README.md) explain one effect each, and the [case studies](../case-studies/README.md) follow a whole demo from the problem to the picture.

---

[◀ A look of your own](06-a-look-of-your-own.md) · [⏫ Tutorials](README.md)
