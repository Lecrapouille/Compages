# See-through things

[Guides](README.md)

Draw glass, water, a ghost: shapes you can see through.

![06e_TexturedSpheres](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06e_TexturedSpheres.png)

## With a look

```cpp
scene.box("Glass", compages::renderer::seeThrough(compages::renderer::color(0.2f, 0.7f, 0.9f), 0.35f))
    .position(0.0f, 0.0f, 1.0f);
```

Opacity 1 is solid, 0 invisible. `06e_TexturedSpheres` puts a glass ball in front of a colour, a photo and a tinted photo.

## How it is drawn

A see-through surface blends its colour with what is behind it, so what is behind must be drawn first. The Renderer draws all the solid shapes, then the see-through ones **from the farthest to the nearest**, blended over the picture. They do not write the depth, so a second glass behind the first still shows.

The sorting is per object, by the centre of its box: two see-through objects that cross each other can still be drawn in the wrong order. Cut them into smaller pieces when it shows.

## With a material of your own

A material gives a second state, used when the opacity of a shape is under one:

```cpp
.state = { .depth_test = true, .cull = compages::gpu::CullMode::None },
.see_through = compages::gpu::RenderState{ .depth_test = true,
                                           .depth_write = false,
                                           .blend = compages::gpu::Blend::alpha(),
                                           .cull = compages::gpu::CullMode::None } }
// ...
renderer.flags = compages::renderer::RenderFlags::NoShadowCaster;
instance->opacity = 0.99f;
```

This is the jet of water of `10f_FlowingWater`. `CullMode::None` draws both faces, so the far side of the jet shows through the near one. Water casts no shadow (`NoShadowCaster`). The shader writes its alpha into `oColor.a`, times `opacity`.

## See also

[Write a material](write-a-material.md) · [One frame](../concepts/one-frame.md) (the order of the draws)
