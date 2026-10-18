// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "50_Complete/54_SweatAndDecals.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"

#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/ShaderLib.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace examples
{

namespace
{

//! \brief Texels of the water and force maps: one is about a millimetre of
//! skin, the size of a drop of sweat.
constexpr std::uint32_t SIZE = 256u;
//! \brief Texels of the decal map.
constexpr std::uint32_t DECAL_SIZE = 1024u;
//! \brief Fluid steps per frame: a drop moves at most one texel per step.
constexpr int STEPS_PER_FRAME = 6;
//! \brief Water left behind by a running drop: the wet trail.
constexpr float FILM = 0.008f;
//! \brief Water a still bead holds before it starts to run.
constexpr float START = 0.35f;

// ----------------------------------------------------------------------------
// The mesh drawn into its own texture: the position on screen is the uv.
// ----------------------------------------------------------------------------
constexpr char const* UV_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec2 uv;
uniform mat4 model;
out vec3 vWorldPos;
out vec3 vLocalPos;
out vec3 vNormal;

void main()
{
    vec4 world = model * vec4(position, 1.0);
    vWorldPos = world.xyz;
    vLocalPos = position;
    vNormal = mat3(transpose(inverse(model))) * normal;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
)";

// ----------------------------------------------------------------------------
// Pass 1, the force field. One pixel of this pass is one texel of the map, so
// dFdx(world position) is the piece of skin one texel to the right covers,
// and dFdy the one above: the two tangents of the UV map. Gravity along the
// skin is written in that frame, in texels per unit of world distance.
// ----------------------------------------------------------------------------
constexpr char const* FORCE_FRAGMENT = R"(#version 450 core
in vec3 vWorldPos;
in vec3 vLocalPos;
in vec3 vNormal;
uniform vec3 gravity;
uniform vec2 zone;      // local height where the forehead starts, head height
uniform float wobble;   // radians: the skin is not smooth, drops meander
uniform float scale;    // 1 / head size, for the wobble noise
out vec4 oColor;

float hash(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

float noise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i), hash(i + vec3(1, 0, 0)), f.x),
                   mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
                   mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y),
               f.z);
}

void main()
{
    vec3 n = normalize(vNormal);
    vec3 pu = dFdx(vWorldPos);
    vec3 pv = dFdy(vWorldPos);
    vec3 slope = gravity - dot(gravity, n) * n;

    // Solve slope = a pu + b pv: the same move, counted in texels.
    float e = dot(pu, pu);
    float f = dot(pu, pv);
    float g = dot(pv, pv);
    float det = e * g - f * f;
    vec2 flow = vec2(0.0);
    if (det > 1e-30)
    {
        float su = dot(slope, pu);
        float sv = dot(slope, pv);
        flow = vec2(g * su - f * sv, e * sv - f * su) / det;
    }
    float angle = (noise(vWorldPos * scale * 14.0) - 0.5) * wobble;
    flow = mat2(cos(angle), sin(angle), -sin(angle), cos(angle)) * flow;

    float facesDown = max(dot(n, gravity), 0.0);
    float forehead = smoothstep(zone.x, zone.x + 0.2 * zone.y, vLocalPos.y);
    oColor = vec4(flow, facesDown, 0.05 + 0.95 * forehead);
}
)";

constexpr char const* SCREEN_VERTEX = R"(#version 450 core
void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)";

// ----------------------------------------------------------------------------
// Pass 2, the water. A texel holds at most one drop, and where the drop sits
// inside it (ba, from -0.5 to 0.5): drops glide, they do not jump. A drop that
// crosses the border of its texel goes to that neighbour (diagonals too) with
// all its water but a film, the wet trail. Each texel works out what its
// eight neighbours send it, from their own state alone: the water moved is
// counted once on each side, so none is created on the way.
// ----------------------------------------------------------------------------
constexpr char const* FLUID_FRAGMENT = R"(#version 450 core
uniform sampler2D previous;  // r amount, g wetness, ba place of the drop in its texel
uniform sampler2D forces;
uniform float dt;
uniform float speed;         // world units per second down a wall
uniform float film;
uniform float start;
uniform float poreRate;
uniform float evaporation;
uniform vec4 drop;           // uv, unused, amount
out vec4 oColor;

