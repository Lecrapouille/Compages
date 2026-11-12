// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The GLSL of 10f_FlowingWater: hooks "main" of the PBR shader of the
//! Renderer (its sun, its lamps, its shadow, its fog and its tone curve stay).
// ****************************************************************************

namespace examples::flowing
{

//! \brief What the water shows: the result, or one step of the paper.
enum class View : int
{
    Water = 0,      //!< The result.
    FlowMap = 1,    //!< The flow map as stored: RG = flow * 0.5 + 0.5.
    Pop = 2,        //!< One phase for the whole surface: it resets at once.
    Edges = 3,      //!< One phase offset by the noise: the edges crawl.
    TwoPhases = 4,  //!< Two phases blended by the cycled noise: no edge.
    Noise = 5,      //!< The noise texture that offsets the phase.
    Foam = 6,       //!< Where the colour map may show (B of the flow map).
    Open = 7,       //!< The water slowed along the walls (A of the flow map).
};

// The concrete and the metal: a picture projected along the three axes and
// blended by the normal (triplanar mapping), so long walls, pillars and
// pipes need no UV. Near the water they are wet (darker) with slime at the
// waterline.
inline constexpr char const* SOLID_MAIN = R"(
uniform sampler2D surfaceMap;
uniform float tileMetres;
uniform float waterLevel;

void main()
{
    vec3 n = normalize(vNormal);
    vec3 w = pow(abs(n), vec3(4.0));
    w /= (w.x + w.y + w.z);
    vec3 p = vWorldPos / tileMetres;
    vec3 c = texture(surfaceMap, p.zy).rgb * w.x + texture(surfaceMap, p.xz).rgb * w.y
           + texture(surfaceMap, p.xy).rgb * w.z;
    vec3 base = c * baseColorFactor;
    float above = vWorldPos.y - waterLevel;
    float wet = 1.0 - smoothstep(0.0, 0.5, above);
    float slime = 1.0 - smoothstep(-0.05, 0.14, above);
    base *= mix(1.0, 0.5, wet);
    base = mix(base, vec3(0.05, 0.075, 0.03), 0.65 * slime);
    oColor = vec4(present(applyFog(shade(base, n, vWorldPos), vWorldPos)), opacity);
}
)";

// The water. The flow map gives, at each place, a speed (RG, metres per
// second), how much foam has gathered (B) and how open the water is (A).
//
// A picture moved along the flow for a time t is read at p - flow * t. It
// can only be moved so far before it is stretched beyond recognition: the
// time goes from -cycle/2 to +cycle/2 and starts again (the pop). The noise
// offsets the phase of each pixel, so they do not all start again at once;
// the edges between those that did and those that did not crawl over the
// surface. So the picture is read twice, half a cycle apart, and blended by
// the same cycled phase: each layer starts again while it is invisible.
//
// The ripples (a normal map) flow first, then distort the reading of the
// foam: the foam rides the ripples instead of gliding under them. Floating
// objects add a swirl of their own, from uniforms: the flow reacts to the
// game, as the emancipation grids of the paper do.
inline constexpr char const* WATER_MAIN = R"(
uniform sampler2D flowMap;
uniform sampler2D noiseMap;
uniform sampler2D rippleMap;
uniform sampler2D foamMap;
uniform vec2 flowOrigin;
uniform float flowSpacing;
uniform vec2 flowCells;
uniform float time;
uniform float cycle;
uniform float flowScale;
uniform float maxSpeed;
uniform int waterView;
uniform vec2 floater0;
uniform vec2 floater1;
uniform vec2 floater2;
uniform float floaterPull;
uniform vec2 splashAt;
uniform float splashPower;

vec4 flowAt(vec2 p)
{
    vec2 uv = ((p - flowOrigin) / flowSpacing + 0.5) / flowCells;
    return texture(flowMap, uv);
}

// Round a floating object, the water turns and is drawn in a little.
vec2 swirl(vec2 p, vec2 at)
{
    vec2 d = p - at;
    return (vec2(-d.y, d.x) * 1.4 - d * 0.5) * floaterPull * exp(-dot(d, d) / 0.8);
}

