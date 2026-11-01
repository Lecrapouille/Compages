# 🏗 7. Architecture

The *why* of the layers is in [chapter 1](01-Introduction.md#-3-three-layers). This page says *where* the code lives, *who may include whom*, and *what happens* during one frame. Read it before you change Compages itself.

---

## 🧭 1. Overview

```mermaid
flowchart TB
  subgraph app [Application]
    WIN[Window and GL context]
  end

  subgraph L3 [renderer]
    SC[Scene]
    AM[AssetManager]
    EX[SceneExtractor]
    RS[RenderSnapshot]
    REN[Renderer]
  end

  subgraph L2 [world]
    W[World]
    SG[SpatialGraph]
    TS[TransformStore]
    ENT[Entity]
    ECS[EnTT components]
  end

  subgraph L1 [gpu]
    DR[Drawable]
    BUF[Buffer Pipeline Texture Compute]
  end

  subgraph BE [private]
    GL[Backends/GL45]
  end

  WIN --> SC
  SC --> W
  SC --> AM
  SC --> EX
  EX --> RS
  REN --> RS
  REN --> AM
  REN --> DR
  W --> SG
  W --> TS
  W --> ECS
  ENT --> W
  DR --> BUF
  BUF --> GL
```

`Core/` (vectors, matrices, `Result`, `Frame`, files) is the shared vocabulary, in the `compages::core` namespace. It is not a fourth layer: no simulation and no drawing starts there.

---

## 🚧 2. Who may include whom

| Layer | Entry point | May include | Never includes |
|-------|-------------|-------------|----------------|
| `compages::gpu` | `Drawable` | `Core` | `World`, `Renderer`, any `gl*` in public headers |
| `compages::world` | `World`, `Entity` | `Core` | `GPU`, `Renderer` |
| `compages::renderer` | `Scene` | `World`, `GPU`, `Core` | window code |

```cpp
#include <Compages/Compages.hpp>       // the whole stack
#include <Compages/GPU/GPU.hpp>        // layer 1
#include <Compages/World/World.hpp>    // ids, each, update
#include <Compages/World/Entity.hpp>   // chaining
#include <Compages/Renderer/Scene/Scene.hpp> // layer 3
```

The only code that includes glad is `src/GPU/Backends/GL45/`. The `GPU_BACKEND` variable of the `Makefile` chooses which backend folder is compiled; callers of `gpu::` change nothing.

---

## 📁 3. Folders

```text
include/Compages/          the public interface, installed
  Core/                    vectors, matrices, Result, Frame, AABB, units
  GPU/                     Drawable, Buffer, Compute, Texture, Pipeline...
  World/                   World, Entity, EntityId, Routine
    Components/            Camera, Light, SkinInstance, PoseDriver
    Spatial/               SpatialGraph, TransformStore, TransformSystem
    Kinematics/            Joint, JointHandle, KinematicSystem
    Import/                URDF: read the file, build the chain
    Controllers/           Orbit, Fly, FPS, input
  Renderer/
    Scene/                 Scene (the facade), Looks, Environment, RenderSettings
    Assets/                AssetManager, meshes, materials, textures, skins, prefabs
    Import/                glTF, STL, URDF visuals, prefab instantiation
    Procedural/            primitives, shader library, CanvasTexture
    Animation/             clips, Animator, AnimationSystem
    Render/                extraction, snapshot, queue, Renderer, ShadowPass, picking
    Components/            MeshRenderer, PrefabInstance
    Serialization/         saving a scene to JSON
src/                       the same folders as include/, plus GPU/Backends/GL45/ (glad is private here)
                           and the private helpers of a folder (Renderer/Import/GltfConvert.hpp,
                           Renderer/Render/Skybox.hpp); Scene keeps its drawing parts in Scene::Impl
examples/                  the gallery, one binary; Common/ holds the window and the manifest
tests/                     GPU, World, Renderer
external/external.mk       the third-party repositories, locked in external/versions.lock
```

`include/` is the contract; `src/` is not installed.

---

## 🟧 4. GPU layer

An OpenGL program is a shader with named data. `Drawable::operator[]` and `vertices<T>()` fill the CPU mirror; `draw()` sends the dirty ranges, then the draw call.

| Type | Role |
|------|------|
| `Buffer<T>` | CPU mirror, storage buffer, shared between compute and draw |
| `Pipeline`, `Program` | one mesh, several shaders |
| `Texture`, `Framebuffer`, `RenderPass` | targets, post-processing |
| `ComputeProgram`, `barrier` | compute grids |
| `UniformBlock` | uniform blocks shared by several passes |

Handles live in pools (index + generation). Destroying twice is harmless. `Buffer<T>` cannot be copied.

---

## 🟦 5. World layer

1. **Graph**: `SpatialGraph` (parent, first child, sibling) and `TransformStore` (local, world, dirty bit). Propagation from parent to child.
2. **ECS**: EnTT. `Entity` is the chaining handle; `EntityId` is what you store.

`World::update(Frame)` runs all routines; then `update()` without argument runs the joints (`KinematicSystem`) and computes the matrices (`TransformSystem`).

`Orbit` and `Fly` are routines of `World/Controllers/Controls.hpp`, reading a `ViewFrame`. `FPSController` is a separate class, used through `apply`; it draws nothing.

---

## 🟩 6. Renderer layer

| Job | Classes |
|-----|---------|
| asset catalog | `AssetManager`, glTF / STL / URDF loaders, `CanvasTexture` |
| instancing | `load`, `instantiate`, prefabs |
| drawing a frame | `SceneExtractor` → `RenderSnapshot` → `Renderer` |

```text
Scene::draw(ViewFrame)
  Scene::update
    World::update(frame)
    AnimationSystem
    World::update()
    skinning on SkinInstance
  Scene::render
    SceneExtractor
    Renderer  ->  gpu::
```

`scene.box(...)` returns a `world::Entity`: layer 2 goes on under layer 3.

---

## 🎭 7. Four actors per frame

```text
application    context, input, ViewFrame
world          entities, routines, joints, matrices
renderer       drawing, asset ids resolved
gpu            sends the CPU changes to the backend, draws and dispatches
```

- `compages::core::Frame`: time and size. Enough for a headless simulation.
- `compages::world::ViewFrame`: a `Frame` plus input.

---

## 🚪 8. Where to enter the code

| Goal | File | Demo |
|------|------|------|
| triangle | `GPU/Drawable.hpp` | `01b_Triangle` |
| compute | `GPU/Compute.hpp` | `12_ComputeParticles` |
| headless world | `World/World.hpp` | `30_HeadlessWorld` |
| joints | `World/Kinematics/Joint.hpp` | `38_RobotArm` |
| scene | `Renderer/Scene/Scene.hpp` | `50_ThreeJsLike` |
| extraction | `Renderer/Render/SceneExtractor.hpp` | renderer tests |
| assets | `Renderer/Assets/`, `Renderer/Import/` | `34_GltfModel` |
| physics engine | `World/Components/PoseDriver.hpp` | `40_ExternalPhysics` |

---

## 🪟 9. Window and physics

- GLFW exists only in `examples/Common/Window.cpp`. After the ImGui panels of the gallery, `gpu::forgetRenderState()` resets the cached state of the backend.
- Compages has no physics engine and will not get one. The world holds the poses; your engine (MuJoCo, Bullet, Jolt, your own) computes them and writes them into the entities at each step. The renderer only shows the result. See [chapter 5](05-World-Layer.md#plug-in-a-physics-engine) and `40_ExternalPhysics`.

---

## 🔨 10. Build

Install and options: [chapter 2](02-Getting-Started.md).

```text
make
make check
```

Next: [08-API-Reference.md](08-API-Reference.md).