uint pcg(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float random(uint v)
{
    return float(pcg(v)) / 4294967295.0;
}

struct Move
{
    float sent;    // water given to the texel "to"
    ivec2 to;      // the texel itself when the drop stays
    vec2 offset;   // where the drop sits, in the texel it ends in
};

// What the drop of texel q does this step.
Move advance(ivec2 q)
{
    Move move = Move(0.0, q, vec2(0.0));
    ivec2 size = textureSize(previous, 0);
    if (any(lessThan(q, ivec2(0))) || any(greaterThanEqual(q, size)))
    {
        return move;
    }
    vec4 water = texelFetch(previous, q, 0);
    vec4 force = texelFetch(forces, q, 0);
    // A still bead starts when it is heavy enough. A running drop goes on
    // until it has left most of its water behind; skin is rough, so where it
    // stops changes from texel to texel.
    float stop = mix(0.08, 0.2, random(uint(q.x) * 3571u + uint(q.y) * 6143u + 17u));
    bool moving = dot(water.ba, water.ba) > 0.0;
    bool runs = (force.a > 0.0) && ((water.r > start) || (moving && (water.r > stop)));
    if (!runs)
    {
        return move;   // stopped: back to the middle of its texel
    }
    // A heavy drop runs faster than a small one; never more than half a texel
    // per step, so that it only ever reaches a neighbour.
    vec2 step = force.rg * speed * clamp(water.r / start, 0.3, 2.0) * dt;
    float length2 = dot(step, step);
    step *= (length2 > 0.2) ? sqrt(0.2 / length2) : 1.0;
    vec2 place = water.ba + step;
    ivec2 cross = ivec2(greaterThan(place, vec2(0.5))) - ivec2(lessThan(place, vec2(-0.5)));
    move.offset = place - vec2(cross);
    if (cross != ivec2(0))
    {
        move.to = q + cross;
        move.sent = water.r - film;
    }
    return move;
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    vec4 water = texelFetch(previous, p, 0);
    vec4 force = texelFetch(forces, p, 0);
    if (force.a <= 0.0)
    {
        oColor = vec4(0.0);   // off the skin: water arriving here is lost
        return;
    }

    // What stays here, then what arrives; the drop sits at the centre of mass.
    Move self = advance(p);
    float amount = (self.to == p) ? water.r : film;
    vec2 weighted = (self.to == p) ? water.r * self.offset : vec2(0.0);
    float inflow = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if ((x == 0) && (y == 0))
            {
                continue;
            }
            Move from = advance(p + ivec2(x, y));
            if (from.to == p)
            {
                inflow += from.sent;
                weighted += from.sent * from.offset;
            }
        }
    }
    amount += inflow;
    vec2 offset = (amount > 0.0) ? weighted / amount : vec2(0.0);

    // A running drop under the nose or the chin falls.
    if (inflow > 0.0)
    {
        amount -= max(amount - film, 0.0) * smoothstep(0.55, 0.85, force.b);
    }

    // A few texels are pores; the forehead sweats most.
    if (random(uint(p.x) * 1973u + uint(p.y) * 9277u + 101u) > 0.994)
    {
        float strength = 0.5 + random(uint(p.x) * 7919u + uint(p.y) * 104729u);
        amount += poreRate * force.a * strength * dt;
    }

    // A drop added with the mouse, in the texel under it.
    if (p == ivec2(drop.xy * vec2(textureSize(previous, 0))))
    {
        amount += drop.w;
    }

    amount *= 1.0 - evaporation * dt;
    amount = (amount < 1e-4) ? 0.0 : amount;

    float wetness = max(water.g - 0.07 * dt, smoothstep(0.0, 1.5 * film, amount));
    oColor = vec4(amount, wetness, offset);
}
)";

