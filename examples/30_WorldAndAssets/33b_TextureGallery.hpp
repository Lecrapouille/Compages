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

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/Renderer/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Nine pictures of the data repository, one per turning box.
//!
//! A picture that is not found falls back to a plain colour, which is how
//! the example still runs with only part of the data checked out:
//! \code
//! const std::string path = dataPath("rocks.png");
//! m_scene.box("rocks", path.empty() ? compages::renderer::color(0.55f, 0.52f, 0.48f)
//!                                   : compages::renderer::texture(path));
//! \endcode
// ****************************************************************************
class TextureGallery final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "33b_TextureGallery";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<compages::world::Entity> m_boxes;
};

} // namespace examples
