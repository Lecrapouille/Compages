// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/NotCopiable.hpp"
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
//! See-through items (opacity below one) come after all the solid ones,
//! sorted from the farthest to the nearest: each one is blended over what is
//! already behind it, which only works when what is behind was drawn first.
// ****************************************************************************
struct QueueEntry
{
    std::uint64_t key = 0u;
    std::uint32_t item_index = 0u;
    //! \brief Drawn blended, after the solid items.
    bool see_through = false;
    //! \brief Squared distance from the camera to the centre of the item:
    //! the order of the see-through items.
    float distance2 = 0.0f;
};

// ****************************************************************************
//! \brief The list of items the Renderer draws in order.
// ****************************************************************************
class RenderQueue: private compages::NotCopiable
{
public:

    RenderQueue() = default;
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
