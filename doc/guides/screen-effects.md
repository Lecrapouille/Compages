# ✨ Screen effects

[⏫ Guides](README.md)

Pass the picture of one view through filters of your own (sepia, outlines from the depth, a vignette) and a bloom, without managing framebuffers.

![12a_ScreenFilters](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/12a_ScreenFilters.png)

## 🎛 A component on the camera

Put a `ScreenEffects` component on a camera, and its view goes through the effects before it reaches the screen:

```cpp
camera.set(compages::renderer::ScreenEffects{
    .filters = { { .name = "Sepia", .glsl = SEPIA, .values = { { "amount", 0.8f } } } },
    .bloom = { .enabled = true, .threshold = 1.0f, .strength = 0.6f } });
```

Attach the component before the camera is rendered. A camera without it keeps its original output; effects are selected per view. In `12a_ScreenFilters`, only the right half has them. The processing order and its cost are explained in [screen-space filters](../techniques/screen-filters.md).

## 🖌 A filter is one function of GLSL

```glsl
uniform float amount;

vec3 effect(vec2 uv)
{
    vec3 c = texture(screenColor, uv).rgb;
    float y = dot(c, vec3(0.299, 0.587, 0.114));
    return mix(c, y * vec3(1.2, 1.0, 0.75), amount);
}
```

`effect(uv)` returns linear colour at the view's texture coordinates. The [filter interface](../reference/shaders.md#-screen-filters) lists its built-in inputs. Match each entry in `values` with a `uniform float` declared by that filter.

The settings may change at any frame: edit the component in place, `camera.get<ScreenEffects>().bloom.strength = 1.2f`.

## 💡 Bloom

Set `.bloom.enabled = true` in the component. Lower `threshold` if no halo is visible, then tune `strength`. Keep enough light above the threshold in the scene. [Bloom](../techniques/bloom.md) explains the bright-pass and image pyramid; [light and screen colour](../concepts/light-and-colour.md) explains why the effect needs unclipped linear values.

## 🎨 Colours that must not change

Colours given for the screen keep their colour with the effects on: the background, unlit looks, the sky, the debug lines go through `screen()`, the inverse of the tone curve. A material of your own that writes a screen colour itself (not through `present()`) should do the same: `oColor = vec4(screen(toSrgb(c)), opacity)`. The text of labels is drawn after the last pass, as it is.

## 🧯 Common problems

- Put `ScreenEffects` on the camera being rendered, not on the object being filtered.
- Check filter compilation errors and uniform names before adding another filter.
- Keep filters in their intended order; later filters process earlier colour results.
- For geometrical silhouettes rather than image-depth edges, use [cel outlines](../techniques/cel-shading.md).

## 🔗 See also

[◀ Bloom](../techniques/bloom.md) · [Cel shading](../techniques/cel-shading.md) (outlines in the scene) · [Draw in several passes](draw-in-several-passes.md) (your own passes, by hand)
