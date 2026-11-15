# 📷 6. What the robot sees

[◀ A look of your own](05-your-own-look.md) · [⏫ Tutorials](README.md) · [Next: The GPU alone ▶](07-the-gpu-alone.md)

![00e_RobotEye](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/00e_RobotEye.png)

A robot that works on a part must see it. You will put a camera on a mast of the arm, draw what it sees in a corner of the window, and draw it again into a small picture of its own that the program reads, pixel by pixel, as a vision program would. Look at the corner of the picture above: the robot sees the plate, the part and its own arm, but **not the grid** on the floor.

## 🎥 A second camera

A camera is an entity like the others: it has a place and can be the child of a joint. From [Workshop/Eye.cpp](../../examples/00_Workshop/Workshop/Eye.cpp):

```cpp
m_camera = p_scene.camera("Robot eye");
m_camera.parent(p_arm.base).position(-0.04f, 1.02f, 0.0f).lookAt(0.75f, 0.0f, 0.0f);
compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
lens.sees_helpers = false;
lens.near_plane = 0.02f;
lens.far_plane = 20.0f;
```

The camera is a child of the base: when the base turns, the camera turns with it, and keeps looking at the area in front of the arm. `lookAt()` takes a point in the axes of the parent: 75 cm in front of the base, at the height of the base. A small box and a cylinder (the mast) give it a body.

The `Camera` component holds the lens. `near_plane` and `far_plane` say what distances it sees, from 2 cm to 20 m. **`sees_helpers = false`** is the line that matters for a robot: the grid on the floor, the debug lines, the gizmos are drawn for you, the person at the screen; a real camera would not see them, and a vision program must not be fooled by them.

The first camera made in a Scene (the one of [step 1](02-first-scene.md)) stays the camera that `render()` uses. This one is drawn only when the program asks.

## 🖼 Two pictures of the same view

**In the window**, in a corner. A camera draws into a rectangle of the target, its **viewport**, given in fractions of the target (here: from 66 % to 98 % of the width, from 64 % to 98 % of the height):

```cpp
m_camera.get<compages::world::Camera>().viewport = { 0.66f, 0.64f, 0.32f, 0.34f };
p_scene.render(m_camera);
```

**In a picture of its own**, a `RenderTarget` of 160 × 120 pixels made once, then filled and read four times a second:

```cpp
// once
COMPAGES_TRY_ASSIGN(m_picture, compages::renderer::RenderTarget::create(160u, 120u));

// four times a second
m_camera.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 1.0f, 1.0f }; // the whole picture
p_scene.render(m_camera, m_picture);
std::vector<std::byte> pixels;
COMPAGES_TRY_ASSIGN(pixels, m_picture.readColor());
```

`render(camera, target)` draws into the target instead of the window. `readColor()` brings the picture back to the CPU: four bytes per pixel (red, green, blue, alpha), the top row first, as an image file or OpenCV expect. It **waits for the GPU** to finish drawing; that is why a sensor reads at its own rate and not at every frame.

Then the program looks at the pixels. Here, it counts the blue ones, the colour of the part:

```cpp
for (std::size_t k = 0u; k + 3u < pixels.size(); k += 4u)
{
    const int r = int(pixels[k]);
    const int g = int(pixels[k + 1u]);
    const int b = int(pixels[k + 2u]);
    blue += ((b > 100) && (b > r + 60) && (b > g + 40)) ? 1u : 0u;
}
m_seen = float(blue) / float(m_picture.width() * m_picture.height());
```

The demo writes the share over the picture ("The robot sees the part: … % of its picture"). It changes as the arm turns and as the forearm hides the part.

## 🔁 The frame, in order

```cpp
m_scene.update(p_frame);           // the arm moves
m_plate.step(…);                   // the plate heats
m_plate.show(m_scene);             // its new colours
m_eye.look(m_scene, p_frame.elapsed); // the robot draws and reads its picture (4 times a second)
m_scene.render();                  // the window, from the first camera
m_eye.show(m_scene);               // the view of the robot, in its corner
```

The World moves once; then three pictures are drawn from it. Drawing never moves anything: it only reads where things are.

## ▶ Run it

```sh
./build/linux-gcc-debug/bin/Compages-examples 00e_RobotEye
```

## 🧪 Try

- Set `sees_helpers` to `true`: the grid appears in the corner, under the plate.
- Paint the part red again (step 2): the share falls to zero. The robot looks for blue.
- Read the depth too: `m_picture.readDepth(m_camera.get<compages::world::Camera>())` gives the distance of each pixel in metres, what an RGB-D camera gives.

## 💡 What happened

A view of the world is a camera entity plus a destination: the window, a corner of it, or a picture of its own. Several cameras, several Scenes even, read the same World without changing it. RobotIK draws the cameras of its robots this way. More in the guide [several views of one world](../guides/several-views.md), and in [one frame](../concepts/one-frame.md) for what happens between `update()` and the pixels.

This is the end of the workshop. You used the three layers without seeing them: the World moved the arm, the Fields held the heat, the Renderer drew it all, and the GPU ran your stripes. The [last page](07-the-gpu-alone.md) opens the bottom layer, alone.

---

[◀ A look of your own](05-your-own-look.md) · [⏫ Tutorials](README.md) · [Next: The GPU alone ▶](07-the-gpu-alone.md)
