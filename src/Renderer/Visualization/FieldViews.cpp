// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Fields/HeightField.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Components/FieldView.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Procedural/Primitives.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Renderer/Visualization/FieldViews.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>

// ****************************************************************************
//! \file
//! \brief FieldViews (Scene::show()): a field drawn with the hooks of the PBR shader. The
//! values are a storage buffer (written from a Field, or by a compute shader),
//! read by the index of each corner; the vertex hook raises the corner by its
//! value, the fragment hook reads a colour map.
// ****************************************************************************

namespace compages::renderer
{

namespace
{

//! \brief How many colours a colour map picture holds.
constexpr std::uint32_t MAP_WIDTH = 256u;

// ----------------------------------------------------------------------------
//! The corners are raised by their value; the normal comes from the values
//! of the neighbours (central differences), so the mesh never changes.
// ----------------------------------------------------------------------------
constexpr char const* FIELD_DEFORM = R"(layout(std430) readonly buffer FieldValues
{
    float fieldValues[];
};
uniform int fieldColumns;
uniform int fieldRows;
uniform float fieldLow;
uniform float fieldHigh;
uniform float fieldRelief;
uniform float fieldSpacing;

out float vFieldValue;

float fieldValue(int i, int j)
{
    i = clamp(i, 0, fieldColumns - 1);
    j = clamp(j, 0, fieldRows - 1);
    return fieldValues[j * fieldColumns + i];
}

// The relief stops at the ends of the range, like the colours.
float fieldHeight(int i, int j)
{
    return fieldRelief * clamp(fieldValue(i, j), min(fieldLow, fieldHigh), max(fieldLow, fieldHigh));
}

void deform(inout vec4 world, inout vec3 normal)
{
    int i = gl_VertexID % fieldColumns;
    int j = gl_VertexID / fieldColumns;
    vFieldValue = fieldValue(i, j);
    float dx = fieldHeight(i + 1, j) - fieldHeight(i - 1, j);
    float dz = fieldHeight(i, j + 1) - fieldHeight(i, j - 1);
    world.y += fieldHeight(i, j);
    normal = normalize(vec3(-dx, 2.0 * fieldSpacing, -dz));
}
)";

// ----------------------------------------------------------------------------
//! The value, placed in the range, picks a colour of the map (an sRGB
//! picture: read as linear light). Lit like a ground, or shown as it is.
// ----------------------------------------------------------------------------
constexpr char const* FIELD_MAIN = R"(uniform sampler2D fieldColors;
uniform float fieldLow;
uniform float fieldHigh;
uniform bool fieldLit;

in float vFieldValue;

void main()
{
    float t = clamp((vFieldValue - fieldLow) / (fieldHigh - fieldLow), 0.0, 1.0);
    float width = float(textureSize(fieldColors, 0).x);
    vec3 colour = texture(fieldColors, vec2((t * (width - 1.0) + 0.5) / width, 0.5)).rgb;
    if (fieldLit)
    {
        vec3 shaded = applyFog(shade(colour, normalize(vNormal), vWorldPos), vWorldPos);
        oColor = vec4(present(shaded), opacity);
    }
    else
    {
        oColor = vec4(toSrgb(colour), opacity);
    }
}
)";

// ----------------------------------------------------------------------------
//! A volume: from where the ray enters the box (this fragment) to where it
//! leaves it, steps through the 3D picture of the values; each step adds its
//! colour and hides what is behind by 1 - exp(-k t dl) (emission and
//! absorption), k chosen so that the top of the range lets 5 % of the light
//! through across the box.
// ----------------------------------------------------------------------------
constexpr char const* VOLUME_MAIN = R"(uniform sampler3D fieldVolume;
uniform sampler2D fieldColors;
uniform float fieldLow;
uniform float fieldHigh;
uniform vec3 volumeMin;
uniform vec3 volumeMax;

const int STEPS = 160;

void main()
{
    vec3 dir = normalize(vWorldPos - cameraPos);
    vec3 inverse = 1.0 / dir;
    vec3 far = max((volumeMin - vWorldPos) * inverse, (volumeMax - vWorldPos) * inverse);
    float through = max(min(min(far.x, far.y), far.z), 0.0);
    float dl = through / float(STEPS);
    vec3 size = volumeMax - volumeMin;
    vec3 texels = vec3(textureSize(fieldVolume, 0));
    float k = 3.0 / max(length(size), 1e-6);
    float width = float(textureSize(fieldColors, 0).x);
    vec3 colour = vec3(0.0);
    float alpha = 0.0;
    for (int s = 0; (s < STEPS) && (alpha < 0.99); ++s)
    {
        vec3 p = vWorldPos + dir * (dl * (float(s) + 0.5));
        // The grid points are the centres of the texels.
        vec3 at = (((p - volumeMin) / size) * (texels - 1.0) + 0.5) / texels;
        float t = clamp((texture(fieldVolume, at).r - fieldLow) / (fieldHigh - fieldLow), 0.0, 1.0);
        float a = 1.0 - exp(-k * t * dl);
        vec3 c = texture(fieldColors, vec2((t * (width - 1.0) + 0.5) / width, 0.5)).rgb;
        colour += (1.0 - alpha) * a * c;
        alpha += (1.0 - alpha) * a;
    }
    oColor = vec4(toSrgb(colour / max(alpha, 1e-4)), alpha * opacity);
}
)";

