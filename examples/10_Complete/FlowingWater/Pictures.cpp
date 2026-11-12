// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/FlowingWater/Pictures.hpp"

#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <span>
#include <utility>

namespace examples::flowing
{

namespace
{

using Rgb = std::array<float, 3>;

// ----------------------------------------------------------------------------
//! \brief A number from 0 to 1 for a corner (x, y) of the lattice.
// ----------------------------------------------------------------------------
float hash(int p_x, int p_y, std::uint32_t p_seed)
{
    std::uint32_t h = (std::uint32_t(p_x) * 73856093u) ^ (std::uint32_t(p_y) * 19349663u) ^ (p_seed * 83492791u);
    h ^= h >> 13u;
    h *= 0x5bd1e995u;
    h ^= h >> 15u;
    return float(h & 0xffffffu) / float(0xffffff);
}

int wrap(int p_value, int p_period)
{
    return ((p_value % p_period) + p_period) % p_period;
}

float smooth(float p_t)
{
    return p_t * p_t * (3.0f - (2.0f * p_t));
}

float smoothstep(float p_from, float p_to, float p_x)
{
    return smooth(std::clamp((p_x - p_from) / (p_to - p_from), 0.0f, 1.0f));
}

// ----------------------------------------------------------------------------
//! \brief Distance to the nearest of random points, one per cell of a lattice
//! that repeats every \c p_period cells (Worley noise): small near a point.
// ----------------------------------------------------------------------------
float cells(float p_x, float p_y, int p_period, std::uint32_t p_seed)
{
    const int ix = int(std::floor(p_x));
    const int iy = int(std::floor(p_y));
    float nearest = 2.0f;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            const int cx = wrap(ix + dx, p_period);
            const int cy = wrap(iy + dy, p_period);
            const float px = float(ix + dx) + hash(cx, cy, p_seed);
            const float py = float(iy + dy) + hash(cx, cy, p_seed + 1u);
            nearest = std::min(nearest, std::hypot(px - p_x, py - p_y));
        }
    }
    return nearest;
}

// ----------------------------------------------------------------------------
//! \brief A picture of \c p_size pixels, each given by a function of (u, v)
//! from 0 to 1, as linear values from 0 to 1 and an opacity.
// ----------------------------------------------------------------------------
Picture paint(int p_size, std::function<std::array<float, 4>(float, float)> const& p_pixel)
{
    Picture picture{ .size = p_size, .rgba = std::vector<std::uint8_t>(std::size_t(p_size * p_size * 4)) };
    for (int y = 0; y < p_size; ++y)
    {
        for (int x = 0; x < p_size; ++x)
        {
            const std::array<float, 4> c = p_pixel(float(x) / float(p_size), float(y) / float(p_size));
            const std::size_t at = std::size_t((y * p_size) + x) * 4u;
            for (std::size_t k = 0u; k < 4u; ++k)
            {
                picture.rgba[at + k] = std::uint8_t(std::lround(std::clamp(c[k], 0.0f, 1.0f) * 255.0f));
            }
        }
    }
    return picture;
}

Rgb mix(Rgb const& p_a, Rgb const& p_b, float p_t)
{
    return { p_a[0] + ((p_b[0] - p_a[0]) * p_t), p_a[1] + ((p_b[1] - p_a[1]) * p_t),
             p_a[2] + ((p_b[2] - p_a[2]) * p_t) };
}

Rgb times(Rgb const& p_a, float p_k)
{
    return { p_a[0] * p_k, p_a[1] * p_k, p_a[2] * p_k };
}

} // namespace

// ----------------------------------------------------------------------------
float tiledNoise(float p_x, float p_y, int p_period, std::uint32_t p_seed)
{
    const int ix = int(std::floor(p_x));
    const int iy = int(std::floor(p_y));
    const float sx = smooth(p_x - float(ix));
    const float sy = smooth(p_y - float(iy));
    const int x0 = wrap(ix, p_period);
    const int y0 = wrap(iy, p_period);
    const int x1 = wrap(ix + 1, p_period);
    const int y1 = wrap(iy + 1, p_period);
    const float bottom = hash(x0, y0, p_seed) + ((hash(x1, y0, p_seed) - hash(x0, y0, p_seed)) * sx);
    const float top = hash(x0, y1, p_seed) + ((hash(x1, y1, p_seed) - hash(x0, y1, p_seed)) * sx);
    return bottom + ((top - bottom) * sy);
}

