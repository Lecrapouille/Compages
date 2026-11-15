# 📖 Glossary

[⏫ Tutorials](README.md) · [Next: install and run ▶](01-install.md)

3D programming has its own words, and the tutorials use them from the first page. Read this page once before you start: ten minutes are enough. You do not need to remember everything. When a tutorial uses a word you do not know, come back here.

## 📐 Places, directions and time

A point in space has three coordinates, **(x, y, z)**. In Compages, as in most 3D tools, **y goes up**; x goes sideways and z toward you along the floor. The point (0, 0, 0) is the **origin**. Distances are in metres: keep the same unit in your own data.

| Word | Meaning |
|------|---------|
| **Position** | Where something is. `(2, 0, 0)` is two metres along x from the origin. |
| **Vector** | A group of numbers. Three numbers can be a position, a direction or a speed: the code says which. |
| **Rotation** | A turn round an axis. Angles are in radians (a full turn is about 6.283) unless written `90.0_deg`. |
| **Scale** | A change of size, maybe different along each axis. |
| **Local / world** | A local position is measured from the parent; a world position includes every parent. A wheel at x = 1 on a car at x = 2 is at world x = 3. |
| **Frame** | One step of the program. `frame.elapsed` is the time since the previous step, `frame.total` the time since the start. A speed of 2 m/s moves a thing by `2 × elapsed` metres at each frame, so the speed does not depend on the frame rate. |

## 📏 Units

A number alone does not say if it is seconds, metres or radians. In Compages, a time, an angle or a speed carries its unit in its type, from the `compages::si` namespace (SI is the international system of units). If you give metres where the code wants an angle, the program does not compile.

| Type | Unit | Written as |
|------|------|------------|
| `compages::si::Seconds` | second | `0.5_s` |
| `compages::si::Radians` | radian (a full turn is about 6.283) | `1.2_rad`, or `90.0_deg` (converted) |
| `compages::si::AngularVelocity` | radian per second | `1.2_rad_per_s` |
| `compages::si::LinearVelocity` | metre per second | `2.0_mps` |
| `compages::si::Length` | metre | `0.5_m`, `12.0_cm` |

The suffixes need one line at the top of your file: `using namespace compages::si::literals;`. The units also do the arithmetic: a speed of `1.2_rad_per_s` times `frame.elapsed` is an angle. For a sine or a cosine, use the functions of the units library: `units::math::sin(2.0_rad_per_s * t)` turns at 2 rad/s, and `units::math::abs`, `exp`, `atan2` and `hypot` work the same way. When code of your own takes plain `float` numbers, `compages::toSeconds(t)`, `compages::toRadians(a)` and `compages::toMetres(d)` give them. Positions and sizes stay plain `float` numbers, in metres.

## 🟧 On the GPU side

| Word | Meaning |
|------|---------|
| **GPU** | The graphics card: thousands of small cores that run the same small program on many data at once. |
| **OpenGL** | The interface a program uses to talk to the GPU. Compages uses OpenGL 4.5. |
| **Shader** | A small program, in the **GLSL** language, that runs on the GPU. A **vertex shader** places each corner; a **fragment shader** colours each pixel. |
| **Vertex** | A corner of a shape, with its data: position, colour, normal… A triangle has three. |
| **Attribute** | A value given per vertex (`in vec2 position;` in the shader). |
| **Uniform** | A value given once for a whole draw: a colour, a time, a matrix. |
| **Texture** | A picture the shader reads, through a **sampler**. |
| **Buffer** | A block of GPU memory: vertices, indices, any array. |
| **Storage buffer** | A buffer a shader may write as well as read. |
| **Draw call** | One order "draw these triangles with this shader". Fewer is faster. |
| **Compute shader** | A shader that draws nothing: it computes, once per item of a grid. |
| **Barrier** | The order "the next reader waits until these writes are done". The GPU does not wait by itself. |
| **CPU mirror** | The copy of a buffer kept in normal memory. You change it; Compages sends the changed part at the next draw. |
| **Render pass** | A drawing session: it chooses the destination picture and its size. |
| **Framebuffer** | A destination of drawing other than the window: one or more textures. |
| **Graphics context** | The state of the driver, made by the program before any OpenGL call. It belongs to a window or to an offscreen program. |

## 🟦 On the World side

| Word | Meaning |
|------|---------|
| **Entity** | One thing of the world: a robot, a wheel, a camera, a light. Only an id and a name. |
| **Component** | A piece of data attached to an entity: a `Velocity`, a `Health`. Your own C++ structs. |
| **ECS** | *Entity Component System*: entities hold components, and **systems** (plain loops) visit every entity that has some components. The data stays packed in memory, so the loops are fast. |
| **Scene graph** | The tree of parents and children. A wheel is a child of the car: when the car moves, the wheel follows. |
| **Transform** | The position, rotation and scale of an entity, relative to its parent. |
| **World matrix** | Where an entity finally is in the world, computed from all its parents. |
| **Joint** | A child that moves one way only: a **revolute** joint turns (an elbow), a **prismatic** joint slides. |
| **Forward kinematics** | Computing where the end of a chain of joints is, from the angles of its joints. |
| **Routine** | A small piece of code attached to one entity, called at each step (`update`, `fixedUpdate`…). |
| **Headless** | Running without a window. The World needs no graphics context at all; drawing without a screen still needs one. |

## 🟩 On the Renderer side

| Word | Meaning |
|------|---------|
| **Scene** | Everything needed to draw a world: cameras, lights, meshes, materials. |
| **Mesh** | The triangles of a shape. |
| **Material** | How a surface looks: a shader, a colour, pictures, how it reacts to light. |
| **Look** | The simple form of a material in a `Scene`: a colour, a picture, unlit, see-through. |
| **Hook** | A named function of a Compages shader that you may replace (`surface`, `light`, `deform`, `main`). |
| **Camera** | The point of view. Its view comes from the world matrix of its entity. |
| **Viewport** | The rectangle of the destination a camera draws into. |
| **Asset** | A resource loaded once and shared: a mesh, a texture, a glTF model, a URDF robot. |
| **glTF, URDF, STL** | File formats: 3D models (glTF), robots with joints (URDF), raw meshes (STL). |
| **Normal** | The direction a surface faces. The light uses it to know how much reaches the surface. |
| **UV coordinates** | Where a vertex lies on a texture, usually 0 to 1 across the picture. |
| **PBR** | *Physically Based Rendering*: a surface described by its colour, how metallic and how rough it is, lit in a way close to physics. |
| **Depth** | The distance of each pixel, used to keep the nearer surface in front. |
| **Snapshot** | A copy of what must be drawn (cameras, lights, placements, asset ids), taken from the world before drawing. |
| **Helper** | A line or a shape drawn for the person at the screen (a grid, axes, a marker), not part of the world. |
| **Tone curve** | The last step of a picture: light, which may be brighter than white, becomes colours the screen can show. |

## 🗺 On the Fields side

| Word | Meaning |
|------|---------|
| **Grid** | Where the points are: a regular grid (`Grid2D`, `Grid3D`) or a grid fitted round a shape (`CurvilinearGrid`). |
| **Field** | One value per point of a grid: a temperature, a height, a wind. |
| **Colour map** | The rule that turns a value into a colour (Viridis, Inferno, Coolwarm…). |
