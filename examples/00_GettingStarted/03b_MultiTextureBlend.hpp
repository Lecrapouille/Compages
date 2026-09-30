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

#include "Common/Example.hpp"

#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace examples
{

class MultiTextureBlend: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "03b_MultiTextureBlend";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief Which channel of the blend map the brush writes to.
    enum class PaintLayer
    {
        Mud,
        Flowers,
        Path,
        Erase,
    };

    void paintAt(float p_u, float p_v);
    void uploadBlendMapIfNeeded();

    //! \brief The blend map, then the four materials it mixes. Declared before
    //! the drawable reading them, so that they are destroyed after it.
    std::array<compages::gpu::Texture, 5u> m_textures;
    compages::gpu::Drawable m_plane;
    std::vector<std::uint8_t> m_blend_pixels;
    std::uint32_t m_blend_width = 0u;
    std::uint32_t m_blend_height = 0u;
    PaintLayer m_paint_layer = PaintLayer::Mud;
    bool m_blend_dirty = false;
    float m_brush = 14.0f;
};

} // namespace examples
