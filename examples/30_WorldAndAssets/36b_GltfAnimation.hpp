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
