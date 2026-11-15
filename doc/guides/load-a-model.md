# 📂 Load a model

[⏫ Guides](README.md)

Put a glTF model, an STL mesh or a URDF robot into a scene, play its animations, make copies, and save the scene.

![08a_GltfModel](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/08a_GltfModel.png)

## 📥 One call

```cpp
compages::world::Entity duck;
COMPAGES_TRY_ASSIGN(duck, scene.loadFile("Duck.glb"));
const compages::Vector3f centre = scene.frameAll();   // camera and sun see everything
scene.camera().add<compages::world::Orbit>(centre);
```

`loadFile` reads **glTF / GLB**, saved **prefabs**, **STL** meshes and **URDF** robots, places the model in the world and returns its root entity. It returns a `Result`: a missing file or a broken model is an error you read, not a crash.

A `.gltf` may keep its textures in files beside it: copy them together. A `.glb` holds the model and its resources in one file. In your own program, give a path that exists from the working directory, or an absolute one; the gallery finds its assets through `examples/Common/DataPath.hpp` and `COMPAGES_DATA_PATH`, which are gallery code.

## 🗃 The catalog, and copies

| Call | Effect |
|------|--------|
| `scene.loadFile(path)` | loads and places in the world; returns the root entity |
| `scene.assets().loadFile(path)` | loads into the catalog only; returns a `PrefabId` |
| `scene.instantiate(prefab)` | one more copy in the world; joints are remapped |
| `scene.copy(entity)` | another entity with the same mesh and look (cheap duplicates) |
| `scene.frameAll()` | places camera and sun to see everything; returns the centre, for `Orbit` |

Meshes, textures and materials live once in the catalog (`AssetManager`), whatever the number of copies; an entity holds their ids.

## 🎬 Animations

```cpp
auto names = scene.clips(soldier);   // the names of its clips, in file order
scene.play(soldier, "Walk");
```

`08b_GltfAnimation` plays the clips of `Soldier.glb` with keys 1, 2, 3. Clips are applied after the routines and before the joints, at each `update()` ([one frame](../concepts/one-frame.md)).

## 🦾 A robot from URDF

A URDF describes a robot's links and joints. The World builds the chain (it runs headless: `compages::world::loadUrdf(world, path)`), and the Renderer adds the visible shapes of the links when the Scene loads the same file. `08d_RobotArm` loads an ABB IRB 2400: one entity per link, one revolute joint between each, and sliders that set the angles within their limits.

## 💾 Save a scene

`08c_PrefabAndSave` places a prefab three times, moves each copy alone, then writes the scene to `/tmp/compages_prefab_scene.json`. The JSON keeps asset **names**, not GPU ids, so it loads back on another run.

## 🧯 Common problems

- The model is black or white. A glTF material uses metallic and roughness with its base colour; check that its textures were copied beside it.
- The model is huge or tiny. glTF is in metres; a file exported in centimetres is 100 times too large. `frameAll()` shows it whole either way.
- Nothing shows. Check the `Result` of `loadFile`, then call `frameAll()`: the model may be behind the camera.

## 🔗 See also

[Write a material](write-a-material.md) for your own shading of a loaded mesh · [The World](../concepts/the-world.md#-joints) for joints · [Reference](../reference/api.md#-5-renderer)
