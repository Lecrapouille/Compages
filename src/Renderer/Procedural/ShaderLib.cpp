// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Procedural/ShaderLib.hpp"

#include <string>

namespace compages::renderer::shaders
{

namespace
{

// ============================================================================
// The pieces, given to every shader of the library as virtual includes.
// ============================================================================

constexpr char const* SKINNING = R"(in ivec4 joints;
in vec4 weights;
uniform int uJointCount;
uniform mat4 uJoints[64];

mat4 skinMatrix()
{
    if (uJointCount <= 0) { return mat4(1.0); }
    return (uJoints[joints.x] * weights.x) + (uJoints[joints.y] * weights.y) +
           (uJoints[joints.z] * weights.z) + (uJoints[joints.w] * weights.w);
}
)";

//! \brief The uniforms of the scene that the Renderer sets on lit materials.
constexpr char const* SCENE = R"(uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambient;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;
)";

constexpr char const* FOG = R"(#include <compages/scene.glsl>

// Volumetric fog (VolumetricFog, Environment::volumetric): a 3D picture
// following the camera (froxels: tiles of the picture times slices of the
// depth), holding the light the fog sends toward the eye from the camera to
// each slice (rgb) and what it lets through (a).
uniform bool fogVolumeOn;
uniform sampler3D fogVolume;
uniform vec2 fogVolumeDepth;      // near, far of the slices
uniform vec4 fogVolumeViewport;   // x, y, width, height in pixels
uniform mat4 fogVolumeView;

vec3 applyFog(vec3 shaded, vec3 worldPos)
{
#ifndef COMPAGES_COMPUTE
    if (fogVolumeOn)
    {
        vec2 at = (gl_FragCoord.xy - fogVolumeViewport.xy) / fogVolumeViewport.zw;
        float depth = max(-(fogVolumeView * vec4(worldPos, 1.0)).z, fogVolumeDepth.x);
        float slices = float(textureSize(fogVolume, 0).z);
        // Slice k holds the fog up to its far end: at s = k + 1.
        float s = log(depth / fogVolumeDepth.x) / log(fogVolumeDepth.y / fogVolumeDepth.x) * slices;
        vec4 fog = texture(fogVolume, vec3(at, (s - 0.5) / slices));
        return shaded * fog.a + fog.rgb;
    }
#endif
    float d = fogDensity * length(worldPos - cameraPos);
    return mix(shaded, fogColor, 1.0 - exp(-d * d));
}
)";

constexpr char const* SUN = R"(// Shadow of the sun: the depth seen from the sun (shadow map), read with a
// 3 x 3 filter so the edges are soft. Off when shadowEnabled is 0.
uniform int shadowEnabled;
uniform mat4 lightViewProj;
uniform sampler2D shadowMap;
uniform float shadowTexel;
uniform float shadowBias;

float sunVisibility(vec3 n, vec3 toLight, vec3 worldPos)
{
    if (shadowEnabled == 0)
    {
        return 1.0;
    }
    // Pushed along the normal by a texel: no acne on lit faces.
    vec4 clip = lightViewProj * vec4(worldPos + n * (shadowTexel * 1.5), 1.0);
    vec3 at = clip.xyz / clip.w * 0.5 + 0.5;
    if (at.x <= 0.0 || at.x >= 1.0 || at.y <= 0.0 || at.y >= 1.0 || at.z >= 1.0)
    {
        return 1.0;
    }
    float bias = shadowBias * (1.0 + 2.0 * (1.0 - max(dot(n, toLight), 0.0)));
    vec2 step = 1.0 / vec2(textureSize(shadowMap, 0));
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float nearest = texture(shadowMap, at.xy + vec2(x, y) * step).r;
            lit += (at.z - bias <= nearest) ? 1.0 : 0.0;
        }
    }
    return lit / 9.0;
}
)";

