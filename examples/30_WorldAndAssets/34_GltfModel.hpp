// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/Renderer/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A glTF model loaded, placed and framed in three lines.
//!
//! load() reads the file once into a prefab, and places it in the World.
//! frameAll() puts a camera and a sun where they see it whole, whatever its
//! size:
//! \code
//! COMPAGES_TRY(m_scene.load(dataPath("Duck.glb")));
//! const Vector3f middle = m_scene.frameAll();
//! m_scene.activeCamera().add<compages::world::Orbit>(middle);   // turn around it
//! \endcode
// ****************************************************************************
class GltfModel final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "34_GltfModel";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
