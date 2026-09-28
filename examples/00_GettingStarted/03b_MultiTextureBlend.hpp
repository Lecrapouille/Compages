// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"

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
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;
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
