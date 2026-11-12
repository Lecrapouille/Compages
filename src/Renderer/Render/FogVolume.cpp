// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/FogVolume.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Compages/Renderer/Render/LightClusters.hpp"
#include "Compages/Renderer/Render/ShadowPass.hpp"
#include "Renderer/Render/Offer.hpp"

#include <algorithm>
#include <cmath>

namespace compages::renderer
{

namespace
{

//! \brief The texture unit of the fog in the lit shaders.
constexpr std::uint32_t FOG_UNIT = 10u;

// ----------------------------------------------------------------------------
//! Pass 1: one invocation per froxel. Its centre in the world (the ray of
//! its tile, at the middle depth of its slice), how thick the fog is there,
//! and the light it sends toward the eye per metre.
// ----------------------------------------------------------------------------
constexpr char const* INJECT = R"(#version 450 core
#define COMPAGES_COMPUTE 1
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba16f, binding = 0) writeonly uniform image3D scattering;

#include <compages/scene.glsl>
#include <compages/sun.glsl>
#include <compages/lamps.glsl>

uniform ivec3 froxels;
uniform vec2 froxelDepth;        // near, far
uniform mat4 froxelInverseView;
uniform mat4 froxelInverseProjection;
uniform float fogDensityAt;      // extinction per metre at fogHeight
uniform float fogHeight;
uniform float fogFalloff;
uniform vec3 fogAlbedo;
uniform float fogAnisotropy;
uniform float fogNoise;
uniform vec3 fogWind;
uniform float fogTime;

float hash(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

float noise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i), hash(i + vec3(1, 0, 0)), f.x), mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x), mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y),
               f.z);
}

// Henyey-Greenstein, scaled so that an even scattering (g = 0) is 1: the
// lights keep the strength they have on surfaces.
float phase(float cosine, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / pow(max(1.0 + g2 - 2.0 * g * cosine, 1e-4), 1.5);
}

vec3 lamp(int i, vec3 pos, vec3 colour, float range, vec3 at, vec3 eye)
{
    vec3 toLamp = pos - at;
    float d = length(toLamp);
    if (d > range)
    {
        return vec3(0.0);
    }
    vec3 l = toLamp / max(d, 1e-4);
    float ratio2 = (d * d) / (range * range);
    float window = clamp(1.0 - ratio2, 0.0, 1.0);
    float att = (window * window) / (1.0 + 8.0 * ratio2);
    return colour * att * lampVisibility(i, l, at, pos, range) * phase(dot(l, eye), fogAnisotropy);
}

void main()
{
    ivec3 id = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(id, froxels)))
    {
        return;
    }
    vec2 uv = (vec2(id.xy) + 0.5) / vec2(froxels.xy);
    float t = (float(id.z) + 0.5) / float(froxels.z);
    float depth = froxelDepth.x * pow(froxelDepth.y / froxelDepth.x, t);
    vec4 near = froxelInverseProjection * vec4(uv * 2.0 - 1.0, -1.0, 1.0);
    vec3 ray = near.xyz / near.w;
    vec3 inView = ray * (depth / max(-ray.z, 1e-6));
    vec3 at = (froxelInverseView * vec4(inView, 1.0)).xyz;
    vec3 eye = normalize(at - cameraPos);

    float density = fogDensityAt * exp(-fogFalloff * max(at.y - fogHeight, 0.0));
    float clumps = noise(at * 0.35 - fogWind * fogTime) * 0.6 + noise(at * 1.1 - fogWind * (1.7 * fogTime)) * 0.4;
    density *= max(1.0 - fogNoise + 2.0 * fogNoise * clumps, 0.0);

    vec3 toSun = normalize(-lightDir);
    vec3 light = lightColor * sunVisibility(toSun, toSun, at) * phase(dot(toSun, eye), fogAnisotropy);
    if (pointLightCount > 0) light += lamp(0, pointLightPos0, pointLightColor0, pointLightRange0, at, eye);
    if (pointLightCount > 1) light += lamp(1, pointLightPos1, pointLightColor1, pointLightRange1, at, eye);
    if (pointLightCount > 2) light += lamp(2, pointLightPos2, pointLightColor2, pointLightRange2, at, eye);
    if (pointLightCount > 3) light += lamp(3, pointLightPos3, pointLightColor3, pointLightRange3, at, eye);
    if (clusterLightCount > 0)
    {
        uvec2 cell = clusterAt(uv, depth);
        for (uint k = 0u; k < cell.y; ++k)
        {
            uint index = clusterIndices[cell.x + k];
            float att;
            vec3 l = clusterLampToward(index, at, att);
            light += clusterLights[2u * index + 1u].rgb * att * phase(dot(l, eye), fogAnisotropy);
        }
    }
    light += ambient;
    imageStore(scattering, id, vec4(light * fogAlbedo * density, density));
}
)";

