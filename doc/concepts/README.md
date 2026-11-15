# 💡 Concepts

[⏫ Documentation](../../README.md#-documentation)

These pages explain design decisions and the models behind Compages. Read [three layers](three-layers.md) first. For a particular rendering algorithm, use [techniques](../techniques/README.md). For steps to apply a choice in your program, use [guides](../guides/README.md).

- [Three layers](three-layers.md): why a simulation is kept apart from its picture, and where to put your data.
- [The World](the-world.md): a scene graph and an ECS in one container; who may move an entity.
- [The GPU layer](the-gpu-layer.md): why shaders and C++ meet by names; who owns the vertices.
- [One frame](one-frame.md): what happens between `draw(frame)` and the pixels.
- [Fields and grids](fields-and-grids.md): how a temperature or a wind is stored, computed and shown.
- [Errors](errors.md): why some errors come back at once and others at the end of the frame.
- [Light and screen colour](light-and-colour.md): why lighting uses linear values, and where exposure and tone mapping apply.
- [Precision and scale](precision-and-scale.md): why large positions and distant depth lose detail, and how their remedies differ.
- [Architecture](architecture.md): why public include boundaries and the private backend keep the layers independent.
- [Glossary](../tutorials/00-glossary.md): the 3D words used in this documentation.
