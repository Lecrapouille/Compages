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

namespace examples
{

// ****************************************************************************
//! \brief Six pictures around the scene, as far away as the sky.
//!
//! A skybox is a cube map drawn around the camera before anything else. It
//! turns with the camera but never comes closer, which is what makes it look
//! infinitely far:
//! \code
//! m_scene.skybox({ dataPath("right.jpg"), dataPath("left.jpg"),
//!                  dataPath("top.jpg"), dataPath("bottom.jpg"),
//!                  dataPath("front.jpg"), dataPath("back.jpg") });
//! \endcode
// ****************************************************************************
class Skybox final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "37_Skybox";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_cube;
};

} // namespace examples
