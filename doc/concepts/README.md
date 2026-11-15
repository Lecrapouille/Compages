# 💡 Concepts

[⏫ Documentation](../../README.md#-documentation)

These pages explain why Compages is built the way it is. Each one starts from a problem, describes the choice made, and says what this choice costs. Read [three layers](three-layers.md) first.

- [Three layers](three-layers.md): why a simulation is kept apart from its picture, and where to put your data.
- [The World](the-world.md): a scene graph and an ECS in one container; who may move an entity.
- [The GPU layer](the-gpu-layer.md): why shaders and C++ meet by names; who owns the vertices.
- [One frame](one-frame.md): what happens between `draw(frame)` and the pixels.
- [Fields and grids](fields-and-grids.md): how a temperature or a wind is stored, computed and shown.
- [Errors](errors.md): why some errors come back at once and others at the end of the frame.
- [Architecture](architecture.md): the folders, who includes whom, where to start reading the code.
- [Glossary](../tutorials/00-glossary.md): the 3D words used in this documentation.
