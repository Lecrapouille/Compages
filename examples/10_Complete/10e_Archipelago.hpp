// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/10e_ArchipelagoLand.hpp"
#include "Common/Example.hpp"

#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <future>
#include <memory>
#include <mutex>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A map of knowledge, after Chréage: islands of themes made of
//! Voronoi cells, regions and parts made of cells, rivers that dig their
//! valleys, a living sea.
//!
//! The land is made on the CPU, in a thread, from a seed
//! (10e_ArchipelagoLand.cpp); the gallery shows a progress bar meanwhile.
//! This file shows it:
//! - level of detail: the map is cut in 8 x 8 tiles. From afar, one mesh of
//!   31 m and one picture of 16 m a pixel; near the camera, tiles of 8 m and
//!   2 m a pixel are made in the background and take the place of the
//!   coarse ones, with skirts that hide the cracks between the two;
//! - the sea and the rivers: the PBR shader of the Renderer with another
//!   main(); the alpha of the picture says where the water is;
//! - boats, birds and fish move by a few lines each;
//! - the camera is a Rig; a click flies to a part with
//!   RigController::flyTo(); a Tooltip lists the links of a region.
// ****************************************************************************
class Archipelago: public Example
{
public:

    ~Archipelago() override;

    [[nodiscard]] std::string name() const override
    {
        return "10e_Archipelago";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;
    [[nodiscard]] Progress progress() const override;

private:

    //! \brief What the loading thread makes: the land and its coarse patch.
    struct Loaded
    {
        archipelago::Land land;
        archipelago::Patch base;
    };

    //! \brief How far the loading thread has gone, read by the gallery.
    struct Report
    {
        std::mutex mutex;
        float done = 0.0f;
        std::string step = "Starting";
    };

    //! \brief A fine tile near the camera: its mesh and picture, made in the
    //! background, then sent to the GPU.
    struct Detail
    {
        //! \brief The tile it shows, -1 for none.
        int tile = -1;
        compages::world::Entity entity;
        compages::renderer::TextureAssetId texture{};
        std::future<archipelago::Patch> job;
        bool ready = false;
        //! \brief The frame it was last wanted at: the oldest is reused.
        std::uint64_t used = 0u;
    };

    //! \brief A boat sails round an island, in water deep enough.
    struct Boat
    {
        compages::world::Entity root;
        compages::Vector2f centre;
        //! \brief The distance to the centre of the loop, per 1/64 turn.
        std::vector<float> radii;
        float angle;
        //! \brief Metres a second; negative goes clockwise.
        float speed;
    };

    //! \brief A bird circles over an island, flapping, then gliding.
    struct Bird
    {
        compages::world::Entity root;
        compages::world::Entity left;
        compages::world::Entity right;
        compages::Vector3f centre;
        float radius;
        float angle;
        float speed;
        float phase;
    };

    //! \brief A fish swims in circles in a lagoon; sometimes it jumps.
    struct Fish
    {
        compages::world::Entity shadow;
        compages::world::Entity jumper;
        compages::Vector2f centre;
        float radius;
        float angle;
        float speed;
        //! \brief Seconds before the next jump, and into the jump (< 0: none).
        float wait;
        float jump;
    };

    //! \brief Start making the land from m_seed in a thread.
    void startLoading();

    //! \brief The land is made: show it (the coarse tiles, the names, the
    //! animals), on the first load or after Generate.
    [[nodiscard]] compages::Status finishLoading(Loaded p_loaded);

    //! \brief The PBR shader with the main() of the sea and the rivers.
    [[nodiscard]] compages::Status makeSea();

    //! \brief Dress a drawn entity with the shader of the map.
    [[nodiscard]] compages::Status wearSea(compages::world::Entity p_entity);

    //! \brief Choose the fine tiles near the camera, start the missing ones,
    //! show those that are ready instead of the coarse ones.
    void updateDetails();

    //! \brief Wait for the fine tiles being made, and hide them all.
    void dropDetails();

    //! \brief The ground of the coarse patch at a point: fast, for the mouse
    //! and the animals.
    [[nodiscard]] float baseHeight(float p_x, float p_z) const;

    //! \brief Put the boats, the birds and the fish where the land allows.
    void placeAnimals();

    //! \brief Move them by \p p_dt seconds of their time.
    void animate(float p_dt);

    //! \brief The region and the part under a pixel of the picture.
    [[nodiscard]] archipelago::Place placeUnder(compages::Vector2f p_mouse) const;

    //! \brief Fly to a part of a region (or to the whole map), and choose it.
    void flyTo(int p_region, int p_part);

    //! \brief The names readable from where the camera is, and the tooltips
    //! of the region under the mouse and of the chosen one.
    void updateText();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_camera;
    compages::world::Entity m_ocean;
    std::vector<compages::world::Entity> m_tiles;
    compages::renderer::TextureAssetId m_picture{};
    compages::renderer::MaterialId m_sea{};

    std::future<Loaded> m_loading;
    std::shared_ptr<Report> m_report;
    bool m_ready = false;
    archipelago::Land m_land;
    archipelago::Patch m_base;
    std::vector<Detail> m_details;
    std::uint64_t m_clock = 0u;

    std::vector<compages::world::Entity> m_island_names;
    std::vector<compages::world::Entity> m_region_names;
    std::vector<std::vector<compages::world::Entity>> m_part_names;
    std::vector<Boat> m_boats;
    std::vector<Bird> m_birds;
    std::vector<Fish> m_fish;

    int m_seed = 7;
    int m_erosion_steps = 60;
    archipelago::Place m_hovered;
    archipelago::Place m_chosen;
    bool m_borders = true;
    bool m_names = true;
    bool m_animals = true;
    bool m_details_on = true;
    //! \brief How much faster the animals live than the clock: a boat at 7
    //! m/s takes an hour round an island.
    float m_time_scale = 8.0f;
    //! \brief Seconds since the start, for the waves.
    float m_time = 0.0f;
    //! \brief Where the left button went down: a click if it comes up near.
    compages::Vector2f m_press{ 0.0f, 0.0f };
    bool m_was_down = false;
};

} // namespace examples
