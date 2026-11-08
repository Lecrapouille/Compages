// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The GLSL of the map and of the sea of 10e_Archipelago: pieces put
//! after the PBR shaders of the Renderer (see Sea.cpp), with the swells of
//! swellsGlsl() before them.
// ****************************************************************************

namespace examples::archipelago
{

// The main() of the map: the PBR shader of the Renderer (its lights, its
// shadow, its fog and its tone curve are kept) with water where the alpha
// of the picture says: the sea from 0 (the shore) to 0.9 (90 m deep), a
// river at 0.94, the land at 1.
//
// The land: near the camera, pictures of grass, sand and rock give it its
// grain. Each is read at two scales (6 m and 23 m: no visible tiling) and
// divided by its mean colour (its smallest mipmap), so the colours of the
// map stay; sand on the beaches, rock on the steep slopes, grass elsewhere.
//
// The sea: waves in the normals (they fade where they are smaller than a
// pixel: fwidth of the phase), the sky in the water at a grazing look
// (Schlick's Fresnel), the glint of the sun, bands of surf running to the
// shore. Where the surface of the sea is drawn (near the camera), these
// fade out: the surface makes them, and the ground shows the sea bed.
inline constexpr char const* MAP_MAIN = R"(
uniform float time;
uniform sampler2D grassMap;
uniform sampler2D sandMap;
uniform sampler2D rockMap;
uniform vec2 surfaceCentre;
uniform float surfaceRadius;

out vec4 oColor;

vec3 grain(sampler2D p_map, vec2 p_at)
{
    vec3 fine = texture(p_map, p_at / 6.0).rgb;
    vec3 coarse = texture(p_map, p_at / 23.0).rgb;
    vec3 mean = textureLod(p_map, vec2(0.5), 12.0).rgb;
    return (0.5 * (fine + coarse)) / max(mean, vec3(0.02));
}

void main()
{
    vec4 texel = hasBaseColorMap ? texture(baseColorMap, vUv) : vec4(1.0);
    vec3 base = texel.rgb * baseColorFactor;
    vec2 p = vWorldPos.xz;
    float near = 1.0 - smoothstep(250.0, 1600.0, length(cameraPos - vWorldPos));
    if (texel.a > 0.97)
    {
        vec3 n = normalize(vNormal);
        float sand = 1.0 - smoothstep(2.0, 6.0, vWorldPos.y);
        float rock = smoothstep(0.22, 0.42, 1.0 - n.y) * (1.0 - sand);
        float grass = max(1.0 - sand - rock, 0.0);
        vec3 ground = (grain(grassMap, p) * grass) + (grain(sandMap, p) * sand) + (grain(rockMap, p) * rock);
        base *= mix(vec3(1.0), ground, 0.9 * near);
        oColor = vec4(present(applyFog(shade(base, n, vWorldPos), vWorldPos)), opacity);
        return;
    }
    bool river = texel.a > 0.915;
    float depth = river ? 0.5 : texel.a / 0.9;
    vec3 up = vec3(0.0, 1.0, 0.0);
    // The sea bed: sand in the shallows.
    base *= mix(vec3(1.0), grain(sandMap, p), 0.6 * near * (1.0 - smoothstep(0.0, 0.3, depth)));
    vec3 bed = shade(base, up, vWorldPos);
    vec2 slope = vec2(0.0);
    for (int i = river ? 5 : 0; i < SWELL_COUNT; ++i)
    {
        vec2 d = normalize(SWELLS[i].xy);
        float k = 2.0 * PI / SWELLS[i].z;
        float phase = k * dot(d, p) - sqrt(GRAVITY * k) * time + float(i) * 1.7;
        float fade = 1.0 - smoothstep(0.3, 1.2, fwidth(phase));
        slope += d * (SWELLS[i].w * cos(phase) * fade);
    }
    // Gusts: patches where the wind ruffles the water more, drifting.
    slope *= 0.6 + 0.4 * sin(p.x * 0.0021 + time * 0.05) * sin(p.y * 0.0017 - time * 0.04);
    vec3 n = normalize(vec3(-slope.x, 1.0, -slope.y));
    vec3 toEye = normalize(cameraPos - vWorldPos);
    vec3 toLight = normalize(-lightDir);
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(normalize(n + up), toEye), 0.0), 5.0);
    vec3 water = mix(bed, fogColor * 1.1, 0.7 * fresnel);
    float sun = sunVisibility(up, toLight, vWorldPos);
    water += lightColor * (2.5 * sun * pow(max(dot(reflect(-toLight, n), toEye), 0.0), 240.0));
    if (!river)
    {
        float shallow = 1.0 - smoothstep(0.0, 0.07, depth);
        float bands = sin(depth * 260.0 + time * 1.4);
        float broken = 0.5 + 0.5 * sin(p.x * 0.031 + time * 0.21) * sin(p.y * 0.027 - time * 0.17);
        float foam = shallow * smoothstep(0.55, 0.95, bands) * broken + (1.0 - smoothstep(0.0, 0.015, depth)) * 0.7;
        water = mix(water, vec3(0.85, 0.9, 0.9) * (0.6 + 0.4 * sun), clamp(foam, 0.0, 1.0) * 0.75);
    }
    // Under the surface of the sea, the sea bed alone.
    float open = (river || surfaceRadius <= 0.0)
                     ? 1.0
                     : smoothstep(0.7 * surfaceRadius, surfaceRadius, length(p - surfaceCentre));
    oColor = vec4(present(applyFog(mix(bed, water, open), vWorldPos)), opacity);
}
)";