// ----------------------------------------------------------------------------
//! Arrows: each corner carries the place of its length in the range (uv.x).
// ----------------------------------------------------------------------------
constexpr char const* ARROW_MAIN = R"(uniform sampler2D fieldColors;

void main()
{
    float width = float(textureSize(fieldColors, 0).x);
    vec3 colour = texture(fieldColors, vec2((clamp(vUv.x, 0.0, 1.0) * (width - 1.0) + 0.5) / width, 0.5)).rgb;
    oColor = vec4(toSrgb(colour), opacity);
}
)";

//! \brief A colour of a map at a place from 0 to 1, in sRGB bytes.
struct Stop
{
    float at;
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

// ----------------------------------------------------------------------------
//! \brief The stops of a map (from the published tables, sampled).
// ----------------------------------------------------------------------------
std::vector<Stop> stopsOf(ColorMap p_map)
{
    switch (p_map)
    {
        case ColorMap::Viridis:
            return { { 0.0f / 9.0f, 0x44, 0x01, 0x54 }, { 1.0f / 9.0f, 0x48, 0x28, 0x78 },
                     { 2.0f / 9.0f, 0x3E, 0x49, 0x89 }, { 3.0f / 9.0f, 0x31, 0x68, 0x8E },
                     { 4.0f / 9.0f, 0x26, 0x82, 0x8E }, { 5.0f / 9.0f, 0x1F, 0x9E, 0x89 },
                     { 6.0f / 9.0f, 0x35, 0xB7, 0x79 }, { 7.0f / 9.0f, 0x6D, 0xCD, 0x59 },
                     { 8.0f / 9.0f, 0xB4, 0xDE, 0x2C }, { 9.0f / 9.0f, 0xFD, 0xE7, 0x25 } };
        case ColorMap::Inferno:
            return { { 0.0f / 9.0f, 0x00, 0x00, 0x04 }, { 1.0f / 9.0f, 0x1B, 0x0C, 0x41 },
                     { 2.0f / 9.0f, 0x4A, 0x0C, 0x6B }, { 3.0f / 9.0f, 0x78, 0x1C, 0x6D },
                     { 4.0f / 9.0f, 0xA5, 0x2C, 0x60 }, { 5.0f / 9.0f, 0xCF, 0x44, 0x46 },
                     { 6.0f / 9.0f, 0xED, 0x69, 0x25 }, { 7.0f / 9.0f, 0xFB, 0x9B, 0x06 },
                     { 8.0f / 9.0f, 0xF7, 0xD1, 0x3D }, { 9.0f / 9.0f, 0xFC, 0xFF, 0xA4 } };
        case ColorMap::Coolwarm:
            return { { 0.0f / 6.0f, 0x3B, 0x4C, 0xC0 }, { 1.0f / 6.0f, 0x73, 0x96, 0xF5 },
                     { 2.0f / 6.0f, 0xB0, 0xCB, 0xFC }, { 3.0f / 6.0f, 0xDD, 0xDD, 0xDD },
                     { 4.0f / 6.0f, 0xF6, 0xBF, 0xA6 }, { 5.0f / 6.0f, 0xE7, 0x74, 0x5B },
                     { 6.0f / 6.0f, 0xB4, 0x04, 0x26 } };
        case ColorMap::Terrain:
            return { { 0.00f, 0x0B, 0x3D, 0x91 }, { 0.30f, 0x2E, 0x7F, 0xD1 },
                     { 0.48f, 0xE8, 0xD9, 0xA0 }, { 0.55f, 0x4C, 0x9A, 0x3A },
                     { 0.80f, 0x8B, 0x6B, 0x3E }, { 1.00f, 0xFF, 0xFF, 0xFF } };
        case ColorMap::Grey:
        default:
            return { { 0.0f, 0x00, 0x00, 0x00 }, { 1.0f, 0xFF, 0xFF, 0xFF } };
    }
}

// ----------------------------------------------------------------------------
//! \brief The picture of a map, made once per Scene catalog.
// ----------------------------------------------------------------------------
Result<TextureAssetId> colorMapTexture(AssetManager& p_assets, ColorMap p_map)
{
    const std::string name =
        "Compages colour map " + std::to_string(static_cast<int>(p_map));
    if (const TextureAssetId found = p_assets.findTexture(name); found.valid())
    {
        return found;
    }
    const std::vector<Stop> stops = stopsOf(p_map);
    std::vector<std::byte> pixels(std::size_t(MAP_WIDTH) * 4u);
    for (std::uint32_t x = 0u; x < MAP_WIDTH; ++x)
    {
        const float t = float(x) / float(MAP_WIDTH - 1u);
        std::size_t k = 1u;
        while ((k + 1u < stops.size()) && (stops[k].at < t))
        {
            ++k;
        }
        Stop const& a = stops[k - 1u];
        Stop const& b = stops[k];
        const float s = std::clamp((t - a.at) / std::max(b.at - a.at, 1.0e-6f), 0.0f, 1.0f);
        auto mix = [s](std::uint8_t p_a, std::uint8_t p_b)
        { return std::byte(std::lround((float(p_a) * (1.0f - s)) + (float(p_b) * s))); };
        pixels[(x * 4u) + 0u] = mix(a.red, b.red);
        pixels[(x * 4u) + 1u] = mix(a.green, b.green);
        pixels[(x * 4u) + 2u] = mix(a.blue, b.blue);
        pixels[(x * 4u) + 3u] = std::byte{ 255 };
    }
    TextureAsset picture;
    picture.name = name;
    COMPAGES_TRY_ASSIGN(picture.texture,
                        compages::gpu::Texture::create(compages::gpu::TextureDesc::image(
                            MAP_WIDTH, 1u, compages::gpu::PixelFormat::SRGB8A8)));
    COMPAGES_TRY(picture.texture.write(pixels));
    return p_assets.addTexture(name, std::move(picture));
}

// ----------------------------------------------------------------------------
//! \brief The corners of the box round the points of a grid of the plane.
// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
std::pair<compages::Vector2f, compages::Vector2f> extentOf(Grid const& p_grid)
{
    if constexpr (std::same_as<Grid, compages::Grid2D>)
    {
        const compages::Vector2f a = p_grid.point(0, 0);
        const compages::Vector2f b = p_grid.point(p_grid.columns - 1, p_grid.rows - 1);
        return { compages::Vector2f(std::min(a.x, b.x), std::min(a.y, b.y)),
                 compages::Vector2f(std::max(a.x, b.x), std::max(a.y, b.y)) };
    }
    else
    {
        return { p_grid.low(), p_grid.high() };
    }
}

// ----------------------------------------------------------------------------
//! \brief The distance between neighbour points: exact on a Grid2D, the
//! mean on other grids (for the normals of the relief, the width of arrows).
// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
float spacingOf(Grid const& p_grid)
{
    if constexpr (std::same_as<Grid, compages::Grid2D>)
    {
        return p_grid.spacing;
    }
    else
    {
        const auto [low, high] = extentOf(p_grid);
        return std::sqrt(std::max((high.x - low.x) * (high.y - low.y), 1.0e-12f) / float(p_grid.count()));
    }
}

// ----------------------------------------------------------------------------
//! \brief One corner per point of the grid, flat; each reads its own texel.
// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
MeshAsset gridMesh(Grid const& p_grid)
{
    MeshAsset mesh;
    mesh.source_vertices.reserve(p_grid.count());
    for (int j = 0; j < p_grid.rows; ++j)
    {
        for (int i = 0; i < p_grid.columns; ++i)
        {
            const compages::Vector2f at = p_grid.point(i, j);
            MeshVertex corner;
            corner.position = compages::Vector3f(at.x, 0.0f, at.y);
            corner.normal = compages::Vector3f(0.0f, 1.0f, 0.0f);
            corner.uv = compages::Vector2f((float(i) + 0.5f) / float(p_grid.columns),
                                           (float(j) + 0.5f) / float(p_grid.rows));
            mesh.source_vertices.push_back(corner);
        }
    }
    for (int j = 0; j + 1 < p_grid.rows; ++j)
    {
        for (int i = 0; i + 1 < p_grid.columns; ++i)
        {
            const auto a = std::uint32_t(p_grid.index(i, j));
            const auto b = std::uint32_t(p_grid.index(i + 1, j));
            const auto c = std::uint32_t(p_grid.index(i, j + 1));
            const auto d = std::uint32_t(p_grid.index(i + 1, j + 1));
            // Counter-clockwise seen from above (+Y).
            mesh.source_indices.insert(mesh.source_indices.end(), { a, c, b, b, c, d });
        }
    }
    mesh.index_count = mesh.source_indices.size();
    return mesh;
}

// ----------------------------------------------------------------------------
//! \brief The box of the field with its relief (which stops at the ends of
//! the range): the culling must see the raised corners.
// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
compages::AABB boundsOf(Grid const& p_grid, FieldLook const& p_look)
{
    const float a = p_look.relief * p_look.low;
    const float b = p_look.relief * p_look.high;
    const auto [low, high] = extentOf(p_grid);
    return compages::AABB::fromCorners(compages::Vector3f(low.x, std::min({ a, b, 0.0f }), low.y),
                                       compages::Vector3f(high.x, std::max({ a, b, 0.0f }), high.y));
}

// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
Status checkField(Grid const& p_grid, FieldLook const& p_look)
{
    if ((p_grid.columns < 2) || (p_grid.rows < 2))
    {
        return failure("show() needs a field of at least 2 x 2 points");
    }
    if (!(p_look.low < p_look.high) && !(p_look.low > p_look.high))
    {
        return failure("show() needs a range: low and high are both " +
                       std::to_string(p_look.low));
    }
    return success();
}

// ----------------------------------------------------------------------------
//! \brief The values, the range, the relief, the light and the colours.
// ----------------------------------------------------------------------------
Status applyLook(AssetManager& p_assets,
                 Material& p_material,
                 compages::gpu::BufferHandle p_values,
                 std::size_t p_count,
                 compages::Grid2D const& p_grid,
                 FieldLook const& p_look)
{
    if (p_count < p_grid.count())
    {
        return failure("a field of " + std::to_string(p_grid.count()) +
                       " points, and a buffer of " + std::to_string(p_count) + " values");
    }
    TextureAssetId colors;
    COMPAGES_TRY_ASSIGN(colors, colorMapTexture(p_assets, p_look.colors));
    p_material.textures = { { .sampler = "fieldColors", .texture = colors } };
    p_material.buffers = { { .block = "FieldValues", .buffer = p_values } };
    p_material.program.set("fieldColumns", p_grid.columns);
    p_material.program.set("fieldRows", p_grid.rows);
    p_material.program.set("fieldLow", p_look.low);
    p_material.program.set("fieldHigh", p_look.high);
    p_material.program.set("fieldLit", p_look.lit);
    p_material.program.set("fieldRelief", p_look.relief);
    p_material.program.set("fieldSpacing", p_grid.spacing);
    return success();
}

// ----------------------------------------------------------------------------
//! \brief The arrows of a field of vectors: a shaft and a head, flat on the
//! ground, from one point in \c every.
// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
MeshAsset arrowsMesh(compages::Field<compages::Vector2f, Grid> const& p_field, ArrowLook const& p_look)
{
    Grid const& grid = p_field.grid();
    const int every = std::max(p_look.every, 1);
    const float width = 0.12f * spacingOf(grid) * float(every);
    MeshAsset mesh;
    auto corner = [&mesh](compages::Vector2f p_at, float p_t)
    {
        MeshVertex v;
        v.position = compages::Vector3f(p_at.x, 0.0f, p_at.y);
        v.normal = compages::Vector3f(0.0f, 1.0f, 0.0f);
        v.uv = compages::Vector2f(p_t, 0.0f);
        mesh.source_vertices.push_back(v);
        return std::uint32_t(mesh.source_vertices.size() - 1u);
    };
    for (int j = 0; j < grid.rows; j += every)
    {
        for (int i = 0; i < grid.columns; i += every)
        {
            const compages::Vector2f v = p_field(i, j);
            const float length = std::hypot(v.x, v.y);
            if (length * p_look.scale < 1.0e-6f)
            {
                continue;
            }
            const float t = (length - p_look.low) / (p_look.high - p_look.low);
            const compages::Vector2f along = v * (1.0f / length);
            const compages::Vector2f side(-along.y, along.x);
            const compages::Vector2f from = grid.point(i, j);
            const float full = length * p_look.scale;
            const float head = std::min(0.35f * full, 3.0f * width);
            const compages::Vector2f neck = from + (along * (full - head));
            const compages::Vector2f tip = from + (along * full);
            const std::uint32_t a = corner(from + (side * (0.5f * width)), t);
            const std::uint32_t b = corner(from - (side * (0.5f * width)), t);
            const std::uint32_t c = corner(neck + (side * (0.5f * width)), t);
            const std::uint32_t d = corner(neck - (side * (0.5f * width)), t);
            const std::uint32_t e = corner(neck + (side * (1.5f * width)), t);
            const std::uint32_t f = corner(neck - (side * (1.5f * width)), t);
            const std::uint32_t g = corner(tip, t);
            mesh.source_indices.insert(mesh.source_indices.end(), { a, b, c, b, d, c, e, f, g });
        }
    }
    if (mesh.source_indices.empty())
    {
        // Nothing to show: one triangle of no size, so that the mesh exists.
        const std::uint32_t a = corner(grid.point(0, 0), 0.0f);
        mesh.source_indices = { a, a, a };
    }
    mesh.index_count = mesh.source_indices.size();
    const auto [first, last] = extentOf(grid);
    const float reach = (std::abs(p_look.high) + std::abs(p_look.low)) * p_look.scale + width;
    mesh.local_bounds = compages::AABB::fromCorners(
        compages::Vector3f(first.x - reach, -0.01f, first.y - reach),
        compages::Vector3f(last.x + reach, 0.01f, last.y + reach));
    return mesh;
}

//! \brief The corners of the box of a volume.
std::pair<compages::Vector3f, compages::Vector3f> boxOf(compages::Grid3D const& p_grid)
{
    const compages::Vector3f a = p_grid.point(0, 0, 0);
    const compages::Vector3f b = p_grid.point(p_grid.columns - 1, p_grid.rows - 1, p_grid.layers - 1);
    return { compages::Vector3f(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)),
             compages::Vector3f(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)) };
}