// The ripple normal map read with a shift, tangent space (x along X, y
// along Z) from -1 to 1.
vec3 ripple(vec2 p, vec2 shift)
{
    vec3 a = texture(rippleMap, (p - shift) / 2.4).xyz * 2.0 - 1.0;
    vec3 b = texture(rippleMap, (p - shift * 1.3) / 0.9 + vec2(0.37, 0.71)).xyz * 2.0 - 1.0;
    return vec3(a.xy + 0.6 * b.xy, a.z);
}

vec3 lampGlint(vec3 pos, vec3 colour, float range, vec3 n, vec3 toEye)
{
    vec3 toLamp = pos - vWorldPos;
    float d = length(toLamp);
    float fade = pow(max(1.0 - d / range, 0.0), 2.0);
    return colour * (2.5 * fade * pow(max(dot(reflect(-toLamp / d, n), toEye), 0.0), 90.0));
}

// The grid of the UV of the colour map: shows how far it is stretched.
float uvLines(vec2 uv)
{
    vec2 g = abs(fract(uv * 2.0 + 0.5) - 0.5) / fwidth(uv * 2.0);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

void main()
{
    vec2 p = vWorldPos.xz;
    vec4 data = flowAt(p);
    if (waterView == 1)
    {
        oColor = vec4(data.xy / (2.0 * maxSpeed) + 0.5, 0.0, 1.0);
        return;
    }
    vec4 noise = texture(noiseMap, p / 9.0);
    if (waterView == 5)
    {
        oColor = vec4(vec3(noise.r), 1.0);
        return;
    }
    if (waterView == 7)
    {
        oColor = vec4(mix(vec3(0.8, 0.15, 0.1), vec3(0.85), data.a), 1.0);
        return;
    }
    vec2 flow = data.xy * flowScale + swirl(p, floater0) + swirl(p, floater1) + swirl(p, floater2);

    // The two phases, the second half a cycle after the first.
    float offset = (waterView == 2) ? 0.0 : noise.r;
    float t = time / cycle + offset;
    float phase0 = fract(t);
    float phase1 = fract(t + 0.5);
    float blend = abs(2.0 * phase0 - 1.0);
    vec2 shift0 = flow * ((phase0 - 0.5) * cycle);
    vec2 shift1 = flow * ((phase1 - 0.5) * cycle);
    bool single = (waterView == 2) || (waterView == 3);
    if (single)
    {
        blend = 0.0;
    }

    vec3 r = mix(ripple(p, shift0), ripple(p, shift1), blend);
    float rough = 0.35 + 0.65 * clamp(length(flow) / maxSpeed, 0.0, 1.0);
    vec3 n = normalize(vec3(r.x * rough, max(r.z, 0.2), r.y * rough));

    // Where the jet of the pipe falls: rings running out, white water.
    vec2 fromSplash = p - splashAt;
    float rs = max(length(fromSplash), 1.0e-3);
    float rings = sin(rs * 16.0 - time * 10.0) * exp(-1.4 * rs) * splashPower;
    n = normalize(n + vec3(fromSplash.x, 0.0, fromSplash.y) * (0.45 * rings / rs));
    float churned = splashPower * (1.0 - smoothstep(0.15, 1.2, rs))
                  * (0.45 + 0.55 * texture(foamMap, fromSplash * 0.6 + vec2(0.0, time * 0.9)).a);

    const float FOAM_TILE = 3.5;
    vec2 bend = r.xy * 0.035;
    vec4 foam0 = texture(foamMap, (p - shift0) / FOAM_TILE + bend);
    vec4 foam1 = texture(foamMap, (p - shift1) / FOAM_TILE + bend);
    vec4 foam = mix(foam0, foam1, blend);
    // The flow map says where the foam gathers; a coarse noise breaks the
    // monotony of the colour map over a large pool.
    float gathered = smoothstep(0.2, 0.75, data.z + (noise.g - 0.5) * 0.6);

    if (waterView == 6)
    {
        oColor = vec4(vec3(gathered), 1.0);
        return;
    }
    if (single || (waterView == 4))
    {
        vec2 uv = (p - shift0) / FOAM_TILE;
        vec3 c = mix(vec3(0.04, 0.07, 0.06), foam.rgb, foam.a);
        c = mix(c, vec3(0.2, 0.9, 1.0), 0.7 * uvLines(uv) * (single ? 1.0 : 0.0));
        if (waterView == 3)
        {
            // The pixels that have just started again.
            c = mix(c, vec3(1.0, 0.1, 0.05), 1.0 - smoothstep(0.0, 0.03, phase0));
        }
        oColor = vec4(toSrgb(c), 1.0);
        return;
    }

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 toEye = normalize(cameraPos - vWorldPos);
    vec3 toLight = normalize(-lightDir);
    float sun = sunVisibility(up, toLight, vWorldPos);
    vec3 body = shade(vec3(0.03, 0.045, 0.032), up, vWorldPos);
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(n, toEye), 0.0), 5.0);
    vec3 sky = mix(ambient * 1.6, fogColor * 1.3, 0.5);
    vec3 water = mix(body, sky, 0.85 * fresnel);
    water += lightColor * (2.0 * sun * pow(max(dot(reflect(-toLight, n), toEye), 0.0), 300.0));
    if (pointLightCount > 0) water += lampGlint(pointLightPos0, pointLightColor0, pointLightRange0, n, toEye);
    if (pointLightCount > 1) water += lampGlint(pointLightPos1, pointLightColor1, pointLightRange1, n, toEye);
    if (pointLightCount > 2) water += lampGlint(pointLightPos2, pointLightColor2, pointLightRange2, n, toEye);
    if (pointLightCount > 3) water += lampGlint(pointLightPos3, pointLightColor3, pointLightRange3, n, toEye);
    vec3 scum = shade(foam.rgb, normalize(up + 0.3 * n), vWorldPos);
    vec3 colour = mix(water, scum, foam.a * gathered);
    colour = mix(colour, shade(vec3(0.75, 0.78, 0.72), normalize(up + 0.5 * n), vWorldPos), clamp(churned, 0.0, 1.0));
    colour *= mix(0.55, 1.0, data.a);
    oColor = vec4(present(applyFog(colour, vWorldPos)), opacity);
}
)";

