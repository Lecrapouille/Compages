// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/DoomLike/Materials.hpp"

#include "Common/DataPath.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"

#include <array>
#include <utility>

namespace examples::doom
{

namespace
{

// The hook "main" of the PBR shader. The normal map is read along each axis
// and its bumps added to the normal of the face ("whiteout" blend, after
// B. Golus, "Normal Mapping for a Triplanar Shader", 2017). Then the
// metallic-roughness model of the library (shadePbr): the lamps glint
// sharper where the roughness map says smooth (worn metal shines, concrete
// does not).
constexpr char const* SURFACE_MAIN = R"(
uniform sampler2D surfaceMap;
uniform sampler2D normalMap;
uniform sampler2D roughMap;
uniform float tileMetres;
uniform float metallic;

void main()
{
    vec3 n = normalize(vNormal);
    vec3 w = pow(abs(n), vec3(4.0));
    w /= (w.x + w.y + w.z);
    vec3 p = vWorldPos / tileMetres;
    vec3 base = (texture(surfaceMap, p.zy).rgb * w.x + texture(surfaceMap, p.xz).rgb * w.y
               + texture(surfaceMap, p.xy).rgb * w.z) * baseColorFactor;
    float rough = texture(roughMap, p.zy).r * w.x + texture(roughMap, p.xz).r * w.y
                + texture(roughMap, p.xy).r * w.z;
    vec3 tx = texture(normalMap, p.zy).xyz * 2.0 - 1.0;
    vec3 ty = texture(normalMap, p.xz).xyz * 2.0 - 1.0;
    vec3 tz = texture(normalMap, p.xy).xyz * 2.0 - 1.0;
    tx = vec3(tx.xy + n.zy, abs(tx.z) * n.x);
    ty = vec3(ty.xy + n.xz, abs(ty.z) * n.y);
    tz = vec3(tz.xy + n.xy, abs(tz.z) * n.z);
    vec3 bumped = normalize(tx.zyx * w.x + ty.xzy * w.y + tz.xyz * w.z);
    oColor = vec4(present(applyFog(shadePbr(base, bumped, vWorldPos, metallic, rough), vWorldPos)), opacity);
}
)";

// ----------------------------------------------------------------------------
compages::Result<compages::renderer::TextureAssetId>
picture(compages::renderer::AssetManager& p_assets, std::string const& p_file, bool p_srgb)
{
    const std::filesystem::path path = dataPath("polyhaven/textures/" + p_file);
    if (path.empty())
    {
        return compages::failure("polyhaven/textures/" + p_file + " is missing from external/Compages-data/");
    }
    compages::gpu::LoadOptions options;
    options.srgb = p_srgb;
    options.wrap = compages::gpu::Wrap::Repeat;
    compages::renderer::TextureAsset asset;
    asset.name = p_file;
    COMPAGES_TRY_ASSIGN(asset.texture, compages::gpu::Texture::fromFile(path, options));
    return p_assets.addTexture(p_file, std::move(asset));
}

} // namespace

// ----------------------------------------------------------------------------
compages::Result<compages::renderer::MaterialId>
makeSurface(compages::renderer::AssetManager& p_assets, std::string const& p_name, float p_tile, float p_metallic)
{
    const std::string stem = p_name + "/" + p_name;
    compages::renderer::TextureAssetId colour;
    compages::renderer::TextureAssetId normals;
    compages::renderer::TextureAssetId roughness;
    COMPAGES_TRY_ASSIGN(colour, picture(p_assets, stem + "_diff_1k.jpg", true));
    COMPAGES_TRY_ASSIGN(normals, picture(p_assets, stem + "_nor_gl_1k.jpg", false));
    COMPAGES_TRY_ASSIGN(roughness, picture(p_assets, stem + "_rough_1k.jpg", false));

    compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
    shaders.fragment().hook("main", SURFACE_MAIN);
    compages::renderer::Material material;
    COMPAGES_TRY_ASSIGN(material,
                        compages::renderer::Material::create(
                            p_name, { .family = compages::renderer::ShaderFamily::PbrMinimal,
                                      .shaders = std::move(shaders),
                                      .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
                                      .see_through = std::nullopt }));
    material.textures = { { .sampler = "surfaceMap", .texture = colour },
                          { .sampler = "normalMap", .texture = normals },
                          { .sampler = "roughMap", .texture = roughness } };
    material.program.set("tileMetres", p_tile);
    material.program.set("metallic", p_metallic);
    return p_assets.addMaterial(p_name, std::move(material));
}

} // namespace examples::doom
