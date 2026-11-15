// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief A texture owned by the AssetManager.
//!
//! compages::world::World and Scene never hold a \c compages::gpu::Texture
//! directly: they store a TextureAssetId and ask the manager when a draw needs
//! a binding.
//!
//! Make one when a texture comes from your program (a noise, a field, a
//! picture computed once) and must be read by a material or a look. The
//! pixels may wait on the CPU until upload(), so a scene can be built before
//! a GPU exists.
//!
//! \code
//! compages::renderer::TextureAsset flow;
//! flow.name = "Flow map";
//! COMPAGES_TRY_ASSIGN(flow.texture, compages::gpu::Texture::fromField(field));
//! COMPAGES_TRY_ASSIGN(id, scene.assets().addTexture(flow.name, std::move(flow)));
//! \endcode
// ****************************************************************************
struct TextureAsset
{
    //! \brief A short label, used in logs and when deduplicating imports.
    std::string name;
    //! \brief The device image.
    compages::gpu::Texture texture;
    compages::gpu::TextureDesc description;
    std::vector<std::byte> pixels;

    [[nodiscard]] Status upload()
    {
        if (texture.valid())
        {
            return success();
        }
        if (pixels.empty())
        {
            return failure("texture has no CPU pixels to upload");
        }
        COMPAGES_TRY_ASSIGN(texture,
                            compages::gpu::Texture::create(description));
        COMPAGES_TRY(texture.write(pixels));
        // A picture of one pixel, or made with one level, has none to build.
        return (texture.levels() > 1u) ? texture.generateMipmaps() : success();
    }
};

} // namespace compages::renderer