//! \brief The range, the colours and the box of a volume.
Status applyVolumeLook(AssetManager& p_assets,
                       Material& p_material,
                       TextureAssetId p_volume,
                       compages::Grid3D const& p_grid,
                       FieldLook const& p_look)
{
    TextureAssetId colors;
    COMPAGES_TRY_ASSIGN(colors, colorMapTexture(p_assets, p_look.colors));
    p_material.textures = { { .sampler = "fieldVolume", .texture = p_volume },
                            { .sampler = "fieldColors", .texture = colors } };
    const auto [low, high] = boxOf(p_grid);
    p_material.program.set("volumeMin", low);
    p_material.program.set("volumeMax", high);
    p_material.program.set("fieldLow", p_look.low);
    p_material.program.set("fieldHigh", p_look.high);
    return success();
}

Status checkVolume(compages::Grid3D const& p_grid, FieldLook const& p_look)
{
    if ((p_grid.columns < 2) || (p_grid.rows < 2) || (p_grid.layers < 2))
    {
        return failure("show() needs a volume of at least 2 x 2 x 2 points");
    }
    return checkField(compages::Grid2D{ .columns = 2, .rows = 2 }, p_look);
}

} // namespace

// ----------------------------------------------------------------------------
compages::world::Entity FieldViews::show(Scene& p_scene,
                                         compages::Field<float, compages::Grid3D> const& p_volume,
                                         FieldLook const& p_look,
                                         std::string p_name)
{
    compages::Grid3D const& grid = p_volume.grid();
    if (const Status valid = checkVolume(grid, p_look); !valid)
    {
        compages::gpu::reportError(valid.error());
        return p_scene.world().entity(std::move(p_name));
    }
    const auto [low, high] = boxOf(grid);
    auto made = [&]() -> Result<std::pair<MeshAsset, FieldView>>
    {
        MeshAsset box;
        COMPAGES_TRY_ASSIGN(box, makeBox(high.x - low.x, high.y - low.y, high.z - low.z));
        const compages::Vector3f centre = (low + high) * 0.5f;
        for (MeshVertex& corner : box.source_vertices)
        {
            corner.position = corner.position + centre;
        }
        box.releaseGpu();
        box.local_bounds = compages::AABB::fromCorners(low, high);

        TextureAsset values;
        values.name = p_name + " volume";
        COMPAGES_TRY_ASSIGN(values.texture, compages::gpu::Texture::fromField(p_volume));
        FieldView view{ .material = {},
                        .columns = grid.columns,
                        .rows = grid.rows,
                        .layers = grid.layers,
                        .volume = {} };
        COMPAGES_TRY_ASSIGN(view.volume, p_scene.assets().addTexture({}, std::move(values)));

        shaders::ShaderPair pbr = shaders::pbr();
        pbr.fragment().hook("main", VOLUME_MAIN);
        const compages::gpu::RenderState front{ .depth_test = true, .cull = compages::gpu::CullMode::Back };
        Material material;
        COMPAGES_TRY_ASSIGN(
            material,
            Material::create(p_name + " volume",
                             { .family = ShaderFamily::PbrMinimal,
                               .shaders = std::move(pbr),
                               .state = front,
                               .see_through = compages::gpu::RenderState{
                                   .depth_test = true,
                                   .depth_write = false,
                                   .blend = compages::gpu::Blend::alpha(),
                                   .cull = compages::gpu::CullMode::Back } }));
        COMPAGES_TRY(applyVolumeLook(p_scene.assets(), material, view.volume, grid, p_look));
        COMPAGES_TRY_ASSIGN(view.material, p_scene.assets().addMaterial({}, std::move(material)));
        return std::pair{ std::move(box), view };
    }();
    if (!made)
    {
        compages::gpu::reportError(made.error());
        return p_scene.world().entity(std::move(p_name));
    }
    auto [box, view] = made.take();
    compages::world::Entity entity = p_scene.mesh(std::move(box), std::move(p_name));
    p_scene.look(entity, view.material);
    MeshRenderer& renderer = entity.get<MeshRenderer>();
    // See-through, drawn after the solid ones, and casting no shadow.
    renderer.flags = RenderFlags::NoShadowCaster;
    if (MaterialInstance* instance = p_scene.assets().materialInstance(renderer.material_instance))
    {
        instance->opacity = 0.999f;
    }
    entity.set(view);
    return entity;
}