constexpr char const* LAMPS = R"(uniform int pointLightCount;
uniform vec3 pointLightPos0;
uniform vec3 pointLightPos1;
uniform vec3 pointLightPos2;
uniform vec3 pointLightPos3;
uniform vec3 pointLightColor0;
uniform vec3 pointLightColor1;
uniform vec3 pointLightColor2;
uniform vec3 pointLightColor3;
uniform float pointLightRange0;
uniform float pointLightRange1;
uniform float pointLightRange2;
uniform float pointLightRange3;

// Shadows of the lamps. The six faces of a cube around each lamp are drawn in
// one depth picture, an atlas: one row per lamp, one column per face. A texel
// holds the distance from the lamp to the nearest surface, over the range.
uniform vec4 lampShadows;      // 1 where lamp i casts shadows
uniform mat4 lampFaces[24];    // face f of lamp i: lampFaces[6 * i + f]
uniform sampler2D lampShadowMap;

float lampVisibility(int i, vec3 n, vec3 worldPos, vec3 pos, float range)
{
    if (lampShadows[i] < 0.5)
    {
        return 1.0;
    }
    vec3 v = worldPos - pos;
    vec3 a = abs(v);
    // The face that sees this point: the axis it is furthest along.
    int face = (a.x >= a.y && a.x >= a.z) ? ((v.x > 0.0) ? 0 : 1)
             : ((a.y >= a.z) ? ((v.y > 0.0) ? 2 : 3) : ((v.z > 0.0) ? 4 : 5));
    vec2 size = vec2(textureSize(lampShadowMap, 0));
    float tile = size.x / 6.0;
    // One texel of the face, in metres at this distance: the point is pushed
    // along its normal by about that much, so that lit faces have no acne.
    float texel = 2.0 * length(v) / tile;
    vec3 pushed = worldPos + n * (1.5 * texel);
    vec4 clip = lampFaces[6 * i + face] * vec4(pushed, 1.0);
    vec2 uv = (clip.xy / clip.w * 0.5 + 0.5) * tile;
    float depth = (length(pushed - pos) - texel) / range;
    vec2 corner = vec2(float(face), float(i)) * tile;
    // A 3 x 3 filter, kept inside the face so that it never reads its neighbour.
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 at = corner + clamp(uv + vec2(x, y), vec2(0.5), vec2(tile - 0.5));
            lit += (depth <= texture(lampShadowMap, at / size).r) ? 1.0 : 0.0;
        }
    }
    return lit / 9.0;
}

vec3 samplePointLight(int i, vec3 n, vec3 worldPos, vec3 pos, vec3 col, float range)
{
    vec3 toLight = pos - worldPos;
    float dist = length(toLight);
    if (dist > range)
    {
        return vec3(0.0);
    }
    vec3 L = toLight / dist;
    float lambert = max(dot(n, L), 0.0);
    if (lambert <= 0.0)
    {
        return vec3(0.0);
    }
    float ratio2 = (dist * dist) / (range * range);
    float window = clamp(1.0 - ratio2, 0.0, 1.0);
    float att = (window * window) / (1.0 + 8.0 * ratio2);
    return lambert * att * col * lampVisibility(i, n, worldPos, pos, range);
}

// The other lamps: clustered (LightClusters). The picture is cut into
// tiles, the depth into slices that grow with the distance; a pixel reads
// the lamps of its cluster only. No shadows for these.
layout(std430, binding = 9) readonly buffer ClusterLights { vec4 clusterLights[]; };
layout(std430, binding = 10) readonly buffer ClusterCells { uint clusterCells[]; };
layout(std430, binding = 11) readonly buffer ClusterIndices { uint clusterIndices[]; };
uniform int clusterLightCount;
uniform ivec2 clusterTiles;
uniform int clusterSlices;
uniform vec2 clusterDepth;     // near, far
uniform vec4 clusterViewport;  // x, y, width, height in pixels
uniform mat4 clusterView;

