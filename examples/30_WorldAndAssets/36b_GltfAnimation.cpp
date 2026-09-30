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

#include "30_WorldAndAssets/36b_GltfAnimation.hpp"
#include "Common/DataPath.hpp"

#include "Common/Gui.hpp"

namespace examples
{

//------------------------------------------------------------------------------
std::string GltfAnimation::description() const
{
    return "Soldier.glb and the clips it came with. Choose one in the Try it "
           "panel, or press "
           "1 to idle, 2 to walk, 3 to run. Right drag turns around it. Needs "
           "Soldier.glb in external/Compages-data/.";
}

//------------------------------------------------------------------------------
compages::Status GltfAnimation::setUp()
{
    // The clips live in the file. Walk is the one the example starts on.
    const std::string path = dataPath("Soldier.glb");
    if (path.empty())
    {
        return compages::failure("Soldier.glb is missing: run make download in "
                                 "external/, or set COMPAGES_DATA_PATH");
    }
    m_scene.background(0.10f, 0.12f, 0.15f);
    COMPAGES_TRY_ASSIGN(m_soldier, m_scene.load(path));
    // The model looks down -Z, away from a camera placed in front: half a turn
    // shows its face.
    m_soldier.rotation(3.14159265f, compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    if (!m_scene.play(m_soldier, "Walk"))
    {
        return compages::failure("Soldier.glb has no Walk clip");
    }
    m_clips = m_scene.clips(m_soldier);
    const compages::core::Vector3f middle = m_scene.frameAll();
    m_scene.activeCamera().add<compages::world::Orbit>(middle);
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void GltfAnimation::draw(compages::world::ViewFrame const& p_frame)
{
    // play() is cheap to call every frame: the same clip keeps playing.
    if (p_frame.input.down(compages::world::Key::D1))
    {
        m_scene.play(m_soldier, "Idle");
    }
    else if (p_frame.input.down(compages::world::Key::D2))
    {
        m_scene.play(m_soldier, "Walk");
    }
    else if (p_frame.input.down(compages::world::Key::D3))
    {
        m_scene.play(m_soldier, "Run");
    }
    m_scene.draw(p_frame);
}

//------------------------------------------------------------------------------
void GltfAnimation::controls()
{
    // One button per clip of the file, the one playing highlighted.
    const std::string current = m_scene.playing(m_soldier);
    for (std::string const& clip : m_clips)
    {
        if (ImGui::RadioButton(clip.c_str(), clip == current))
        {
            m_scene.play(m_soldier, clip);
        }
    }
}

} // namespace examples
