# 🎨 Techniques

[⏫ Documentation](../../README.md#-documentation)

You have a working scene and want to understand a visual effect before choosing it. These pages explain the effect's algorithm through one Compages demo, then show its cost and limits. For integration steps in your application, follow the linked [guides](../guides/README.md).

Techniques are explanations focused on rendering methods. They complement the architectural [concepts](../concepts/README.md); they are not another learning sequence after the tutorials. Each page links the implementation and gives changes you can try in its demo.

## ✅ Documented techniques

- [Shadow mapping](shadow-mapping.md): light-view depth comparisons, sun shadows and the six faces of a lamp.
- [Clustered lighting](clustered-lighting.md): per-view lamp lists that avoid testing every lamp at every fragment.
- [Volumetric fog](volumetric-fog.md): light scattering and transmission in a grid that follows the camera.
- [Skybox](skybox.md): a directional background without camera translation or parallax.
- [Cel shading and outlines](cel-shading.md): quantized light and a second mesh for the silhouette.
- [Screen-space filters](screen-filters.md): depth edges and colour treatment after drawing a view.
- [Bloom](bloom.md): thresholded bright light spread through a chain of smaller targets.
- [Texture ping-pong](texture-ping-pong.md): read one generation and write another without texture feedback.
- [Flow mapping](flow-mapping.md): moving a water texture along a direction field without a fluid solver.

## 📄 Coverage still to extend

Normal mapping and vertex deformation are implemented, but do not yet have dedicated technique pages. The [shader library](../reference/shaders.md), [material guide](../guides/write-a-material.md) and [archipelago case study](../case-studies/archipelago.md) cover their current interfaces and use.

SSAO, depth of field and screen-space reflections are not implemented effects. They should not be confused with techniques available today. The complete demos remain in [case studies](../case-studies/README.md).