// The first index and the count of the lamps of a cluster, from a place on
// the picture (0 to 1 across the viewport) and a distance in front of the
// camera.
uvec2 clusterAt(vec2 at, float depth)
{
    ivec2 tile = clamp(ivec2(at * vec2(clusterTiles)), ivec2(0), clusterTiles - 1);
    depth = max(depth, clusterDepth.x);
    float t = log(depth / clusterDepth.x) / log(clusterDepth.y / clusterDepth.x);
    int slice = clamp(int(t * float(clusterSlices)), 0, clusterSlices - 1);
    uint cell = uint(((slice * clusterTiles.y) + tile.y) * clusterTiles.x + tile.x);
    return uvec2(clusterCells[2u * cell], clusterCells[2u * cell + 1u]);
}

// The light a clustered lamp sends to a point, like samplePointLight()
// without the shadow: its direction (xyz) and its strength (w) folded into
// the colour by the caller.
vec3 clusterLampToward(uint lamp, vec3 worldPos, out float att)
{
    vec4 posRange = clusterLights[2u * lamp];
    vec3 toLight = posRange.xyz - worldPos;
    float dist = length(toLight);
    float ratio2 = (dist * dist) / (posRange.w * posRange.w);
    float window = clamp(1.0 - ratio2, 0.0, 1.0);
    att = (window * window) / (1.0 + 8.0 * ratio2);
    return toLight / max(dist, 1e-4);
}

#ifndef COMPAGES_COMPUTE
// The cluster of the pixel being drawn (fragment shaders only).
uvec2 clusterOf(vec3 worldPos)
{
    vec2 at = (gl_FragCoord.xy - clusterViewport.xy) / clusterViewport.zw;
    return clusterAt(at, -(clusterView * vec4(worldPos, 1.0)).z);
}

vec3 clusteredLampsContribution(vec3 n, vec3 worldPos)
{
    vec3 sum = vec3(0.0);
    if (clusterLightCount == 0)
    {
        return sum;
    }
    uvec2 cell = clusterOf(worldPos);
    for (uint k = 0u; k < cell.y; ++k)
    {
        uint lamp = clusterIndices[cell.x + k];
        float att;
        vec3 l = clusterLampToward(lamp, worldPos, att);
        sum += max(dot(n, l), 0.0) * att * clusterLights[2u * lamp + 1u].rgb;
    }
    return sum;
}

vec3 pointLightsContribution(vec3 n, vec3 worldPos)
{
    vec3 sum = clusteredLampsContribution(n, worldPos);
    if (pointLightCount > 0)
    {
        sum += samplePointLight(0, n, worldPos, pointLightPos0, pointLightColor0, pointLightRange0);
    }
    if (pointLightCount > 1)
    {
        sum += samplePointLight(1, n, worldPos, pointLightPos1, pointLightColor1, pointLightRange1);
    }
    if (pointLightCount > 2)
    {
        sum += samplePointLight(2, n, worldPos, pointLightPos2, pointLightColor2, pointLightRange2);
    }
    if (pointLightCount > 3)
    {
        sum += samplePointLight(3, n, worldPos, pointLightPos3, pointLightColor3, pointLightRange3);
    }
    return sum;
}
#endif
)";

constexpr char const* SHADE = R"(// Shading shared by the lit and PBR materials. One sun gives form; the rest
// keeps every face readable:
// - a hemisphere ambient: faces turned up get more sky than faces turned
//   down, so the sides of a box differ even in the shadow of the sun;
// - a soft fill light from the eye: faces away from the sun keep their
//   shape instead of one flat colour;
// - a small highlight (Blinn-Phong) that shows the curvature;
// - the shadow of the sun (sunVisibility), on the sun light only;
// - two-sided normals (open meshes, inverted STL faces).
// Everything is linear light; present() turns it into screen colours.

#include <compages/scene.glsl>
#include <compages/sun.glsl>
#include <compages/lamps.glsl>