// ----------------------------------------------------------------------------
float tiledFbm(float p_x, float p_y, int p_period, int p_octaves, std::uint32_t p_seed)
{
    float sum = 0.0f;
    float weight = 0.0f;
    float amplitude = 1.0f;
    float scale = 1.0f;
    for (int octave = 0; octave < p_octaves; ++octave)
    {
        sum += amplitude * tiledNoise(p_x * scale, p_y * scale, p_period * int(scale), p_seed + std::uint32_t(octave));
        weight += amplitude;
        amplitude *= 0.5f;
        scale *= 2.0f;
    }
    return sum / weight;
}

// ----------------------------------------------------------------------------
Picture noisePicture()
{
    return paint(256, [](float u, float v) -> std::array<float, 4> {
        // Stretched to its full range: the phases of the pixels spread over
        // the whole cycle, so the surface never resets everywhere at once.
        const float phase = smoothstep(0.25f, 0.75f, tiledFbm(u * 6.0f, v * 6.0f, 6, 4, 11u));
        const float gather = tiledFbm(u * 3.0f, v * 3.0f, 3, 3, 12u);
        return { phase, gather, 0.0f, 1.0f };
    });
}

// ----------------------------------------------------------------------------
//! The heights of the ripples, then their slopes by central differences (the
//! neighbours wrap round, so the normals tile too).
// ----------------------------------------------------------------------------
Picture ripplePicture()
{
    constexpr int SIZE = 256;
    std::vector<float> height(std::size_t(SIZE * SIZE));
    for (int y = 0; y < SIZE; ++y)
    {
        for (int x = 0; x < SIZE; ++x)
        {
            const float u = float(x) / float(SIZE);
            const float v = float(y) / float(SIZE);
            const float waves = tiledFbm(u * 8.0f, v * 8.0f, 8, 5, 21u);
            const float drops = cells(u * 12.0f, v * 12.0f, 12, 22u);
            height[std::size_t((y * SIZE) + x)] = waves + (0.25f * std::sqrt(drops));
        }
    }
    auto at = [&](int x, int y) { return height[std::size_t((wrap(y, SIZE) * SIZE) + wrap(x, SIZE))]; };
    return paint(SIZE, [&](float u, float v) -> std::array<float, 4> {
        const int x = int(u * float(SIZE));
        const int y = int(v * float(SIZE));
        const float steep = 18.0f;
        float nx = -(at(x + 1, y) - at(x - 1, y)) * steep;
        float ny = -(at(x, y + 1) - at(x, y - 1)) * steep;
        const float length = std::sqrt((nx * nx) + (ny * ny) + 1.0f);
        nx /= length;
        ny /= length;
        return { (nx * 0.5f) + 0.5f, (ny * 0.5f) + 0.5f, ((1.0f / length) * 0.5f) + 0.5f, 1.0f };
    });
}

// ----------------------------------------------------------------------------
Picture foamPicture()
{
    return paint(512, [](float u, float v) -> std::array<float, 4> {
        const float patches = smoothstep(0.48f, 0.68f, tiledFbm(u * 5.0f, v * 5.0f, 5, 5, 31u));
        const float bubble = cells(u * 40.0f, v * 40.0f, 40, 32u);
        const float rims = smoothstep(0.25f, 0.42f, bubble) * (1.0f - smoothstep(0.42f, 0.55f, bubble));
        const float lace = smoothstep(0.55f, 0.75f, tiledFbm(u * 14.0f, v * 14.0f, 14, 3, 33u));
        const float foam = std::clamp((patches * (0.55f + (0.45f * rims))) + (0.35f * lace * rims), 0.0f, 1.0f);
        const Rgb clean{ 0.80f, 0.78f, 0.66f };
        const Rgb grime{ 0.48f, 0.44f, 0.30f };
        const Rgb c = mix(grime, clean, tiledFbm(u * 9.0f, v * 9.0f, 9, 3, 34u));
        return { c[0], c[1], c[2], foam };
    });
}

