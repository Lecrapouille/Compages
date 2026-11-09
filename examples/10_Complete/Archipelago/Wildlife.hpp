// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

#include <cstdint>
#include <random>
#include <vector>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief Boats sail round the islands where the water is deep enough, on
//! the swell; gulls circle low along the coasts and flap; fish swim in the
//! lagoons and jump. A few lines each. Each one is drawn at least as large
//! as a part of its distance to the camera, like the icons of a map.
// ****************************************************************************
class Wildlife
{
public:

    //! \brief Make them (the first time) and put them where the land
    //! allows. \p p_base tells where the sea is and how deep.
    void place(compages::renderer::Scene& p_scene,
               Land const& p_land,
               Patch const& p_base,
               std::uint32_t p_seed);

    //! \brief Move them by \p p_dt seconds of their time.
    //! \param[in] p_time Seconds since the start, for the swell.
    //! \param[in] p_eye Where the camera is.
    void animate(Patch const& p_base,
                 float p_dt,
                 float p_time,
                 compages::Vector3f p_eye);

private:

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

    //! \brief A gull circles along the coast of an island, low over the
    //! ground or the sea, flapping, then gliding.
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
        //! \brief Metres above what is under it, and its height now (it
        //! follows the relief smoothly; negative before its first flight).
        float altitude;
        float height;
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

    using Random = std::mt19937;

    void makeBoats(compages::renderer::Scene& p_scene);
    void makeBirds(compages::renderer::Scene& p_scene);
    void makeFish(compages::renderer::Scene& p_scene);
    void placeBoats(Land const& p_land, Patch const& p_base, Random& p_random);
    void placeBirds(Land const& p_land, Random& p_random);
    void placeFish(Patch const& p_base, Random& p_random);
    static void sail(Boat& p_boat,
                     Patch const& p_base,
                     float p_dt,
                     float p_time,
                     compages::Vector3f p_eye);
    static void fly(Bird& p_bird,
                    Patch const& p_base,
                    float p_dt,
                    float p_time,
                    compages::Vector3f p_eye);
    static void swim(Fish& p_fish, float p_dt, compages::Vector3f p_eye);

    std::vector<Boat> m_boats;
    std::vector<Bird> m_birds;
    std::vector<Fish> m_fish;
};

} // namespace examples::archipelago