vec3 shade(vec3 base, vec3 n, vec3 worldPos)
{
    vec3 toLight = normalize(-lightDir);
    vec3 toEye = normalize(cameraPos - worldPos);
    if (dot(n, toEye) < 0.0)
    {
        n = -n;
    }
    float sky = 0.5 + 0.5 * n.y;
    vec3 ambientLight = ambient * mix(0.45, 1.3, sky);
    float diffuse = max(dot(n, toLight), 0.0) * sunVisibility(n, toLight, worldPos);
    float fill = 0.22 * max(dot(n, toEye), 0.0);
    vec3 halfway = normalize(toLight + toEye);
    float specular = diffuse > 0.0 ? 0.18 * pow(max(dot(n, halfway), 0.0), 40.0) : 0.0;
    vec3 light = ambientLight + lightColor * (0.85 * diffuse + fill) + pointLightsContribution(n, worldPos);
    return base * light + lightColor * specular;
}

// ---------------------------------------------------------------------------
// The metallic-roughness model of glTF (Cook-Torrance): a diffuse part, and
// a glint whose size is the roughness (GGX distribution, Smith shadowing,
// Schlick Fresnel; B. Karis, "Real Shading in Unreal Engine 4", 2013).
// A metal has no diffuse part and tints its glint with its colour. Without a
// picture of the surroundings, the ambient light stands for the sky in the
// glint, so that metals are not black. Scaled like shade(): a light of
// colour c on a white face turned to it gives about c.
// ---------------------------------------------------------------------------
const float PBR_PI = 3.14159265;

vec3 pbrLight(vec3 albedo, vec3 f0, float metallic, float roughness, vec3 n, vec3 v, vec3 l, vec3 radiance)
{
    float nl = max(dot(n, l), 0.0);
    if (nl <= 0.0)
    {
        return vec3(0.0);
    }
    vec3 h = normalize(v + l);
    float nv = max(dot(n, v), 1e-4);
    float nh = max(dot(n, h), 0.0);
    float a = max(roughness * roughness, 0.002);
    float a2 = a * a;
    float d = nh * nh * (a2 - 1.0) + 1.0;
    float distribution = a2 / (PBR_PI * d * d);
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float shadowing = (nv / (nv * (1.0 - k) + k)) * (nl / (nl * (1.0 - k) + k));
    vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(h, v), 0.0), 5.0);
    vec3 glint = distribution * shadowing * fresnel / (4.0 * nv * nl + 1e-4);
    vec3 diffuse = (1.0 - fresnel) * (1.0 - metallic) * albedo;
    return (diffuse + PBR_PI * glint) * nl * radiance;
}

vec3 pbrLamp(int i, vec3 albedo, vec3 f0, float metallic, float roughness, vec3 n, vec3 v, vec3 worldPos,
             vec3 pos, vec3 col, float range)
{
    vec3 toLight = pos - worldPos;
    float dist = length(toLight);
    if (dist > range)
    {
        return vec3(0.0);
    }
    float ratio2 = (dist * dist) / (range * range);
    float window = clamp(1.0 - ratio2, 0.0, 1.0);
    float att = (window * window) / (1.0 + 8.0 * ratio2);
    vec3 radiance = att * col * lampVisibility(i, n, worldPos, pos, range);
    return pbrLight(albedo, f0, metallic, roughness, n, v, toLight / dist, radiance);
}