// ----------------------------------------------------------------------------
// Pass 3, a decal: the box in front of the click is projected on the skin it
// contains, and the picture is painted into the decal map at those texels.
// ----------------------------------------------------------------------------
constexpr char const* DECAL_FRAGMENT = R"(#version 450 core
in vec3 vWorldPos;
in vec3 vLocalPos;
in vec3 vNormal;
uniform sampler2D picture;
uniform vec3 center;
uniform vec3 right;     // half size included
uniform vec3 up;
uniform vec3 forward;
uniform vec3 tint;
out vec4 oColor;

void main()
{
    vec3 d = vWorldPos - center;
    vec3 box = vec3(dot(d, right) / dot(right, right),
                    dot(d, up) / dot(up, up),
                    dot(d, forward) / dot(forward, forward));
    // Outside the box, or skin turned away from the decal: not painted.
    if (any(greaterThan(abs(box), vec3(1.0))) ||
        dot(normalize(vNormal), -normalize(forward)) < 0.2)
    {
        discard;
    }
    vec4 color = texture(picture, box.xy * 0.5 + 0.5);
    oColor = vec4(color.rgb * tint, color.a);
}
)";

// ----------------------------------------------------------------------------
// The skin: the PBR shader of the Renderer up to its main(), then this one.
// ----------------------------------------------------------------------------
constexpr char const* SKIN_MAIN = R"(
uniform sampler2D normalMap;
uniform sampler2D decalMap;
uniform sampler2D fluidMap;
uniform float film;
uniform int showWater;

// A tangent frame from the derivatives of the position and the uv, since the
// mesh has no tangents (Schüler, "Normal mapping without precomputed tangents").
vec3 bend(vec3 n, vec3 tangentNormal)
{
    vec3 dp1 = dFdx(vWorldPos);
    vec3 dp2 = dFdy(vWorldPos);
    vec2 duv1 = dFdx(vUv);
    vec2 duv2 = dFdy(vUv);
    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 t = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 b = dp2perp * duv1.y + dp1perp * duv2.y;
    float scale = inversesqrt(max(max(dot(t, t), dot(b, b)), 1e-20));
    return normalize(mat3(t * scale, b * scale, n) * tangentNormal);
}

out vec4 oColor;