// ----------------------------------------------------------------------------
//! Two panels of formwork per tile: their joints, and the four holes of the
//! ties near the corners of each (the mark of cast-in-place concrete).
// ----------------------------------------------------------------------------
Picture concretePicture()
{
    return paint(512, [](float u, float v) -> std::array<float, 4> {
        float grey = 0.46f * (0.82f + (0.36f * tiledFbm(u * 16.0f, v * 16.0f, 16, 5, 41u)));
        grey *= 0.78f + (0.22f * tiledFbm(u * 3.0f, v * 3.0f, 3, 3, 42u));
        const float pits = cells(u * 64.0f, v * 64.0f, 64, 43u);
        grey *= 0.75f + (0.25f * smoothstep(0.03f, 0.12f, pits));
        const float pu = std::fmod(u * 2.0f, 1.0f);
        const float pv = std::fmod(v * 2.0f, 1.0f);
        const float joint = std::min(std::min(pu, 1.0f - pu), std::min(pv, 1.0f - pv));
        grey *= 0.6f + (0.4f * smoothstep(0.002f, 0.008f, joint));
        const float tie = std::hypot(std::min(pu, 1.0f - pu) - 0.12f, std::min(pv, 1.0f - pv) - 0.12f);
        grey *= 0.45f + (0.55f * smoothstep(0.008f, 0.016f, tie));
        const float stain = tiledFbm(u * 4.0f, v * 4.0f, 4, 4, 44u);
        const Rgb c = mix({ grey, grey * 0.99f, grey * 0.96f }, { grey * 0.8f, grey * 0.78f, grey * 0.62f },
                          smoothstep(0.5f, 0.75f, stain));
        return { c[0], c[1], c[2], 1.0f };
    });
}

// ----------------------------------------------------------------------------
Picture paintedMetalPicture()
{
    return paint(256, [](float u, float v) -> std::array<float, 4> {
        const Rgb paint{ 0.24f, 0.31f, 0.27f };
        const float wear = tiledFbm(u * 6.0f, v * 6.0f, 6, 5, 51u);
        const float fine = tiledFbm(u * 32.0f, v * 32.0f, 32, 3, 52u);
        const Rgb rust = times({ 0.40f, 0.19f, 0.08f }, 0.6f + (0.7f * fine));
        Rgb c = mix(times(paint, 0.85f + (0.3f * fine)), rust, smoothstep(0.56f, 0.68f, wear));
        // A thin bright edge of bare metal where the paint flakes.
        const float edge = smoothstep(0.53f, 0.56f, wear) * (1.0f - smoothstep(0.56f, 0.59f, wear));
        c = mix(c, { 0.5f, 0.5f, 0.48f }, 0.5f * edge);
        return { c[0], c[1], c[2], 1.0f };
    });
}

// ----------------------------------------------------------------------------
Picture hazardPicture()
{
    return paint(256, [](float u, float v) -> std::array<float, 4> {
        const bool yellow = std::fmod((u + v) * 4.0f, 1.0f) < 0.5f;
        Rgb c = yellow ? Rgb{ 0.85f, 0.62f, 0.05f } : Rgb{ 0.05f, 0.05f, 0.05f };
        c = times(c, 0.7f + (0.3f * tiledFbm(u * 12.0f, v * 12.0f, 12, 4, 61u)));
        c = mix(c, { 0.30f, 0.18f, 0.08f }, 0.8f * smoothstep(0.62f, 0.75f, tiledFbm(u * 5.0f, v * 5.0f, 5, 4, 62u)));
        return { c[0], c[1], c[2], 1.0f };
    });
}

// ----------------------------------------------------------------------------
compages::Result<compages::renderer::TextureAssetId>
addPicture(compages::renderer::Scene& p_scene, std::string const& p_name, Picture p_picture, bool p_srgb)
{
    compages::gpu::TextureDesc desc =
        compages::gpu::TextureDesc::image(std::uint32_t(p_picture.size), std::uint32_t(p_picture.size),
                                          p_srgb ? compages::gpu::PixelFormat::SRGB8A8
                                                 : compages::gpu::PixelFormat::RGBA8)
            .wrap(compages::gpu::Wrap::Repeat);
    desc.levels = 0u;
    compages::renderer::TextureAsset asset;
    asset.name = p_name;
    COMPAGES_TRY_ASSIGN(asset.texture, compages::gpu::Texture::create(desc));
    COMPAGES_TRY(asset.texture.write(std::as_bytes(std::span(p_picture.rgba))));
    COMPAGES_TRY(asset.texture.generateMipmaps());
    asset.description = desc;
    return p_scene.assets().addTexture(p_name, std::move(asset));
}

} // namespace examples::flowing
