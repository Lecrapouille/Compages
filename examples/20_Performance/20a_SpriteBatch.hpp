// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A hundred thousand sprites, one draw call.
//!
//! The corners of a quad are four vertices. A hundred thousand quads used to
//! be a hundred thousand draws, or four hundred thousand vertices rebuilt
//! whenever one sprite moved. Instancing is the third way: the shader makes
//! the four corners from gl_VertexID, and each record of the buffer is one
//! sprite, read once per instance:
//! \code
//! m_sprites.vertices(sprites,
//! compages::gpu::VertexLayout::of<Sprite>().perInstance());
//! m_sprites.drawInstanced(m_sprites.count(), 4u);   // 4 corners per sprite
//! \endcode
//!
//! The atlas is four by four tiles computed here. Each sprite names a tile;
//! the fragment shader reads it. One texture, one draw, a hundred thousand
//! quads.
// ****************************************************************************
class SpriteBatch: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "20a_SpriteBatch";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Sprite
    {
        compages::core::Vector2f center;
        compages::core::Vector2f extent;
        float tile = 0.0f;
        float phase = 0.0f;
    };

    [[nodiscard]] compages::Status makeAtlas();

    compages::gpu::Texture m_atlas;
    compages::gpu::Drawable m_sprites;
};

} // namespace examples
