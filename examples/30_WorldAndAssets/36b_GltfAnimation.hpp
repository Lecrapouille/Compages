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
//! \brief A skinned glTF soldier, and the clips it came with.
//!
//! A model loaded with its animations plays the first one; play() changes to
//! another by the name it has in the file:
//! \code
//! COMPAGES_TRY_ASSIGN(m_soldier, m_scene.load(dataPath("Soldier.glb")));
//! ...
//! if (p_frame.input.down(compages::world::Key::D2)) m_scene.play(m_soldier,
//! "Walk");
//! \endcode
//! The "Try it" panel lists every clip of the file, from
//! m_scene.clips(m_soldier), and plays the one chosen.
//!
//! Each frame, the Scene samples the clip onto the joints, then gives the
//! joint matrices to the shader, which bends the mesh.
// ****************************************************************************
class GltfAnimation final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "36b_GltfAnimation";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_soldier;
    //! \brief The clips of the file, read once.
    std::vector<std::string> m_clips;
};

} // namespace examples
