// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Job.hpp"
#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

#include <cstdint>
#include <vector>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief The fine tiles near the camera (8 m a quad, 2 m a pixel): made by
//! two threads, they take the place of the coarse tiles under them; a skirt
//! along their edges hides the cracks with the coarse ones. At most twelve
//! live at once; the one unused for the longest is made again elsewhere.
//! The heights come from one function of the position (Land::heightAt), so
//! a coarse and a fine tile agree.
// ****************************************************************************
class FineTiles
{
public:

    //! \brief Wait for the tiles being made, and hide them all: the land
    //! they read is going away.
    void drop();

    //! \brief Choose the tiles around what the camera looks at, start the
    //! missing ones, show those that are ready instead of the coarse ones.
    //! \param[in] p_land Read by the threads until drop().
    //! \param[in] p_on Fine tiles wanted at all.
    void update(compages::renderer::Scene& p_scene,
                Land const& p_land,
                compages::renderer::MaterialId p_material,
                compages::Vector3f p_target,
                float p_distance,
                bool p_on,
                std::vector<compages::world::Entity>& p_coarse);

    //! \brief How many are shown.
    [[nodiscard]] std::size_t shown() const;

    //! \brief The ground as the screen shows it at a point: the fine tile
    //! shown there, else the coarse patch \p p_base (bilinear between their
    //! corners). What an overlay draped on the ground must stay above.
    [[nodiscard]] float
    groundAt(Patch const& p_base, float p_x, float p_z) const;

    //! \brief The tiles shown fine, one bit per tile of the map (TILES x
    //! TILES = 64): an overlay draped on the ground is made again when the
    //! bits under it change.
    [[nodiscard]] std::uint64_t shownMask() const
    {
        return m_shown_mask;
    }

    //! \brief The bit of the tile under a point (0 outside the map).
    [[nodiscard]] static std::uint64_t bitAt(float p_x, float p_z);

    //! \brief The side of a quad of a fine tile (about 8 m).
    [[nodiscard]] static float step();

private:

    //! \brief A fine tile: its mesh and picture, made in the background,
    //! then sent to the GPU.
    struct Detail
    {
        //! \brief The tile it shows, -1 for none.
        int tile = -1;
        compages::world::Entity entity;
        compages::renderer::TextureAssetId texture{};
        Job<Patch> job;
        bool ready = false;
        //! \brief Shown this frame, in the place of its coarse tile.
        bool shown = false;
        //! \brief Its heights, kept for groundAt() (without its picture).
        Patch ground{};
        //! \brief The frame it was last wanted at: the oldest is reused.
        std::uint64_t used = 0u;
    };

    //! \brief The tiles wanted, nearest first.
    [[nodiscard]] static std::vector<int> wanted(compages::Vector3f p_target,
                                                 float p_distance);

    //! \brief Send the tiles made since the last frame to the GPU.
    void receive(compages::renderer::Scene& p_scene,
                 compages::renderer::MaterialId p_material);

    //! \brief Send one made tile to the GPU.
    [[nodiscard]] compages::Status
    send(compages::renderer::Scene& p_scene,
         compages::renderer::MaterialId p_material,
         std::size_t p_slot,
         Patch const& p_patch);

    //! \brief Start the wanted tiles nobody makes yet.
    void start(Land const& p_land, std::vector<int> const& p_wanted);

    std::vector<Detail> m_details;
    std::uint64_t m_clock = 0u;
    std::uint64_t m_shown_mask = 0u;
};

} // namespace examples::archipelago
