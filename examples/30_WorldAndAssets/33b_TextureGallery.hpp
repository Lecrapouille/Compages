// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

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
//! m_scene.box("rocks", path.empty() ? compages::renderer::color(0.55f, 0.52f,
//! 0.48f)
//!                                   : compages::renderer::texture(path));
//! \endcode
// ****************************************************************************
class TextureGallery final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "33b_TextureGallery";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<compages::world::Entity> m_boxes;
};

} // namespace examples