vec3 shadePbr(vec3 albedo, vec3 n, vec3 worldPos, float metallic, float roughness)
{
    vec3 v = normalize(cameraPos - worldPos);
    if (dot(n, v) < 0.0)
    {
        n = -n;
    }
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 l = normalize(-lightDir);
    vec3 colour = pbrLight(albedo, f0, metallic, roughness, n, v, l, 0.85 * lightColor * sunVisibility(n, l, worldPos));
    if (pointLightCount > 0) colour += pbrLamp(0, albedo, f0, metallic, roughness, n, v, worldPos, pointLightPos0, pointLightColor0, pointLightRange0);
    if (pointLightCount > 1) colour += pbrLamp(1, albedo, f0, metallic, roughness, n, v, worldPos, pointLightPos1, pointLightColor1, pointLightRange1);
    if (pointLightCount > 2) colour += pbrLamp(2, albedo, f0, metallic, roughness, n, v, worldPos, pointLightPos2, pointLightColor2, pointLightRange2);
    if (pointLightCount > 3) colour += pbrLamp(3, albedo, f0, metallic, roughness, n, v, worldPos, pointLightPos3, pointLightColor3, pointLightRange3);
    if (clusterLightCount > 0)
    {
        uvec2 cell = clusterOf(worldPos);
        for (uint k = 0u; k < cell.y; ++k)
        {
            uint lamp = clusterIndices[cell.x + k];
            float att;
            vec3 l = clusterLampToward(lamp, worldPos, att);
            colour += pbrLight(albedo, f0, metallic, roughness, n, v, l, att * clusterLights[2u * lamp + 1u].rgb);
        }
    }
    // The sky and the fill light, as in shade(): diffuse, and in the glint.
    float sky = 0.5 + 0.5 * n.y;
    vec3 around = ambient * mix(0.45, 1.3, sky) + 0.22 * lightColor * max(dot(n, v), 0.0);
    vec3 fresnel = f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - max(dot(n, v), 0.0), 5.0);
    colour += around * ((1.0 - metallic) * albedo + fresnel * mix(1.0, 0.35, roughness));
    return colour;
}

// ---------------------------------------------------------------------------
// A normal map read without tangents on the mesh: the tangent frame comes
// from the derivatives of the position and of the uv (C. Schuler, "Normal
// Mapping Without Precomputed Tangents", 2013). The map holds OpenGL normals
// (+y up), as glTF does.
// ---------------------------------------------------------------------------
vec3 bumpNormal(vec3 n, vec3 worldPos, vec2 uv, vec3 tangentNormal)
{
    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 t = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 b = dp2perp * duv1.y + dp1perp * duv2.y;
    float scale = inversesqrt(max(max(dot(t, t), dot(b, b)), 1e-20));
    return normalize(mat3(t * scale, b * scale, n) * tangentNormal);
}
)";

constexpr char const* SRGB = R"(// A linear colour from 0 to 1, encoded as the screen expects (sRGB).
vec3 toSrgb(vec3 c)
{
    c = clamp(c, 0.0, 1.0);
    return mix(12.92 * c, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055,
               step(0.0031308, c));
}
)";

constexpr char const* PRESENT = R"(#include <compages/srgb.glsl>

// From the light computed in linear units to the colour of the screen:
// exposure, then a tone curve that brings any brightness under 1 without
// burning the hue (Environment::tone_mapping), then the sRGB encoding the
// screen expects. A custom main() calls it too.
uniform float exposure;
uniform int toneMapping;   // 0 none (clamp), 1 Reinhard, 2 ACES
uniform float opacity;

vec3 present(vec3 linear)
{
    vec3 c = max(linear * exposure, 0.0);
    if (toneMapping == 1)
    {
        c = c / (1.0 + c);
    }
    else if (toneMapping == 2)
    {
        // ACES filmic, Narkowicz's fit.
        c = (c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14);
    }
    return toSrgb(c);
}
)";

// ============================================================================
// The shaders.
// ============================================================================

constexpr char const* LIT_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vWorldPos;

void main()
{
    mat4 skin = skinMatrix();
    vec4 world = model * (skin * vec4(position, 1.0));
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(model * skin))) * normal;
    gl_Position = projection * view * world;
}
)";

constexpr char const* LIT_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vWorldPos;
uniform vec3 color;
#include <compages/fog.glsl>
#include <compages/shade.glsl>
#include <compages/present.glsl>

out vec4 oColor;

