# 💡 Light the scene

[⏫ Guides](README.md)

Light an existing Scene with a sun and lamps, set exposure, and enable the shadows or fog it needs. Start from a prepared scene such as [the workshop](../tutorials/04-a-scene.md).

![06d_Lights](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06d_Lights.png)

## ☀ Sun, lamps, sky

```cpp
scene.sun("Sun", { 1.0f, 0.96f, 0.9f }, 1.0f).position(3.0f, 6.0f, 2.0f).lookAt(0.0f, 0.0f, 0.0f);
scene.lamp("Lamp", { 1.0f, 0.85f, 0.6f }, 2.0f, 3.0f).position(0.2f, 1.5f, 0.3f);  // colour, intensity, range
scene.ambient(0.25f, 0.27f, 0.32f);
scene.skybox({ "+x.jpg", "-x.jpg", "+y.jpg", "-y.jpg", "+z.jpg", "-z.jpg" });
```

The sun is far away: only its direction counts, the forward axis of its entity. A lamp lights round it up to its range. The ambient light stands for the sky: faces turned up get more of it than faces turned down. A sky box turns with the camera but never comes closer (`06h_Skybox`).

## 🎨 Colours and exposure

Give Scene colours as sRGB values from a colour picker. Set exposure and the tone curve through the environment:

```cpp
scene.environment().tone_mapping = compages::renderer::ToneMapping::Aces;  // the default
scene.environment().exposure = 1.5f;   // brighter, like opening the lens
```

Adjust exposure while keeping the light setup fixed, then choose a tone curve. `None` clamps values above white; `Aces` is the default. [Light and screen colour](../concepts/light-and-colour.md) explains linear light, texture encoding and presentation.

## 🌑 Shadows

The sun casts shadows by default. Change `scene.environment().shadow_resolution` to control its map size, or set `shadows = false` to disable sun shadows. Keep large receiving floors from casting by setting `RenderFlags::NoShadowCaster` on their `MeshRenderer`.

**A lamp** casts shadows when you ask:

```cpp
scene.lamp("Torch", { 1.0f, 0.55f, 0.22f }, 3.0f, 11.0f)
    .get<compages::world::PointLight>().shadows = true;
```

Set `lamp_shadow_resolution` on the environment to control each lamp face's size; zero disables lamp shadow maps. Request shadows only on lamps that need them. A bulb enclosing a lamp must have `RenderFlags::NoShadowCaster`, or it can block its own light.

Check `10a_MvpDemo` for a moving shadowed lamp. [Shadow mapping](../techniques/shadow-mapping.md) explains the depth comparisons, six lamp faces, bias and rendering cost.

## 🏮 Hundreds of lamps

Create ordinary lamps with `scene.lamp(...)`; no additional setting enables the clustered path. Keep their `range` close to the region they should light. Large overlapping ranges increase work even when the lamps are dim.

Run `06m_ManyLights` to vary count and range. [Clustered lighting](../techniques/clustered-lighting.md) describes the view-dependent lists, capacity limits and reserved shader bindings. Only the four direct lamps can use lamp shadow maps.

## 🌫 Fog that the light crosses

```cpp
compages::renderer::VolumetricFog& fog = scene.environment().volumetric;
fog.enabled = true;
fog.density = 0.035f;     // light taken per metre, at fog.height
fog.falloff = 0.25f;      // thinner higher up
fog.anisotropy = 0.6f;    // light sent mostly forward: bright toward the lights
fog.noise = 0.5f;         // clumps, drifting with fog.wind
```

Use this effect when shafts or halos in the air matter. For distance haze, leave it disabled and set `fog_density` and `fog_color` instead. Tune density first, then falloff and anisotropy. The [volumetric fog technique](../techniques/volumetric-fog.md) explains the volume, integration cost and limits.

## 🧯 Common problems

- If an untextured object is black, check its normals and the light direction before raising intensity.
- If a lamp lights nothing, check its position and range. Its visible bulb is a separate object.
- If the sky looks correct but the model stays dark, add ambient or direct light. A [skybox](../techniques/skybox.md) is a background.
- If shadows are blurred, first check the extent of the casting objects, then increase resolution.

## 🔗 See also

[Write a material](write-a-material.md) (the hook `light` changes how light falls) · [Screen effects](screen-effects.md) (bloom) · [Cel shading](../techniques/cel-shading.md)
