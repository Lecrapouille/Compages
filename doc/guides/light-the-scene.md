# Light the scene

[Guides](README.md)

Choose the sky, the sun, the lamps and their shadows, the exposure, hundreds of lamps, and fog that the light crosses.

![06d_Lights](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06d_Lights.png)

## Sun, lamps, sky

```cpp
scene.sun("Sun", { 1.0f, 0.96f, 0.9f }, 1.0f).position(3.0f, 6.0f, 2.0f).lookAt(0.0f, 0.0f, 0.0f);
scene.lamp("Lamp", { 1.0f, 0.85f, 0.6f }, 2.0f, 3.0f).position(0.2f, 1.5f, 0.3f);  // colour, intensity, range
scene.ambient(0.25f, 0.27f, 0.32f);
scene.skybox({ "+x.jpg", "-x.jpg", "+y.jpg", "-y.jpg", "+z.jpg", "-z.jpg" });
```

The sun is far away: only its direction counts, the forward axis of its entity. A lamp lights round it up to its range. The ambient light stands for the sky: faces turned up get more of it than faces turned down. A sky box turns with the camera but never comes closer (`06h_Skybox`).

## Colours and exposure

Every colour you give a Scene (looks, lights, ambient, fog) is **sRGB**, the colour of a colour picker. The Renderer turns it into **linear** light, where two lamps give twice the light, and computes in linear. At the end a **tone curve** turns the light, which has no upper limit, into a screen colour between 0 and 1.

```cpp
scene.environment().tone_mapping = compages::renderer::ToneMapping::Aces;  // the default
scene.environment().exposure = 1.5f;   // brighter, like opening the lens
```

`Aces` (the film curve) keeps contrast and keeps bright colours coloured; `Reinhard` is softer; `None` clamps, so what is too bright turns flat white. `06d_Lights` has both settings in its panel. Pictures given with `texture()` are read as sRGB; glTF colour factors are already linear, as the format says. The background is written as it is.

## Shadows

**The sun** casts shadows by default. Before drawing, the Renderer draws the depth of the scene seen from the sun into a picture (`Environment::shadow_resolution`, 2048), fitted to the objects that cast; the materials read it with a 3 × 3 filter, so edges are soft. A floor should not cast (`RenderFlags::NoShadowCaster` on its `MeshRenderer`): it would stretch the fitted box and blur every shadow. `Environment::shadows = false` turns them off.

**A lamp** casts shadows when you ask:

```cpp
scene.lamp("Torch", { 1.0f, 0.55f, 0.22f }, 3.0f, 11.0f)
    .get<compages::world::PointLight>().shadows = true;
```

A lamp shines in every direction, so the Renderer draws the distance to it six times, as six cameras of 90° (the faces of a cube). The faces of the four lamps nearest the camera sit in one atlas of `Environment::lamp_shadow_resolution` texels per face (512; zero turns them off). That is six more draws of the nearby scene per lamp: turn it on for the lamps that matter. Something that holds or surrounds a lamp (a bracket, a flame) must not cast, or it hides the lamp from everything. `10a_MvpDemo` and `10b_DoomLike` use them.

## Hundreds of lamps

A scene may hold up to `LightClusters::MAX_LIGHTS` (256) lamps. The four nearest the camera get uniforms of their own and their shadows; the others are sorted into **clusters**: the picture is cut into 16 × 9 tiles and the depth into 24 slices that thicken with distance, and each lamp goes into every cluster its sphere of light may touch. A pixel reads only the lamps of its cluster, so the cost follows how many lamps reach a pixel, not how many the scene holds. These lamps cast no shadow. The clusters are three storage buffers at bindings 9, 10 and 11 of the lit shaders: keep these free in your own hooks.

![06m_ManyLights](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06m_ManyLights.png)

`06m_ManyLights` flies two hundred lamps over a floor. The method is O. Olsson, M. Billeter and U. Assarsson, "Clustered Deferred and Forward Shading" (HPG 2012).

## Fog that the light crosses

```cpp
compages::renderer::VolumetricFog& fog = scene.environment().volumetric;
fog.enabled = true;
fog.density = 0.035f;     // light taken per metre, at fog.height
fog.falloff = 0.25f;      // thinner higher up
fog.anisotropy = 0.6f;    // light sent mostly forward: bright toward the lights
fog.noise = 0.5f;         // clumps, drifting with fog.wind
```

![06n_VolumetricFog](https://raw.githubusercontent.com/Lecrapouille/Compages-data/master/doc/examples/06n_VolumetricFog.png)

Shafts of the sun between pillars, halos round the lamps, shadows in the air. The Renderer keeps a 3D picture that follows the camera, 128 × 72 tiles by 64 slices (a **froxel** is a voxel of the view); a compute shader fills each with the density and the light it sends toward the eye, a second one adds them up from the camera outward, and every lit shader reads the result at its pixel. It replaces the plain fog of `fog_density` while it is on. After B. Wronski, "Volumetric Fog" (SIGGRAPH 2014), and S. Hillaire, Frostbite (SIGGRAPH 2015). `06n_VolumetricFog` shows a hall lit through the slats of its roof.

## See also

[Write a material](write-a-material.md) (the hook `light` changes how light falls) · [Screen effects](screen-effects.md) (bloom) · [Cel shading](../techniques/cel-shading.md)