void main()
{
    vec3 n = normalize(vNormal);
    vec3 skin = bend(n, texture(normalMap, vUv).xyz * 2.0 - 1.0);

    // Water. Each drop is a small round cap, centred where the water pass
    // says it sits; its radius grows as the cube root of its water. Caps of
    // nearby drops add up, so drops that touch merge.
    vec2 size = vec2(textureSize(fluidMap, 0));
    vec2 at = vUv * size;
    ivec2 base = ivec2(floor(at));
    float height = 0.0;
    vec2 slope = vec2(0.0);
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            ivec2 q = clamp(base + ivec2(x, y), ivec2(0), ivec2(size) - 1);
            vec4 cell = texelFetch(fluidMap, q, 0);
            float water = cell.r - film;
            if (water <= 0.0)
            {
                continue;
            }
            float radius = min(1.8 * pow(water, 1.0 / 3.0), 2.0);
            vec2 d = at - (vec2(q) + 0.5 + cell.ba);
            float r2 = dot(d, d) / (radius * radius);
            if (r2 < 1.0)
            {
                float top = 0.45 * radius;   // about the contact angle of sweat
                height += top * (1.0 - r2);
                slope -= 2.0 * top * d / (radius * radius);
            }
        }
    }
    float bead = smoothstep(0.0, 0.1, height);
    float wet = clamp(texture(fluidMap, vUv).g, 0.0, 1.0);
    vec3 drop = bend(n, normalize(vec3(-slope, 1.0)));
    vec3 normal = normalize(mix(skin, drop, bead));

    vec3 albedo = baseColorFactor;
    if (hasBaseColorMap)
    {
        albedo *= texture(baseColorMap, vUv).rgb;
    }
    vec4 decal = texture(decalMap, vUv);
    albedo = mix(albedo, decal.rgb, clamp(decal.a, 0.0, 1.0));
    // Wet skin is darker, and a bead darker still: light goes through it.
    albedo *= mix(1.0, 0.8, wet) * mix(1.0, 0.7, bead);

    vec3 shaded = shade(albedo, normal, vWorldPos);

    // Water is a mirror: a sharp highlight of the sun and of the sky at
    // grazing angles, on beads and, fainter, on the wet trail.
    vec3 toLight = normalize(-lightDir);
    vec3 toEye = normalize(cameraPos - vWorldPos);
    vec3 halfway = normalize(toLight + toEye);
    float sun = sunVisibility(normal, toLight, vWorldPos);
    float gloss = 0.5 * wet + 1.5 * bead;
    float highlight = pow(max(dot(normal, halfway), 0.0), mix(80.0, 300.0, bead));
    // And the sky it reflects (Schlick, 2 % for water): the rim of each bead.
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(normal, toEye), 0.0), 5.0);
    vec3 mirrored = reflect(-toEye, normal);
    vec3 sky = mix(0.3 * ambient, 2.0 * ambient + 0.5 * lightColor, smoothstep(-0.3, 0.8, mirrored.y));
    shaded += lightColor * highlight * gloss * sun + sky * fresnel * gloss;
    // The glint that makes a bead read as water: a small window of light
    // above the viewer, mirrored on the top of each drop.
    vec3 window = normalize(toEye + vec3(0.0, 0.8, 0.0));
    float glint = pow(max(dot(reflect(-window, normal), toEye), 0.0), 120.0);
    shaded += vec3(0.9) * glint * bead;

    if (showWater == 1)
    {
        shaded = mix(shaded, vec3(0.1, 0.45, 1.0), max(bead, 0.5 * wet));
    }
    shaded = applyFog(shaded, vWorldPos);
    oColor = vec4(pow(shaded, vec3(1.0 / 2.2)), 1.0);
}
)";

} // namespace

std::string SweatAndDecals::description() const
{
    return "The head of three.js webgl_decals sweats. Water and decals live in "
           "the UV map of the mesh: gravity is projected on the skin, beads "
           "fill at the pores and run down, decals are boxes projected on the "
           "skin. Left click paints a decal (or drops water), right drag "
           "turns.";
}

compages::Status SweatAndDecals::setUp()
{
    const std::string head = dataPath("LeePerrySmith/LeePerrySmith.glb");
    if (head.empty())
    {
        return compages::failure("LeePerrySmith/LeePerrySmith.glb is missing "
                                 "from external/Compages-data/");
    }

    m_scene.background(0.09f, 0.10f, 0.12f).ambient(0.30f, 0.30f, 0.33f);
    COMPAGES_TRY_ASSIGN(m_root, m_scene.load(head));
    m_world.each<compages::renderer::MeshRenderer>(
        [this](compages::world::Entity p_entity,
               compages::renderer::MeshRenderer&) { m_head = p_entity; });
    if (!m_head)
    {
        return compages::failure("LeePerrySmith.glb holds no mesh");
    }

    // The UV passes and the raycast need the triangles on the CPU.
    compages::renderer::MeshRenderer const& drawn =
        m_head.get<compages::renderer::MeshRenderer>();
    compages::renderer::MeshAsset const* mesh =
        m_scene.assets().mesh(drawn.mesh);
    if ((mesh == nullptr) || mesh->source_vertices.empty())
    {
        return compages::failure("the head mesh has no vertices");
    }
    m_vertices = mesh->source_vertices;
    m_indices = mesh->source_indices;
    for (compages::renderer::MeshVertex const& vertex : m_vertices)
    {
        m_bounds.expand(vertex.position);
    }

    // frameAll() sees the shoulders too: come closer, in front of the face.
    m_scene.frameAll();
    const compages::core::Matrix44f& model = m_head.worldMatrix();
    const compages::core::Vector3f low =
        compages::core::transformPoint(model, m_bounds.min);
    const compages::core::Vector3f high =
        compages::core::transformPoint(model, m_bounds.max);
    const compages::core::Vector3f face((low.x + high.x) * 0.5f,
                                        low.y + 0.68f * (high.y - low.y),
                                        (low.z + high.z) * 0.5f);
    const float height = high.y - low.y;
    m_scene.activeCamera()
        .position(face + compages::core::Vector3f(
                             0.2f * height, 0.05f * height, 0.75f * height))
        .lookAt(face)
        .add<compages::world::Orbit>(face);

    COMPAGES_TRY(makeSkinMaterial(m_head));
    COMPAGES_TRY(makeTargets());
    return m_scene.prepare();
}