// ----------------------------------------------------------------------------
//! Pass 2: one invocation per tile, from the camera outward. Over a slice of
//! thickness dz, with extinction s and light sent per metre L, the light
//! gathered is L (1 - exp(-s dz)) / s and what passes is exp(-s dz).
// ----------------------------------------------------------------------------
constexpr char const* INTEGRATE = R"(#version 450 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba16f, binding = 0) readonly uniform image3D scattering;
layout(rgba16f, binding = 1) writeonly uniform image3D integrated;
uniform ivec3 froxels;
uniform vec2 froxelDepth;

void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(id, froxels.xy)))
    {
        return;
    }
    vec3 gathered = vec3(0.0);
    float through = 1.0;
    float before = 0.0;
    for (int k = 0; k < froxels.z; ++k)
    {
        float after = froxelDepth.x * pow(froxelDepth.y / froxelDepth.x, float(k + 1) / float(froxels.z));
        vec4 slice = imageLoad(scattering, ivec3(id, k));
        float s = max(slice.a, 1e-6);
        float passes = exp(-s * (after - before));
        gathered += through * (slice.rgb - slice.rgb * passes) / s;
        through *= passes;
        imageStore(integrated, ivec3(id, k), vec4(gathered, through));
        before = after;
    }
}
)";

// ----------------------------------------------------------------------------
//! The background: one triangle over the picture, the fog of the farthest
//! slice added to what is there (blend: one, source alpha).
// ----------------------------------------------------------------------------
constexpr char const* BACKGROUND_VERTEX = R"(#version 450 core
void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)";

constexpr char const* BACKGROUND_FRAGMENT = R"(#version 450 core
#include <compages/fog.glsl>
#include <compages/present.glsl>
out vec4 oColor;

void main()
{
    vec2 at = (gl_FragCoord.xy - fogVolumeViewport.xy) / fogVolumeViewport.zw;
    float last = 1.0 - 0.5 / float(textureSize(fogVolume, 0).z);
    vec4 fog = texture(fogVolume, vec3(at, last));
    oColor = vec4(present(fog.rgb), fog.a);
}
)";

//! \brief The distance in front of the camera at a depth of normalised
//! device coordinates.
float depthAt(compages::Matrix44f const& p_inverse_projection, float p_ndc)
{
    const compages::Vector4f p = p_inverse_projection * compages::Vector4f(0.0f, 0.0f, p_ndc, 1.0f);
    return -p.z / ((std::abs(p.w) > 1.0e-12f) ? p.w : 1.0e-12f);
}

} // namespace

// ----------------------------------------------------------------------------
Status FogVolume::create()
{
    COMPAGES_TRY(m_inject.load(shaders::compute("fog-inject.comp", INJECT)));
    COMPAGES_TRY(m_integrate.load(shaders::compute("fog-integrate.comp", INTEGRATE)));
    compages::gpu::TextureDesc desc;
    desc.kind = compages::gpu::TextureKind::Texture3D;
    desc.format = compages::gpu::PixelFormat::RGBA16F;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    desc.depth = SLICES;
    COMPAGES_TRY_ASSIGN(m_scattering, compages::gpu::Texture::create(desc));
    COMPAGES_TRY_ASSIGN(m_integrated, compages::gpu::Texture::create(desc));
    compages::gpu::ResolvedSource vertex;
    compages::gpu::ResolvedSource fragment;
    COMPAGES_TRY_ASSIGN(vertex, shaders::compute("fog-background.vert", BACKGROUND_VERTEX).resolve());
    COMPAGES_TRY_ASSIGN(fragment, shaders::compute("fog-background.frag", BACKGROUND_FRAGMENT).resolve());
    COMPAGES_TRY(m_background.loadSources(vertex.text, fragment.text));
    m_background.state().depth_test = false;
    m_background.state().depth_write = false;
    m_background.state().cull = compages::gpu::CullMode::None;
    m_background.state().blend = compages::gpu::Blend{ true,
                                                       compages::gpu::BlendFactor::One,
                                                       compages::gpu::BlendFactor::SrcAlpha,
                                                       compages::gpu::BlendEquation::Add,
                                                       compages::gpu::BlendFactor::Zero,
                                                       compages::gpu::BlendFactor::One,
                                                       compages::gpu::BlendEquation::Add };
    return success();
}

