# 💡 Concepts: why Compages is built this way

[⏫ Documentation](../README.md)

These pages explain; they do not teach step by step (the [tutorials](../tutorials/README.md) do) and they do not list every call (the [reference](../reference/README.md) does). Each one starts from a problem, shows the idea Compages chose, and says what that choice costs and what it leaves out.

| Page | The question it answers |
|------|-------------------------|
| [Glossary](glossary.md) | What do shader, entity, mesh, normal… mean here? |
| [Three layers](three-layers.md) | Why is a simulation kept apart from its picture? Where should my data live? |
| [The World](the-world.md) | How do a scene graph and an ECS share one container? Who may move an entity? |
| [The GPU layer](the-gpu-layer.md) | Why are shaders and C++ glued by names? Who owns the vertices? |
| [One frame](one-frame.md) | What happens between `draw(frame)` and the pixels? |
| [Fields and grids](fields-and-grids.md) | How are a temperature, a height or a wind stored, computed and shown? |
| [Errors](errors.md) | Why some errors return at once and others wait for the end of the frame |
| [Architecture](architecture.md) | Folders, who includes whom, where to enter the code, what comes after OpenGL 4.5 |

Read [three layers](three-layers.md) first: the other pages lean on it.