compages::Status
SweatAndDecals::makeSkinMaterial(compages::world::Entity p_mesh)
{
    // The colour map through an ordinary look, then the material of that look
    // is swapped for ours. The renderer still sets every PBR uniform.
    m_scene.look(
        p_mesh.id(),
        compages::renderer::texture(dataPath("LeePerrySmith/Map-COL.jpg")));

    std::string source = compages::renderer::shaders::pbrFragment();
    source = source.substr(0u, source.find("out vec4 oColor;")) + SKIN_MAIN;

    compages::renderer::Material material;
    material.name = "SweatySkin";
    material.family = compages::renderer::ShaderFamily::PbrMinimal;
    COMPAGES_TRY_ASSIGN(material.program,
                        compages::gpu::Program::fromSources(
                            compages::renderer::shaders::pbrVertex(), source));
    COMPAGES_TRY_ASSIGN(
        material.pipeline,
        compages::gpu::Pipeline::create<compages::renderer::MeshVertex>(
            material.program,
            compages::gpu::RenderState{
                .depth_test = true, .cull = compages::gpu::CullMode::Back }));
    material.program.set("normalMap", 2);
    material.program.set("decalMap", 3);
    material.program.set("fluidMap", 4);
    material.program.set("film", FILM);
    COMPAGES_TRY_ASSIGN(
        m_skin,
        m_scene.assets().addMaterial("SweatySkin", std::move(material)));

    compages::renderer::MaterialInstance* instance =
        m_scene.assets().materialInstance(
            p_mesh.get<compages::renderer::MeshRenderer>().material_instance);
    if (instance == nullptr)
    {
        return compages::failure("the head has no material instance");
    }
    instance->material = m_skin;

    // A normal map holds directions, not colours: no sRGB.
    COMPAGES_TRY(m_normal_map.load(
        dataPath("LeePerrySmith/Infinite-Level_02_Tangent_SmoothUV.jpg")));
    compages::gpu::LoadOptions picture;
    picture.srgb = true;
    picture.wrap = compages::gpu::Wrap::ClampToEdge;
    return m_decal_image.load(dataPath("decal/decal-diffuse.png"), picture);
}