// ----------------------------------------------------------------------------
Status FieldViews::show(Scene& p_scene,
                        compages::world::EntityId p_entity,
                        compages::Field<float, compages::Grid3D> const& p_volume,
                        FieldLook const& p_look)
{
    FieldView const* view = p_scene.world().tryGet<FieldView>(p_entity);
    compages::Grid3D const& grid = p_volume.grid();
    if ((view == nullptr) || !view->volume.valid())
    {
        return failure("show(entity, volume) on an entity not made by show(volume)");
    }
    if ((view->columns != grid.columns) || (view->rows != grid.rows) || (view->layers != grid.layers))
    {
        return failure("show(entity, volume) with another size of grid");
    }
    COMPAGES_TRY(checkVolume(grid, p_look));
    TextureAsset* values = p_scene.assets().texture(view->volume);
    Material* material = p_scene.assets().material(view->material);
    if ((values == nullptr) || (material == nullptr))
    {
        return failure("show(entity, volume): the entity lost its volume");
    }
    COMPAGES_TRY(values->texture.write(std::as_bytes(p_volume.values())));
    return applyVolumeLook(p_scene.assets(), *material, view->volume, grid, p_look);
}

// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
compages::world::Entity FieldViews::show(Scene& p_scene,
                                         compages::Field<compages::Vector2f, Grid> const& p_field,
                                         ArrowLook const& p_look,
                                         std::string p_name)
{
    auto made = [&]() -> Result<MaterialId>
    {
        COMPAGES_TRY(checkField(p_field.grid(),
                                FieldLook{ .colors = p_look.colors, .low = p_look.low, .high = p_look.high,
                                           .relief = 0.0f, .lit = false }));
        shaders::ShaderPair pbr = shaders::pbr();
        pbr.fragment().hook("main", ARROW_MAIN);
        Material material;
        COMPAGES_TRY_ASSIGN(
            material,
            Material::create(p_name + " arrows",
                             { .family = ShaderFamily::PbrMinimal,
                               .shaders = std::move(pbr),
                               .state = { .depth_test = true, .cull = compages::gpu::CullMode::None },
                               .see_through = std::nullopt }));
        TextureAssetId colors;
        COMPAGES_TRY_ASSIGN(colors, colorMapTexture(p_scene.assets(), p_look.colors));
        material.textures = { { .sampler = "fieldColors", .texture = colors } };
        return p_scene.assets().addMaterial({}, std::move(material));
    }();
    if (!made)
    {
        compages::gpu::reportError(made.error());
        return p_scene.world().entity(std::move(p_name));
    }
    compages::world::Entity entity = p_scene.mesh(arrowsMesh(p_field, p_look), std::move(p_name));
    p_scene.look(entity, made.value());
    entity.get<MeshRenderer>().flags = RenderFlags::NoShadowCaster;
    entity.set(FieldView{ .material = made.value(),
                          .columns = p_field.grid().columns,
                          .rows = p_field.grid().rows });
    return entity;
}

// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
Status FieldViews::show(Scene& p_scene,
                        compages::world::EntityId p_entity,
                        compages::Field<compages::Vector2f, Grid> const& p_field,
                        ArrowLook const& p_look)
{
    FieldView const* view = p_scene.world().tryGet<FieldView>(p_entity);
    if (view == nullptr)
    {
        return failure("show(entity, vectors) on an entity not made by show(vectors)");
    }
    COMPAGES_TRY(checkField(p_field.grid(),
                            FieldLook{ .colors = p_look.colors, .low = p_look.low, .high = p_look.high,
                                       .relief = 0.0f, .lit = false }));
    Material* material = p_scene.assets().material(view->material);
    if (material == nullptr)
    {
        return failure("show(entity, vectors): the entity lost its material");
    }
    TextureAssetId colors;
    COMPAGES_TRY_ASSIGN(colors, colorMapTexture(p_scene.assets(), p_look.colors));
    material->textures = { { .sampler = "fieldColors", .texture = colors } };
    return p_scene.reshape(p_entity, arrowsMesh(p_field, p_look));
}

// ----------------------------------------------------------------------------
Result<compages::gpu::BufferHandle>
FieldViews::values(compages::world::EntityId p_entity, std::span<const float> p_values)
{
    auto found = m_values.find(p_entity);
    if ((found != m_values.end()) && (found->second.count() == p_values.size()))
    {
        found->second.write(p_values);
        return found->second.handle();
    }
    compages::gpu::StorageBuffer<float> buffer;
    COMPAGES_TRY_ASSIGN(buffer,
                        compages::gpu::StorageBuffer<float>::from(
                            p_values, compages::gpu::BufferUsage::Stream));
    const compages::gpu::BufferHandle handle = buffer.handle();
    m_values.insert_or_assign(p_entity, std::move(buffer));
    return handle;
}

// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
compages::world::Entity FieldViews::grid(Scene& p_scene,
                                         Grid const& p_grid,
                                         FieldLook const& p_look,
                                         std::string p_name)
{
    MeshAsset grid = gridMesh(p_grid);
    grid.local_bounds = boundsOf(p_grid, p_look);
    compages::world::Entity entity = p_scene.mesh(std::move(grid), p_name);

    shaders::ShaderPair pbr = shaders::pbr();
    pbr.vertex().hook("deform", FIELD_DEFORM);
    pbr.fragment().hook("main", FIELD_MAIN);
    auto made = Material::create(
        p_name + " field",
        { .family = ShaderFamily::PbrMinimal,
          .shaders = std::move(pbr),
          .state = { .depth_test = true, .cull = compages::gpu::CullMode::None },
          .see_through = std::nullopt });
    Result<MaterialId> added = made ? p_scene.assets().addMaterial({}, made.take())
                                    : Result<MaterialId>(failure(made.error()));
    if (!added)
    {
        compages::gpu::reportError(added.error());
        return entity;
    }
    p_scene.look(entity, added.value());
    entity.set(FieldView{ .material = added.value(),
                          .columns = p_grid.columns,
                          .rows = p_grid.rows });
    return entity;
}

// ----------------------------------------------------------------------------
Status FieldViews::apply(Scene& p_scene,
                        compages::world::EntityId p_entity,
                         compages::gpu::BufferHandle p_values,
                         std::size_t p_count,
                         FieldLook const& p_look)
{
    FieldView const* view = p_scene.world().tryGet<FieldView>(p_entity);
    if (view == nullptr)
    {
        return failure("show(entity, ...) on an entity not made by show()");
    }
    const compages::Grid2D grid{ .columns = view->columns, .rows = view->rows };
    MeshRenderer const* renderer = p_scene.world().tryGet<MeshRenderer>(p_entity);
    MeshAsset* mesh = (renderer == nullptr) ? nullptr : p_scene.assets().mesh(renderer->mesh);
    Material* material = p_scene.assets().material(view->material);
    if ((mesh == nullptr) || (material == nullptr))
    {
        return failure("show(entity, ...): the entity lost its field");
    }
    COMPAGES_TRY(checkField(grid, p_look));
    // The grid of the mesh keeps its place and spacing; only the heights move.
    const compages::Vector3f low = mesh->local_bounds.min;
    const compages::Vector3f high = mesh->local_bounds.max;
    const float a = p_look.relief * p_look.low;
    const float b = p_look.relief * p_look.high;
    mesh->local_bounds = compages::AABB::fromCorners(
        compages::Vector3f(low.x, std::min({ a, b, 0.0f }), low.z),
        compages::Vector3f(high.x, std::max({ a, b, 0.0f }), high.z));
    const float spacing = (high.x - low.x) / float(std::max(view->columns - 1, 1));
    COMPAGES_TRY(applyLook(p_scene.assets(),
                           *material,
                           p_values,
                           p_count,
                           { .columns = view->columns, .rows = view->rows, .spacing = spacing },
                           p_look));
    return success();
}

// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
compages::world::Entity FieldViews::show(Scene& p_scene,
                                         compages::Field<float, Grid> const& p_field,
                                    FieldLook const& p_look,
                                    std::string p_name)
{
    if (const Status valid = checkField(p_field.grid(), p_look); !valid)
    {
        compages::gpu::reportError(valid.error());
        return p_scene.world().entity(std::move(p_name));
    }
    compages::world::Entity entity = grid(p_scene, p_field.grid(), p_look, std::move(p_name));
    if (const Status shown = show(p_scene, entity, p_field, p_look); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    return entity;
}

// ----------------------------------------------------------------------------
template <compages::PlanarGrid Grid>
Status FieldViews::show(Scene& p_scene,
                        compages::world::EntityId p_entity,
                   compages::Field<float, Grid> const& p_field,
                   FieldLook const& p_look)
{
    FieldView const* view = p_scene.world().tryGet<FieldView>(p_entity);
    if ((view != nullptr) &&
        ((view->columns != p_field.grid().columns) || (view->rows != p_field.grid().rows)))
    {
        return failure("show(entity, field) with " + std::to_string(p_field.grid().columns) +
                       " x " + std::to_string(p_field.grid().rows) + " points, made with " +
                       std::to_string(view->columns) + " x " + std::to_string(view->rows));
    }
    if (view == nullptr)
    {
        return failure("show(entity, field) on an entity not made by show()");
    }
    compages::gpu::BufferHandle buffer;
    COMPAGES_TRY_ASSIGN(buffer, values(p_entity, p_field.values()));
    return apply(p_scene, p_entity, buffer, p_field.size(), p_look);
}

// ----------------------------------------------------------------------------
compages::world::Entity FieldViews::show(Scene& p_scene,
                                         compages::Grid2D const& p_grid,
                                    compages::gpu::StorageBuffer<float> const& p_values,
                                    FieldLook const& p_look,
                                    std::string p_name)
{
    if (const Status valid = checkField(p_grid, p_look); !valid)
    {
        compages::gpu::reportError(valid.error());
        return p_scene.world().entity(std::move(p_name));
    }
    compages::world::Entity entity = grid(p_scene, p_grid, p_look, std::move(p_name));
    if (const Status shown = apply(p_scene, entity, p_values.handle(), p_values.count(), p_look);
        !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    return entity;
}

// ----------------------------------------------------------------------------
Status FieldViews::show(Scene& p_scene,
                        compages::world::EntityId p_entity,
                   compages::gpu::StorageBuffer<float> const& p_values,
                   FieldLook const& p_look)
{
    return apply(p_scene, p_entity, p_values.handle(), p_values.count(), p_look);
}

// The grids of the plane a field may be shown on.
template compages::world::Entity FieldViews::show(Scene&, compages::Field<float, compages::Grid2D> const&,
                                                  FieldLook const&, std::string);
template compages::world::Entity FieldViews::show(Scene&, compages::Field<float, compages::CurvilinearGrid> const&,
                                                  FieldLook const&, std::string);
template Status FieldViews::show(Scene&, compages::world::EntityId, compages::Field<float, compages::Grid2D> const&,
                                 FieldLook const&);
template Status FieldViews::show(Scene&, compages::world::EntityId,
                                 compages::Field<float, compages::CurvilinearGrid> const&, FieldLook const&);
template compages::world::Entity FieldViews::show(Scene&, compages::Field<compages::Vector2f, compages::Grid2D> const&,
                                                  ArrowLook const&, std::string);
template compages::world::Entity FieldViews::show(Scene&,
                                                  compages::Field<compages::Vector2f, compages::CurvilinearGrid> const&,
                                                  ArrowLook const&, std::string);
template Status FieldViews::show(Scene&, compages::world::EntityId,
                                 compages::Field<compages::Vector2f, compages::Grid2D> const&, ArrowLook const&);
template Status FieldViews::show(Scene&, compages::world::EntityId,
                                 compages::Field<compages::Vector2f, compages::CurvilinearGrid> const&, ArrowLook const&);

// ----------------------------------------------------------------------------
std::optional<FieldProbe> probeField(Scene const& p_scene,
                                     compages::Field<float> const& p_field,
                                     FieldLook const& p_look,
                                     compages::Vector2f p_pixel)
{
    CameraFrame const& camera = p_scene.lastCamera();
    compages::Grid2D const& grid = p_field.grid();
    if ((camera.viewport_width == 0u) || (camera.viewport_height == 0u) ||
        (grid.columns < 2) || (grid.rows < 2))
    {
        return std::nullopt;
    }
    const compages::Ray ray = camera.screenRay(
        p_pixel.x, p_pixel.y, camera.viewport_width, camera.viewport_height);
    const float low = std::min(p_look.low, p_look.high);
    const float high = std::max(p_look.low, p_look.high);
    // As the vertex hook draws it: the relief stops at the ends of the range;
    // no ground outside the grid.
    auto ground = [&](float p_x, float p_z)
    {
        const compages::Vector2f at = grid.coordinates(compages::Vector2f(p_x, p_z));
        const bool inside = (at.x >= 0.0f) && (at.y >= 0.0f) &&
                            (at.x <= float(grid.columns - 1)) && (at.y <= float(grid.rows - 1));
        return inside ? p_look.relief * std::clamp(p_field.sample(compages::Vector2f(p_x, p_z)), low, high)
                      : -std::numeric_limits<float>::infinity();
    };
    const float top = std::max({ p_look.relief * low, p_look.relief * high, 0.0f });
    const float bottom = std::min({ p_look.relief * low, p_look.relief * high, 0.0f });
    if (std::abs(ray.direction.y) < 1.0e-6f)
    {
        return std::nullopt;
    }
    const float a = (top - ray.origin.y) / ray.direction.y;
    const float b = (bottom - ray.origin.y) / ray.direction.y;
    const float step = 0.5f * grid.spacing;
    const std::optional<float> t = compages::intersectHeight(
        ray, ground, std::max(0.0f, std::min(a, b)), std::max(a, b) + step, step);
    if (!t)
    {
        return std::nullopt;
    }
    const compages::Vector3f point = ray.pointAt(*t);
    return FieldProbe{ .point = point,
                       .value = p_field.sample(compages::Vector2f(point.x, point.z)) };
}

} // namespace compages::renderer
