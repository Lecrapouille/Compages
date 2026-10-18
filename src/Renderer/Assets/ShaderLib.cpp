// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Assets/ShaderLib.hpp"

#include <string>

namespace compages::renderer::shaders
{

namespace
{

constexpr char const* POINT_LIGHT_UNIFORMS = R"(
uniform int pointLightCount;
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

vec3 samplePointLight(vec3 n, vec3 worldPos, vec3 pos, vec3 col, float range)
{
    vec3 toLight = pos - worldPos;
    float dist = length(toLight);
    if (dist > range)
    {
        return vec3(0.0);
    }
    vec3 L = toLight / dist;
    float ratio2 = (dist * dist) / (range * range);
    float window = clamp(1.0 - ratio2, 0.0, 1.0);
    float att = (window * window) / (1.0 + 8.0 * ratio2);
    return max(dot(n, L), 0.0) * att * col;
}

vec3 pointLightsContribution(vec3 n, vec3 worldPos)
{
    vec3 sum = vec3(0.0);
    if (pointLightCount > 0)
    {
        sum += samplePointLight(n, worldPos, pointLightPos0, pointLightColor0, pointLightRange0);
    }
    if (pointLightCount > 1)
    {
        sum += samplePointLight(n, worldPos, pointLightPos1, pointLightColor1, pointLightRange1);
    }
    if (pointLightCount > 2)
    {
        sum += samplePointLight(n, worldPos, pointLightPos2, pointLightColor2, pointLightRange2);
    }
    if (pointLightCount > 3)
    {
        sum += samplePointLight(n, worldPos, pointLightPos3, pointLightColor3, pointLightRange3);
    }
    return sum;
}
)";

constexpr char const* LIT_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in ivec4 joints;
in vec4 weights;
uniform int uJointCount;
uniform mat4 uJoints[64];
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vWorldPos;

mat4 skinMatrix()
{
    if (uJointCount <= 0) { return mat4(1.0); }
    return (uJoints[joints.x] * weights.x) + (uJoints[joints.y] * weights.y) +
           (uJoints[joints.z] * weights.z) + (uJoints[joints.w] * weights.w);
}

void main()
{
    mat4 skin = skinMatrix();
    vec4 local = skin * vec4(position, 1.0);
    vec4 world = model * local;
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(model * skin))) * normal;
    gl_Position = projection * view * world;
}
)";

constexpr char const* LIT_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 lightDir;
uniform vec3 ambient;
uniform vec3 color;

uniform vec3 lightColor;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;

vec3 applyFog(vec3 shaded, vec3 worldPos)
{
    float d = fogDensity * length(worldPos - cameraPos);
    return mix(shaded, fogColor, 1.0 - exp(-d * d));
}
)";

constexpr char const* LIT_FRAGMENT_TAIL = R"(

// Shading shared by the lit and PBR materials. One sun gives form; the rest
// keeps every face readable:
// - a hemisphere ambient: faces turned up get more sky than faces turned
//   down, so the sides of a box differ even in the shadow of the sun;
// - a soft fill light from the eye: faces away from the sun keep their
//   shape instead of one flat colour;
// - a small highlight (Blinn-Phong) that shows the curvature;
// - the shadow of the sun (sunVisibility), on the sun light only;
// - two-sided normals (open meshes, inverted STL faces) and a soft tone
//   curve, so bright colours do not burn to flat white.

// Shadow of the sun: the depth seen from the sun (shadow map), read with a
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
    vec3 shaded = base * light + lightColor * specular;
    // Soft shoulder above 0.8: keeps the hue of bright surfaces.
    vec3 over = max(shaded - 0.8, 0.0);
    return min(shaded, 0.8) + over / (1.0 + over * 2.5);
}

out vec4 oColor;

void main()
{
    vec3 shaded = applyFog(shade(color, normalize(vNormal), vWorldPos), vWorldPos);
    oColor = vec4(pow(shaded, vec3(1.0 / 2.2)), 1.0);
}
)";

constexpr char const* PBR_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec2 uv;
in ivec4 joints;
in vec4 weights;
uniform int uJointCount;
uniform mat4 uJoints[64];
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec2 vUv;
out vec3 vWorldPos;

mat4 skinMatrix()
{
    if (uJointCount <= 0) { return mat4(1.0); }
    return (uJoints[joints.x] * weights.x) + (uJoints[joints.y] * weights.y) +
           (uJoints[joints.z] * weights.z) + (uJoints[joints.w] * weights.w);
}

void main()
{
    mat4 skin = skinMatrix();
    vec4 local = skin * vec4(position, 1.0);
    vec4 world = model * local;
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(model * skin))) * normal;
    vUv = uv;
    gl_Position = projection * view * world;
}
)";