compages::Status SweatAndDecals::makeTargets()
{
    compages::gpu::TextureDesc field{ .format =
                                          compages::gpu::PixelFormat::RGBA32F,
                                      .width = SIZE,
                                      .height = SIZE,
                                      .magnify = compages::gpu::Filter::Nearest,
                                      .minify =
                                          compages::gpu::Filter::Nearest };
    COMPAGES_TRY(m_forces.allocate(field));
    COMPAGES_TRY(m_forces_target.attach(m_forces));
    // The skin reads the water between texels: linear.
    field.magnify = compages::gpu::Filter::Linear;
    field.minify = compages::gpu::Filter::Linear;
    for (int i = 0; i < 2; ++i)
    {
        COMPAGES_TRY(m_fluid[i].allocate(field));
        COMPAGES_TRY(m_fluid_target[i].attach(m_fluid[i]));
        clear(m_fluid_target[i]);
    }
    field.format = compages::gpu::PixelFormat::RGBA16F;
    field.width = DECAL_SIZE;
    field.height = DECAL_SIZE;
    COMPAGES_TRY(m_decals.allocate(field));
    COMPAGES_TRY(m_decals_target.attach(m_decals));
    clear(m_decals_target);

    // UV passes: no depth, both faces (the winding in UV space is any).
    const compages::gpu::RenderState flat{ .depth_test = false,
                                           .cull =
                                               compages::gpu::CullMode::None };
    COMPAGES_TRY(m_force_pass.load(UV_VERTEX, FORCE_FRAGMENT));
    m_force_pass.vertices(m_vertices);
    m_force_pass.indices(m_indices);
    m_force_pass.state() = flat;
    const float height = std::max(m_bounds.max.y - m_bounds.min.y, 1e-6f);
    m_force_pass["zone"] =
        compages::core::Vector2f(m_bounds.min.y + 0.55f * height, height);
    m_force_pass["wobble"] = 1.2f;

    COMPAGES_TRY(m_decal_pass.load(UV_VERTEX, DECAL_FRAGMENT));
    m_decal_pass.vertices(m_vertices);
    m_decal_pass.indices(m_indices);
    m_decal_pass.state() = flat;
    m_decal_pass.blend(compages::gpu::Blend::alpha());
    m_decal_pass["picture"] = m_decal_image;

    COMPAGES_TRY(m_fluid_pass.load(SCREEN_VERTEX, FLUID_FRAGMENT));
    m_fluid_pass["film"] = FILM;
    m_fluid_pass["start"] = START;
    return compages::success();
}

void SweatAndDecals::clear(compages::gpu::Framebuffer const& p_target)
{
    compages::gpu::RenderPass const pass(p_target,
                                         { .clear_color = true,
                                           .color = { 0.0f, 0.0f, 0.0f, 0.0f },
                                           .clear_depth = false });
}

void SweatAndDecals::computeForces()
{
    // Gravity is fixed; the head turns: the field is made again every frame.
    const compages::core::Matrix44f& model = m_head.worldMatrix();
    m_force_pass["model"] = model;
    m_force_pass["gravity"] = compages::core::Vector3f(0.0f, -1.0f, 0.0f);
    const float size = compages::core::vector::norm(
        compages::core::transformPoint(model, m_bounds.max) -
        compages::core::transformPoint(model, m_bounds.min));
    m_force_pass["scale"] = 1.0f / std::max(size, 1e-6f);
    compages::gpu::RenderPass const pass(m_forces_target,
                                         { .clear_color = true,
                                           .color = { 0.0f, 0.0f, 0.0f, 0.0f },
                                           .clear_depth = false });
    m_force_pass.draw();
}

void SweatAndDecals::simulate(float p_dt)
{
    const compages::core::Matrix44f& model = m_head.worldMatrix();
    const float height = compages::core::vector::norm(
        compages::core::transformPoint(
            model, compages::core::Vector3f(0.0f, m_bounds.max.y, 0.0f)) -
        compages::core::transformPoint(
            model, compages::core::Vector3f(0.0f, m_bounds.min.y, 0.0f)));

    // At most one texel per step: long times are cut into more steps.
    const int steps =
        STEPS_PER_FRAME * std::max(1, int(std::ceil(p_dt / 0.05f)));
    const float dt =
        std::min(p_dt, 0.05f * float(steps / STEPS_PER_FRAME)) / float(steps);
    m_fluid_pass["dt"] = dt;
    m_fluid_pass["speed"] = m_flow_speed * height;
    m_fluid_pass["poreRate"] = m_sweat ? m_pore_rate : 0.0f;
    m_fluid_pass["evaporation"] = m_evaporation;
    m_fluid_pass["forces"] = m_forces;
    for (int i = 0; i < steps; ++i)
    {
        const int next = 1 - m_current;
        m_fluid_pass["previous"] = m_fluid[m_current];
        m_fluid_pass["drop"] =
            (i == 0) ? m_drop
                     : compages::core::Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
        compages::gpu::RenderPass const pass(
            m_fluid_target[next],
            { .clear_color = false, .clear_depth = false });
        m_fluid_pass.draw(3u);
        m_current = next;
    }
    m_drop = compages::core::Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
}

