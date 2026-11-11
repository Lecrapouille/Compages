// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Renderer/Render/MaterialResources.hpp"
#include "Renderer/Render/Offer.hpp"

#include <string>

namespace compages::renderer
{

//! \brief Bind the textures of a material, set their samplers, bind its
//! storage buffers.
Status bindMaterialResources(compages::renderer::Material& p_material,
                            compages::renderer::AssetManager& p_assets)
{
    if (p_material.textures.size() > compages::renderer::Material::MAX_TEXTURES)
    {
        return failure("the material '" + p_material.name + "' has " +
                       std::to_string(p_material.textures.size()) +
                       " textures, more than the " +
                       std::to_string(compages::renderer::Material::MAX_TEXTURES) +
                       " units the Renderer leaves free");
    }
    std::uint32_t unit = compages::renderer::Material::FIRST_TEXTURE_UNIT;
    for (compages::renderer::MaterialTexture const& texture : p_material.textures)
    {
        COMPAGES_TRY(p_assets.prepare(texture.texture));
        compages::renderer::TextureAsset const* asset =
            p_assets.texture(texture.texture);
        if (asset == nullptr)
        {
            return failure("the material '" + p_material.name +
                           "' lost its texture '" + texture.sampler + "'");
        }
        asset->texture.bind(unit);
        offer(p_material.program, texture.sampler, int(unit));
        ++unit;
    }
    for (compages::renderer::MaterialBuffer const& buffer : p_material.buffers)
    {
        COMPAGES_TRY(p_material.program.bind(buffer.block, buffer.buffer));
    }
    return success();
}

} // namespace compages::renderer
