//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

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
        return texture.generateMipmaps();
    }
};

} // namespace compages::renderer