std::optional<SweatAndDecals::Hit>
SweatAndDecals::raycast(compages::core::Vector2f p_pixel) const
{
    compages::renderer::CameraFrame const& camera = m_scene.lastCamera();
    if ((camera.viewport_width == 0u) || (camera.viewport_height == 0u))
    {
        return std::nullopt;
    }
    const compages::core::Ray ray =
        camera.screenRay(p_pixel.x - camera.viewport_x,
                         p_pixel.y - camera.viewport_y,
                         camera.viewport_width,
                         camera.viewport_height);
    namespace vec = compages::core::vector;
    const compages::core::Matrix44f& model = m_head.worldMatrix();

    // Möller-Trumbore against every triangle; the nearest wins.
    std::optional<Hit> best;
    float nearest = std::numeric_limits<float>::max();
    for (std::size_t i = 0u; i + 2u < m_indices.size(); i += 3u)
    {
        compages::renderer::MeshVertex const& a = m_vertices[m_indices[i]];
        compages::renderer::MeshVertex const& b = m_vertices[m_indices[i + 1u]];
        compages::renderer::MeshVertex const& c = m_vertices[m_indices[i + 2u]];
        const compages::core::Vector3f p0 =
            compages::core::transformPoint(model, a.position);
        const compages::core::Vector3f e1 =
            compages::core::transformPoint(model, b.position) - p0;
        const compages::core::Vector3f e2 =
            compages::core::transformPoint(model, c.position) - p0;
        const compages::core::Vector3f h = vec::cross(ray.direction, e2);
        const float det = vec::dot(e1, h);
        if (std::abs(det) < 1e-12f)
        {
            continue;
        }
        const compages::core::Vector3f s = ray.origin - p0;
        const float u = vec::dot(s, h) / det;
        if ((u < 0.0f) || (u > 1.0f))
        {
            continue;
        }
        const compages::core::Vector3f q = vec::cross(s, e1);
        const float v = vec::dot(ray.direction, q) / det;
        if ((v < 0.0f) || (u + v > 1.0f))
        {
            continue;
        }
        const float t = vec::dot(e2, q) / det;
        if ((t <= 0.0f) || (t >= nearest))
        {
            continue;
        }
        nearest = t;
        compages::core::Vector3f normal = vec::normalize(vec::cross(e1, e2));
        if (vec::dot(normal, ray.direction) > 0.0f)
        {
            normal = normal * -1.0f;
        }
        const float w = 1.0f - u - v;
        best =
            Hit{ ray.pointAt(t), normal, (a.uv * w) + (b.uv * u) + (c.uv * v) };
    }
    return best;
}

void SweatAndDecals::stampDecal(Hit const& p_hit)
{
    namespace vec = compages::core::vector;
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    const compages::core::Matrix44f& model = m_head.worldMatrix();
    const float head =
        vec::norm(compages::core::transformPoint(model, m_bounds.max) -
                  compages::core::transformPoint(model, m_bounds.min));

    // The box of three.js: facing the skin, turned at random, of random size.
    const compages::core::Vector3f forward = p_hit.normal * -1.0f;
    const compages::core::Vector3f helper =
        (std::abs(forward.y) < 0.99f)
            ? compages::core::Vector3f(0.0f, 1.0f, 0.0f)
            : compages::core::Vector3f(1.0f, 0.0f, 0.0f);
    const compages::core::Vector3f x =
        vec::normalize(vec::cross(helper, forward));
    const compages::core::Vector3f y = vec::cross(forward, x);
    const float angle = unit(m_random) * 2.0f * std::numbers::pi_v<float>;
    const float half = head * (0.035f + 0.035f * unit(m_random));
    const compages::core::Vector3f right =
        ((x * std::cos(angle)) + (y * std::sin(angle))) * half;
    const compages::core::Vector3f up =
        ((y * std::cos(angle)) - (x * std::sin(angle))) * half;

    m_decal_pass["model"] = model;
    m_decal_pass["center"] = p_hit.point;
    m_decal_pass["right"] = right;
    m_decal_pass["up"] = up;
    m_decal_pass["forward"] = forward * half;
    m_decal_pass["tint"] =
        compages::core::Vector3f(0.2f + 0.8f * unit(m_random),
                                 0.2f + 0.8f * unit(m_random),
                                 0.2f + 0.8f * unit(m_random));
    compages::gpu::RenderPass const pass(
        m_decals_target, { .clear_color = false, .clear_depth = false });
    m_decal_pass.draw();
}

