# ✨ Screen effects

[⏫ Guides](README.md)

**Goal:** pass the picture of one view through filters of your own (sepia, outlines from the depth, a vignette) and a bloom, without managing framebuffers.

![12a_ScreenFilters](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/12a_ScreenFilters.png)

## 🎛 A component on the camera

Put a `ScreenEffects` component on a camera, and its view goes through the effects before it reaches the screen:

```cpp
camera.set(compages::renderer::ScreenEffects{
    .filters = { { .name = "Sepia", .glsl = SEPIA, .values = { { "amount", 0.8f } } } },
    .bloom = { .enabled = true, .threshold = 1.0f, .strength = 0.6f } });
```

The Scene then draws this view in linear light into a picture of its own (RGBA16F, where a bulb can be brighter than white), runs the filters one after the other, then the bloom, and a last pass gives the tone curve and the screen colours. A camera without the component is drawn as before: each view chooses. In `12a_ScreenFilters`, only the right half has filters.

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

`effect(uv)` returns the light of the pixel at `uv` (0 to 1 across the view). It reads:

| Name | Holds |
|------|-------|
| `screenColor` | the light of the view, linear, or what the filter before made |
| `screenDepth` | the depth, reversed: 1 near, 0 far and for the background |
| `screenSize` | the size of the view in pixels |
| its own `uniform float` | set by `values`; a value whose uniform is not in the text is an error |

The settings may change at any frame: edit the component in place, `camera.get<ScreenEffects>().bloom.strength = 1.2f`.

## 💡 Bloom

The bloom keeps the light above `threshold` (1 is the white of the screen), makes it half as large up to six times, then enlarges and sums it back: a halo of every size, added with `strength` before the tone curve. An unlit bulb of colour 1 is several times the white of the screen in linear light, so it blooms. [The bloom technique page](../techniques/bloom.md) shows the passes and their cost.

## 🎨 Colours that must not change

Colours given for the screen keep their colour with the effects on: the background, unlit looks, the sky, the debug lines go through `screen()`, the inverse of the tone curve. A material of your own that writes a screen colour itself (not through `present()`) should do the same: `oColor = vec4(screen(toSrgb(c)), opacity)`. The text of labels is drawn after the last pass, as it is.

## 🔗 See also

[Bloom](../techniques/bloom.md) · [Cel shading](../techniques/cel-shading.md) (outlines in the scene) · [Draw in several passes](draw-in-several-passes.md) (your own passes, by hand)