// The jet of the pipe and the crown of spray where it falls: a picture of
// streaks and foam scrolling along V (down the jet, up the crown), more
// opaque where it is white, thinner seen face on than at the edges. The
// crown thins out towards its ragged top (fade = 1).
inline constexpr char const* JET_MAIN = R"(
uniform sampler2D noiseMap;
uniform sampler2D foamMap;
uniform float time;
uniform float scroll;
uniform float fade;

void main()
{
    float s = vUv.y - time * scroll;
    float streaks = texture(noiseMap, vec2(vUv.x * 3.0, s * 0.35)).r;
    vec4 foam = texture(foamMap, vec2(vUv.x * 2.0, s * 0.5));
    vec3 n = normalize(vNormal);
    vec3 toEye = normalize(cameraPos - vWorldPos);
    float edge = 1.0 - abs(dot(n, toEye));
    float white = clamp(foam.a + 0.9 * streaks - 0.15, 0.0, 1.0);
    vec3 c = mix(shade(vec3(0.10, 0.12, 0.09), n, vWorldPos), shade(foam.rgb * 1.15, n, vWorldPos), white);
    c += lightColor * (0.15 * pow(edge, 3.0));
    float alpha = mix(0.5, 0.97, white) * (0.6 + 0.4 * edge);
    alpha *= mix(1.0, smoothstep(0.0, 0.2, (0.5 + 0.5 * streaks) - vUv.y), fade);
    oColor = vec4(present(applyFog(c, vWorldPos)), alpha * opacity);
}
)";

} // namespace examples::flowing