void SweatAndDecals::draw(compages::world::ViewFrame const& p_frame)
{
    m_root.rotation(compages::core::Quatf::fromAngleAxis(
                        units::angle::radian_t(m_roll), { 0.0f, 0.0f, 1.0f }) *
                    compages::core::Quatf::fromAngleAxis(
                        units::angle::radian_t(m_pitch), { 1.0f, 0.0f, 0.0f }));
    m_scene.update(p_frame);

    if (m_wipe)
    {
        clear(m_fluid_target[0]);
        clear(m_fluid_target[1]);
        m_wipe = false;
    }
    if (m_peel)
    {
        clear(m_decals_target);
        m_peel = false;
    }
    if (p_frame.input.mouse_left_pressed && p_frame.input.mouse_over)
    {
        if (std::optional<Hit> hit = raycast(p_frame.input.mouse))
        {
            if (m_click == 0)
            {
                stampDecal(*hit);
            }
            else
            {
                m_drop =
                    compages::core::Vector4f(hit->uv.x, hit->uv.y, 0.0f, 1.0f);
            }
        }
    }

    // Three decals once the camera of the last render is known, as if clicked.
    if (!m_first_decals && (m_scene.lastCamera().viewport_width != 0u))
    {
        m_first_decals = true;
        compages::renderer::CameraFrame const& camera = m_scene.lastCamera();
        for (compages::core::Vector2f at :
             { compages::core::Vector2f(0.40f, 0.62f),
               compages::core::Vector2f(0.62f, 0.45f),
               compages::core::Vector2f(0.47f, 0.30f) })
        {
            const compages::core::Vector2f pixel(
                camera.viewport_x + at.x * float(camera.viewport_width),
                camera.viewport_y + at.y * float(camera.viewport_height));
            if (std::optional<Hit> hit = raycast(pixel))
            {
                stampDecal(*hit);
            }
        }
    }

    computeForces();
    // The first frame runs a few seconds of sweat: the face is already wet.
    simulate(m_warm ? p_frame.elapsed : 6.0f);
    m_warm = true;

    // The extra maps of the skin, on the units its shader was given.
    m_normal_map.bind(2u);
    m_decals.bind(3u);
    m_fluid[m_current].bind(4u);
    if (compages::renderer::Material* skin = m_scene.assets().material(m_skin))
    {
        skin->program.set("showWater", m_show_water ? 1 : 0);
    }
    m_scene.render();
}

void SweatAndDecals::controls()
{
    ImGui::TextUnformatted("Left click");
    ImGui::RadioButton("Decal", &m_click, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Drop of water", &m_click, 1);

    ImGui::Checkbox("Sweat", &m_sweat);
    ImGui::SameLine();
    ImGui::Checkbox("Show the water", &m_show_water);
    ImGui::SliderFloat("Pores", &m_pore_rate, 0.0f, 0.5f, "%.2f /s");
    ImGui::SliderFloat(
        "Flow speed", &m_flow_speed, 0.01f, 0.5f, "%.2f heads/s");
    ImGui::SliderFloat("Evaporation", &m_evaporation, 0.0f, 0.3f, "%.2f /s");
    ImGui::SliderAngle("Pitch", &m_pitch, -60.0f, 60.0f);
    ImGui::SliderAngle("Roll", &m_roll, -60.0f, 60.0f);

    // Done in the next draw(), where the GPU passes are made.
    m_wipe = ImGui::Button("Wipe the face") || m_wipe;
    ImGui::SameLine();
    m_peel = ImGui::Button("Remove the decals") || m_peel;
}

} // namespace examples