void main()
{
    vec3 shaded = applyFog(shade(color, normalize(vNormal), vWorldPos), vWorldPos);
    oColor = vec4(present(shaded), opacity);
}
)";

constexpr char const* PBR_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec2 uv;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec2 vUv;
out vec3 vWorldPos;

#include <hook/deform>

void main()
{
    mat4 skin = skinMatrix();
    vec4 world = model * (skin * vec4(position, 1.0));
    vec3 n = mat3(transpose(inverse(model * skin))) * normal;
    deform(world, n);
    vWorldPos = world.xyz;
    vNormal = n;
    vUv = uv;
    gl_Position = projection * view * world;
}
)";

constexpr char const* PBR_DEFORM = R"(void deform(inout vec4 world, inout vec3 normal)
{
}
)";

constexpr char const* PBR_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec2 vUv;
in vec3 vWorldPos;
uniform vec3 baseColorFactor;
uniform sampler2D baseColorMap;
uniform bool hasBaseColorMap;
// The metallic-roughness model of glTF, when the instance asks for it.
uniform bool pbrUseMetalRoughness;
uniform float pbrMetallic;
uniform float pbrRoughness;
uniform sampler2D pbrMetalRoughnessMap;   // G roughness, B metallic
uniform bool pbrHasMetalRoughnessMap;
uniform sampler2D pbrNormalMap;
uniform bool pbrHasNormalMap;
uniform float pbrNormalScale;
#include <compages/fog.glsl>
#include <compages/shade.glsl>
#include <compages/present.glsl>

out vec4 oColor;

#include <hook/main>
)";

constexpr char const* PBR_MAIN = R"(void main()
{
    vec3 albedo = baseColorFactor;
    if (hasBaseColorMap)
    {
        albedo *= texture(baseColorMap, vUv).rgb;
    }
    vec3 n = normalize(vNormal);
    vec3 lit;
    if (pbrUseMetalRoughness)
    {
        if (pbrHasNormalMap)
        {
            vec3 t = texture(pbrNormalMap, vUv).xyz * 2.0 - 1.0;
            n = bumpNormal(n, vWorldPos, vUv, normalize(vec3(t.xy * pbrNormalScale, t.z)));
        }
        float metallic = pbrMetallic;
        float roughness = pbrRoughness;
        if (pbrHasMetalRoughnessMap)
        {
            vec4 mr = texture(pbrMetalRoughnessMap, vUv);
            roughness *= mr.g;
            metallic *= mr.b;
        }
        lit = shadePbr(albedo, n, vWorldPos, metallic, roughness);
    }
    else
    {
        lit = shade(albedo, n, vWorldPos);
    }
    oColor = vec4(present(applyFog(lit, vWorldPos)), opacity);
}
)";

constexpr char const* SHADOW_VERTEX = R"(#version 450 core
in vec3 position;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 lightViewProj;

void main()
{
    gl_Position = lightViewProj * model * skinMatrix() * vec4(position, 1.0);
}
)";

constexpr char const* SHADOW_FRAGMENT = R"(#version 450 core
void main()
{
}
)";

// One face of the shadow cube of a lamp: the distance to the lamp, over its
// range, is what the depth picture keeps.
constexpr char const* LAMP_SHADOW_VERTEX = R"(#version 450 core
in vec3 position;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 lightViewProj;
out vec3 vWorldPos;

void main()
{
    vec4 world = model * skinMatrix() * vec4(position, 1.0);
    vWorldPos = world.xyz;
    gl_Position = lightViewProj * world;
}
)";

constexpr char const* LAMP_SHADOW_FRAGMENT = R"(#version 450 core
in vec3 vWorldPos;
uniform vec3 lampPos;
uniform float lampRange;

void main()
{
    gl_FragDepth = length(vWorldPos - lampPos) / lampRange;
}
)";

constexpr char const* DEPTH_VERTEX = R"(#version 450 core
in vec3 position;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float vEyeDepth;