// The surface of the sea, its vertex shader: the PBR one, with each corner
// moved by Gerstner waves (GPU Gems 1, chapter 1): a point of the water goes
// round a small circle, up and forward on the crest, so the crests are
// sharp and the troughs wide. The waves die in the shallows; on land the
// surface goes under the ground. Swells shorter than three quads are left
// to the normals of the fragment shader.
inline constexpr char const* SURFACE_WAVES = R"(
uniform float time;
uniform sampler2D baseColorMap;
uniform float gridStep;

out float vCrest;

vec3 gWaveNormal;

vec4 displace(vec4 p_world)
{
    float water = textureLod(baseColorMap, p_world.xz / MAP_SIZE + 0.5, 0.0).a;
    float calm = (water > 0.91) ? 0.0 : smoothstep(0.0, 0.08, water / 0.9);
    vec3 offset = vec3(0.0);
    vec3 normal = vec3(0.0, 1.0, 0.0);
    vCrest = 0.0;
    for (int i = 0; i < SWELL_COUNT; ++i)
    {
        vec2 d = normalize(SWELLS[i].xy);
        float k = 2.0 * PI / SWELLS[i].z;
        float amplitude = calm * smoothstep(3.0, 6.0, SWELLS[i].z / gridStep) * SWELLS[i].w / k;
        float phase = k * dot(d, p_world.xz) - sqrt(GRAVITY * k) * time + float(i) * 1.7;
        const float Q = 0.6;
        offset.xz += Q * amplitude * d * cos(phase);
        offset.y += amplitude * sin(phase);
        normal.xz -= d * (k * amplitude * cos(phase));
        normal.y -= Q * k * amplitude * sin(phase);
        vCrest += k * amplitude * sin(phase);
    }
    if (water > 0.91)
    {
        offset.y = -8.0;
    }
    gWaveNormal = normalize(normal);
    return vec4(p_world.xyz + offset, 1.0);
}
)";

// The surface of the sea, its fragment shader: the sea bed shows through
// the shallows, the deep water is darker and more opaque; the sky, the
// glint, foam on the steepest crests and in the surf; the surface fades at
// its edge into the waves of the ground.
inline constexpr char const* SURFACE_MAIN = R"(
uniform float time;
uniform vec2 surfaceCentre;
uniform float surfaceRadius;

in float vCrest;

out vec4 oColor;

void main()
{
    vec2 p = vWorldPos.xz;
    vec4 texel = hasBaseColorMap ? texture(baseColorMap, p / MAP_SIZE + 0.5) : vec4(0.0);
    if (texel.a > 0.91)
    {
        discard;
    }
    float depth = texel.a / 0.9;
    vec3 up = vec3(0.0, 1.0, 0.0);
    // Ripples smaller than the quads, in the normal.
    vec2 slope = vec2(0.0);
    for (int i = 5; i < SWELL_COUNT; ++i)
    {
        vec2 d = normalize(SWELLS[i].xy);
        float k = 2.0 * PI / SWELLS[i].z;
        float phase = k * dot(d, p) - sqrt(GRAVITY * k) * time + float(i) * 1.7;
        slope += d * (SWELLS[i].w * cos(phase) * (1.0 - smoothstep(0.3, 1.2, fwidth(phase))));
    }
    vec3 n = normalize(normalize(vNormal) + vec3(-slope.x, 0.0, -slope.y));
    vec3 toEye = normalize(cameraPos - vWorldPos);
    vec3 toLight = normalize(-lightDir);
    float sun = sunVisibility(up, toLight, vWorldPos);
    vec3 body = shade(mix(vec3(0.05, 0.32, 0.38), vec3(0.01, 0.06, 0.14), smoothstep(0.0, 0.6, depth)) * baseColorFactor,
                      up, vWorldPos);
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(n, toEye), 0.0), 5.0);
    vec3 water = mix(body, fogColor * 1.1, fresnel);
    water += lightColor * (3.0 * sun * pow(max(dot(reflect(-toLight, n), toEye), 0.0), 200.0));
    float shallow = 1.0 - smoothstep(0.0, 0.07, depth);
    float bands = sin(depth * 260.0 + time * 1.4);
    float broken = 0.5 + 0.5 * sin(p.x * 0.031 + time * 0.21) * sin(p.y * 0.027 - time * 0.17);
    float foam = clamp(shallow * smoothstep(0.55, 0.95, bands) * broken + smoothstep(0.16, 0.24, vCrest) * 0.6, 0.0, 1.0);
    water = mix(water, vec3(0.9, 0.95, 0.95) * (0.6 + 0.4 * sun), foam);
    float alpha = mix(0.3, 0.92, smoothstep(0.0, 0.35, depth));
    alpha = max(alpha, max(fresnel, foam));
    alpha *= 1.0 - smoothstep(0.7 * surfaceRadius, surfaceRadius, length(p - surfaceCentre));
    oColor = vec4(present(applyFog(water, vWorldPos)), alpha * opacity);
}
)";

} // namespace examples::archipelago