// ----------------------------------------------------------------------------
Status FogVolume::update(RenderSnapshot const& p_snapshot, ShadowPass const& p_shadows,
                         LightClusters const& p_clusters)
{
    VolumetricFog const& fog = p_snapshot.environment.volumetric;
    CameraFrame const& camera = p_snapshot.camera;
    m_on = fog.enabled && (camera.viewport_width > 0u) && (camera.viewport_height > 0u);
    if (!m_on)
    {
        return success();
    }
    if (!m_inject.valid())
    {
        COMPAGES_TRY(create());
    }
    const float near = std::max(depthAt(camera.inverse_projection, -1.0f), 0.05f);
    m_depth = compages::Vector2f(near, std::max(fog.distance, near * 2.0f));
    m_viewport = compages::Vector4f(camera.viewport_x, camera.viewport_y, float(camera.viewport_width),
                                    float(camera.viewport_height));
    m_view = camera.view;
    const auto [light_direction, light_color] = sunOf(p_snapshot);
    const float seconds =
        std::chrono::duration<float>(std::chrono::steady_clock::now() - m_start).count();

    compages::gpu::Program& inject = m_inject.program();
    p_shadows.bind(inject);
    COMPAGES_TRY(p_clusters.bind(inject));
    offer(inject, "lightDir", light_direction);
    offer(inject, "lightColor", light_color);
    offer(inject, "ambient", p_snapshot.environment.ambient);
    offer(inject, "cameraPos", camera.position);
    const int lamps = int(std::min(p_snapshot.point_lights.size(), std::size_t(shaders::MAX_POINT_LIGHTS)));
    offer(inject, "pointLightCount", lamps);
    static constexpr char const* POSITIONS[] = { "pointLightPos0", "pointLightPos1", "pointLightPos2",
                                                 "pointLightPos3" };
    static constexpr char const* COLOURS[] = { "pointLightColor0", "pointLightColor1", "pointLightColor2",
                                               "pointLightColor3" };
    static constexpr char const* RANGES[] = { "pointLightRange0", "pointLightRange1", "pointLightRange2",
                                              "pointLightRange3" };
    for (int i = 0; i < lamps; ++i)
    {
        PointLightFrame const& lamp = p_snapshot.point_lights[std::size_t(i)];
        offer(inject, POSITIONS[i], lamp.position);
        offer(inject, COLOURS[i], lamp.color * lamp.intensity);
        offer(inject, RANGES[i], lamp.range);
    }
    const compages::Vector3i froxels{ int(WIDTH), int(HEIGHT), int(SLICES) };
    offer(inject, "froxels", froxels);
    offer(inject, "froxelDepth", m_depth);
    offer(inject, "froxelInverseView", camera.inverse_view);
    offer(inject, "froxelInverseProjection", camera.inverse_projection);
    offer(inject, "fogDensityAt", fog.density);
    offer(inject, "fogHeight", fog.height);
    offer(inject, "fogFalloff", fog.falloff);
    offer(inject, "fogAlbedo", fog.color);
    offer(inject, "fogAnisotropy", std::clamp(fog.anisotropy, -0.95f, 0.95f));
    offer(inject, "fogNoise", std::clamp(fog.noise, 0.0f, 1.0f));
    offer(inject, "fogWind", fog.wind);
    offer(inject, "fogTime", seconds);
    COMPAGES_TRY(m_scattering.bindAsImage(0u, compages::gpu::ImageAccess::Write));
    COMPAGES_TRY(m_inject.dispatch((WIDTH + 7u) / 8u, (HEIGHT + 7u) / 8u, SLICES));
    compages::gpu::barrier(compages::gpu::Barrier::Image);

    compages::gpu::Program& integrate = m_integrate.program();
    offer(integrate, "froxels", froxels);
    offer(integrate, "froxelDepth", m_depth);
    COMPAGES_TRY(m_scattering.bindAsImage(0u, compages::gpu::ImageAccess::Read));
    COMPAGES_TRY(m_integrated.bindAsImage(1u, compages::gpu::ImageAccess::Write));
    COMPAGES_TRY(m_integrate.dispatch((WIDTH + 7u) / 8u, (HEIGHT + 7u) / 8u, 1u));
    compages::gpu::barrier(compages::gpu::Barrier::TextureFetch);
    return success();
}

// ----------------------------------------------------------------------------
void FogVolume::drawBackground(RenderSnapshot const& p_snapshot)
{
    if (!m_on)
    {
        return;
    }
    compages::gpu::Program& program = m_background.program();
    offer(program, "fogVolumeViewport", m_viewport);
    offer(program, "exposure", p_snapshot.environment.exposure);
    offer(program, "toneMapping", int(p_snapshot.environment.tone_mapping));
    m_background["fogVolume"] = m_integrated;
    m_background.draw(3u);
}

// ----------------------------------------------------------------------------
void FogVolume::bind(compages::gpu::Program& p_program) const
{
    if (!p_program.has("fogVolumeOn"))
    {
        return;
    }
    p_program.set("fogVolumeOn", m_on);
    if (!m_on)
    {
        return;
    }
    m_integrated.bind(FOG_UNIT);
    offer(p_program, "fogVolume", int(FOG_UNIT));
    offer(p_program, "fogVolumeDepth", m_depth);
    offer(p_program, "fogVolumeViewport", m_viewport);
    offer(p_program, "fogVolumeView", m_view);
}

} // namespace compages::renderer
