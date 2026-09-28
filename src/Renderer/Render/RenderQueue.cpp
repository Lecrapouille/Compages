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

#include "Compages/Renderer/Render/RenderQueue.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"

#include <algorithm>

namespace compages::renderer
{

//------------------------------------------------------------------------------
void RenderQueue::build(RenderSnapshot const& p_snapshot,
                        compages::renderer::AssetManager const& p_assets)
{
    m_entries.clear();
    m_entries.reserve(p_snapshot.items.size());

    for (std::size_t i = 0u; i < p_snapshot.items.size(); ++i)
    {
        RenderItem const& item = p_snapshot.items[i];

        compages::renderer::MaterialInstance const* instance =
            p_assets.materialInstance(item.material_instance);
        if (instance == nullptr)
        {
            continue;
        }
        if (p_assets.material(instance->material) == nullptr)
        {
            continue;
        }
        if (p_assets.mesh(item.mesh) == nullptr)
        {
            continue;
        }

        // The high 32 bits sort by material family, the low 32 bits by mesh.
        // Items with the same material end up together; among those, items
        // with the same mesh end up together.
        const std::uint64_t material_bits =
            static_cast<std::uint64_t>(instance->material.bits());
        const std::uint64_t mesh_bits =
            static_cast<std::uint64_t>(item.mesh.bits());

        QueueEntry entry;
        entry.key = (material_bits << 32u) | mesh_bits;
        entry.item_index = static_cast<std::uint32_t>(i);
        m_entries.emplace_back(entry);
    }

    std::sort(m_entries.begin(), m_entries.end(),
              [](QueueEntry const& p_a, QueueEntry const& p_b) {
                  return p_a.key < p_b.key;
              });
}

} // namespace compages::renderer