constexpr char const* PBR_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec2 vUv;
in vec3 vWorldPos;

uniform vec3 lightDir;
uniform vec3 ambient;
uniform vec3 baseColorFactor;
uniform sampler2D baseColorMap;
uniform bool hasBaseColorMap;

uniform vec3 lightColor;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;

vec3 applyFog(vec3 shaded, vec3 worldPos)
{
    float d = fogDensity * length(worldPos - cameraPos);
    return mix(shaded, fogColor, 1.0 - exp(-d * d));
}
)";

constexpr char const* PBR_FRAGMENT_TAIL = R"(

// Shading shared by the lit and PBR materials. One sun gives form; the rest
// keeps every face readable:
// - a hemisphere ambient: faces turned up get more sky than faces turned
//   down, so the sides of a box differ even in the shadow of the sun;
// - a soft fill light from the eye: faces away from the sun keep their
//   shape instead of one flat colour;
// - a small highlight (Blinn-Phong) that shows the curvature;
// - the shadow of the sun (sunVisibility), on the sun light only;
// - two-sided normals (open meshes, inverted STL faces) and a soft tone
//   curve, so bright colours do not burn to flat white.

// Shadow of the sun: the depth seen from the sun (shadow map), read with a
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
    vec3 shaded = base * light + lightColor * specular;
    // Soft shoulder above 0.8: keeps the hue of bright surfaces.
    vec3 over = max(shaded - 0.8, 0.0);
    return min(shaded, 0.8) + over / (1.0 + over * 2.5);
}

out vec4 oColor;

void main()
{
    vec3 albedo = baseColorFactor;
    if (hasBaseColorMap)
    {
        albedo *= texture(baseColorMap, vUv).rgb;
    }
    vec3 shaded = applyFog(shade(albedo, normalize(vNormal), vWorldPos), vWorldPos);
    oColor = vec4(pow(shaded, vec3(1.0 / 2.2)), 1.0);
}
)";

constexpr char const* SHADOW_VERTEX = R"(#version 450 core
in vec3 position;
in ivec4 joints;
in vec4 weights;
uniform int uJointCount;
uniform mat4 uJoints[64];
uniform mat4 model;
uniform mat4 lightViewProj;

mat4 skinMatrix()
{
    if (uJointCount <= 0) { return mat4(1.0); }
    return (uJoints[joints.x] * weights.x) + (uJoints[joints.y] * weights.y) +
           (uJoints[joints.z] * weights.z) + (uJoints[joints.w] * weights.w);
}

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

std::string concat3(char const* a, char const* b, char const* c)
{
    return std::string(a) + b + c;
}

} // namespace

char const* litVertex() { return LIT_VERTEX; }

char const* litFragment()
{
    static std::string const source =
        concat3(LIT_FRAGMENT, POINT_LIGHT_UNIFORMS, LIT_FRAGMENT_TAIL);
    return source.c_str();
}

char const* shadowVertex() { return SHADOW_VERTEX; }

char const* shadowFragment() { return SHADOW_FRAGMENT; }

char const* pbrVertex() { return PBR_VERTEX; }

char const* pbrFragment()
{
    static std::string const source =
        concat3(PBR_FRAGMENT, POINT_LIGHT_UNIFORMS, PBR_FRAGMENT_TAIL);
    return source.c_str();
}

constexpr char const* DEPTH_VERTEX = R"(#version 450 core
in vec3 position;
in ivec4 joints;
in vec4 weights;
uniform int uJointCount;
uniform mat4 uJoints[64];
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float vEyeDepth;
mat4 skinMatrix()
{
    if (uJointCount <= 0) { return mat4(1.0); }
    return (uJoints[joints.x] * weights.x) + (uJoints[joints.y] * weights.y) +
           (uJoints[joints.z] * weights.z) + (uJoints[joints.w] * weights.w);
}
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
in ivec4 joints;
in vec4 weights;
uniform int uJointCount;
uniform mat4 uJoints[64];
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vNormal;
mat4 skinMatrix()
{
    if (uJointCount <= 0) { return mat4(1.0); }
    return (uJoints[joints.x] * weights.x) + (uJoints[joints.y] * weights.y) +
           (uJoints[joints.z] * weights.z) + (uJoints[joints.w] * weights.w);
}
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

char const* depthVertex() { return DEPTH_VERTEX; }

char const* depthFragment() { return DEPTH_FRAGMENT; }

char const* normalsVertex() { return NORMALS_VERTEX; }

char const* normalsFragment() { return NORMALS_FRAGMENT; }

} // namespace compages::renderer::shaders
