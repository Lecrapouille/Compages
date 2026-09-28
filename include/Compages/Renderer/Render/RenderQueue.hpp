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

#include "Compages/Renderer/Render/RenderSnapshot.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace compages::renderer
{

class AssetManager;

// ****************************************************************************
//! \brief One entry in the sorted list a Renderer walks.
//!
//! The key is what sorts the entry, the index points back into the snapshot's
//! item array. The key is 64 bits packed as:
//! \code
//! [material_id 32][mesh_id 32]
//! \endcode
//! so items with the same material land next to each other, and among those
//! items with the same mesh land next to each other. That is what lets the
//! renderer bind the pipeline once per material and the buffers once per
//! mesh.
//!
//! Depth is not part of the key at this stage: the demos have very few
//! objects and no transparency. The layout of the bits leaves room for a
//! depth bucket later without changing the callers.
// ****************************************************************************
struct QueueEntry
{
    std::uint64_t key = 0u;
    std::uint32_t item_index = 0u;
};

// ****************************************************************************
//! \brief The list of items the Renderer draws in order.
// ****************************************************************************
class RenderQueue
{
public:

    RenderQueue() = default;
    RenderQueue(RenderQueue const&) = delete;
    RenderQueue& operator=(RenderQueue const&) = delete;
    RenderQueue(RenderQueue&&) = default;
    RenderQueue& operator=(RenderQueue&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Fill the queue from a snapshot and sort it.
    //!
    //! Items pointing at ids the AssetManager no longer knows are skipped.
    //! The queue is emptied and rebuilt: it does not accumulate across frames.
    // ------------------------------------------------------------------------
    void build(RenderSnapshot const& p_snapshot, AssetManager const& p_assets);

    [[nodiscard]] std::vector<QueueEntry> const& entries() const
    {
        return m_entries;
    }

    [[nodiscard]] bool empty() const { return m_entries.empty(); }
    [[nodiscard]] std::size_t size() const { return m_entries.size(); }

    void clear() { m_entries.clear(); }

private:

    std::vector<QueueEntry> m_entries;
};

} // namespace compages::renderer