void main()
{
    vec4 world = model * (skinMatrix() * vec4(position, 1.0));
    vec4 eye = view * world;
    vEyeDepth = -eye.z;
    gl_Position = projection * eye;
}
)";

constexpr char const* DEPTH_FRAGMENT = R"(#version 450 core
in float vEyeDepth;
uniform float near;
uniform float far;
uniform float opacity;
out vec4 oColor;

void main()
{
    float shade = 1.0 - smoothstep(near, far, vEyeDepth);
    oColor = vec4(vec3(shade), opacity);
}
)";

constexpr char const* NORMALS_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vNormal;

void main()
{
    mat4 skin = skinMatrix();
    vec4 world = model * (skin * vec4(position, 1.0));
    vNormal = mat3(transpose(inverse(view * model * skin))) * normal;
    gl_Position = projection * view * world;
}
)";

constexpr char const* NORMALS_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
uniform float opacity;
out vec4 oColor;

void main()
{
    oColor = vec4(0.5 * normalize(vNormal) + 0.5, opacity);
}
)";

constexpr char const* UNLIT_VERTEX = R"(#version 450 core
in vec3 position;
#include <compages/skinning.glsl>
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * (skinMatrix() * vec4(position, 1.0));
}
)";

// The colour as it was given: no light, no tone curve, only the sRGB encoding
// of a colour kept linear by the renderer.
constexpr char const* UNLIT_FRAGMENT = R"(#version 450 core
uniform vec3 color;
uniform float opacity;
#include <compages/srgb.glsl>
out vec4 oColor;

void main()
{
    oColor = vec4(toSrgb(color), opacity);
}
)";

// ----------------------------------------------------------------------------
//! \brief A shader of the library: its text, and every piece it may include.
// ----------------------------------------------------------------------------
compages::gpu::ShaderSource piece(char const* p_name, char const* p_text)
{
    compages::gpu::ShaderSource source =
        compages::gpu::ShaderSource::fromText(p_name, p_text);
    source.include("compages/skinning.glsl", SKINNING)
        .include("compages/scene.glsl", SCENE)
        .include("compages/fog.glsl", FOG)
        .include("compages/sun.glsl", SUN)
        .include("compages/lamps.glsl", LAMPS)
        .include("compages/shade.glsl", SHADE)
        .include("compages/srgb.glsl", SRGB)
        .include("compages/present.glsl", PRESENT);
    return source;
}

} // namespace

compages::gpu::ShaderSource compute(char const* p_name, char const* p_text)
{
    return piece(p_name, p_text);
}

ShaderPair lit()
{
    return { piece("lit.vert", LIT_VERTEX), piece("lit.frag", LIT_FRAGMENT) };
}

ShaderPair pbr()
{
    ShaderPair pair{ piece("pbr.vert", PBR_VERTEX),
                     piece("pbr.frag", PBR_FRAGMENT) };
    pair.vertex().declareHook("deform", PBR_DEFORM);
    pair.fragment().declareHook("main", PBR_MAIN);
    return pair;
}

ShaderPair unlit()
{
    return { piece("unlit.vert", UNLIT_VERTEX),
             piece("unlit.frag", UNLIT_FRAGMENT) };
}

ShaderPair depth()
{
    return { piece("depth.vert", DEPTH_VERTEX),
             piece("depth.frag", DEPTH_FRAGMENT) };
}

ShaderPair normals()
{
    return { piece("normals.vert", NORMALS_VERTEX),
             piece("normals.frag", NORMALS_FRAGMENT) };
}

ShaderPair shadow()
{
    return { piece("shadow.vert", SHADOW_VERTEX),
             piece("shadow.frag", SHADOW_FRAGMENT) };
}

ShaderPair lampShadow()
{
    return { piece("lamp_shadow.vert", LAMP_SHADOW_VERTEX),
             piece("lamp_shadow.frag", LAMP_SHADOW_FRAGMENT) };
}

} // namespace compages::renderer::shaders
