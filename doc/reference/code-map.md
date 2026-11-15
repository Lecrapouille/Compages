# 🗂 Code map

[⏫ Reference](README.md)

Public headers, implementation folders and source entry points for contributors. The dependency rules and their reasons are in [architecture](../concepts/architecture.md).

## 📦 Public headers

```cpp
#include <Compages/Compages.hpp>              // the whole stack
#include <Compages/GPU/GPU.hpp>               // the GPU layer
#include <Compages/Fields/Field.hpp>          // Grid2D, Field<T>
#include <Compages/World/World.hpp>           // ids, each, update
#include <Compages/World/Entity.hpp>          // chaining
#include <Compages/Renderer/Scene/Scene.hpp>  // the Renderer
```

## 📁 Folders

```text
include/Compages/          the public interface, installed
  Core/                    vectors, matrices, Result, Frame, AABB, units
  Fields/                  Grid2D, CurvilinearGrid, Grid3D, Field<T, Grid>, intersect, isosurface, streamline
  GPU/                     Drawable, Buffer, Compute, Texture, Pipeline, ShaderSource, Statistics
  World/                   World, Entity, EntityId, Routine
    Components/            Camera, Light, SkinInstance, PoseDriver
    Spatial/               SpatialGraph, TransformStore, TransformSystem
    Kinematics/            Joint, JointHandle, KinematicSystem
    Import/                URDF: read the file, build the chain
    Controllers/           Orbit, Fly, FPS, input
  Renderer/
    Scene/                 Scene (the façade), Looks, Environment, RenderSettings
    Assets/                AssetManager, meshes, materials, textures, skins, prefabs
    Import/                glTF, STL, URDF visuals, prefab instantiation
    Procedural/            primitives, shader library (pieces and hooks), CanvasTexture
    Visualization/         FieldLook, ColorMap, Chart: fields drawn by Scene::show()
    Animation/             clips, Animator, AnimationSystem
    Render/                extraction, snapshot, Renderer, shadows, fog, clusters, RenderTarget, picking
    Components/            MeshRenderer, PrefabInstance, ScreenEffects
    Serialization/         saving a scene to JSON
src/                       the same folders, plus GPU/Backends/GL45/ and the private helpers of a folder
examples/                  the gallery, one binary; Common/ holds the window and the manifest
tests/                     Core, Fields, GPU, World, Renderer (GoogleTest)
external/external.mk       the third-party repositories, locked in external/versions.lock
```

`include/Compages/` is installed; `src/` is not.

## 🚪 Where to enter the code

| Goal | File | Demo |
|------|------|------|
| triangle | `GPU/Drawable.hpp` | `01c_Triangle` |
| compute | `GPU/ComputeProgram.hpp` | `05d_ComputeParticles` |
| ping-pong | `GPU/PingPong.hpp` | `05f_Galaxy`, `05b_GameOfLife` |
| headless world | `World/World.hpp` | `07a_HeadlessWorld` |
| joints | `World/Kinematics/Joint.hpp` | `13a_Workshop`, `08d_RobotArm` |
| scene | `Renderer/Scene/Scene.hpp` | `13a_Workshop` |
| shader hooks | `Renderer/Procedural/ShaderLib.hpp` | `13c_PaintedArm`, `06o_CelShading` |
| extraction | `Renderer/Render/SceneExtractor.hpp` | the renderer tests |
| assets | `Renderer/Assets/`, `Renderer/Import/` | `08a_GltfModel` |
| physics engine | `World/Components/PoseDriver.hpp` | `07e_ExternalPhysics` |

The conventions for contributors (style, naming, commits, the rules easy to break) are in [AGENTS.md](../../AGENTS.md).
