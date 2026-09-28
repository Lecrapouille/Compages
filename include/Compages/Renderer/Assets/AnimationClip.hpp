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

#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief One sampled curve that writes a local TRS channel of an EntityId.
//!
//! Times are in seconds. Values are packed: three floats per key for
//! translation and scale, four (glTF xyzw) for rotation.
// ****************************************************************************
struct AnimationChannel
{
    //! \brief Which transform component this curve drives.
    enum class Path
    {
        Translation,
        Rotation,
        Scale,
    };

    //! \brief How to interpolate between keys.
    enum class Interpolation
    {
        Step,
        Linear,
    };

    //! \brief Index of the target node in the reusable prefab.
    std::uint32_t target = 0u;
    //! \brief Transform component written by this curve.
    Path path = Path::Translation;
    //! \brief Interpolation mode between consecutive keys.
    Interpolation interpolation = Interpolation::Linear;
    //! \brief Key times in seconds, sorted ascending.
    std::vector<float> times;
    //! \brief Packed key values (3 floats per key for T/S, 4 for rotation).
    std::vector<float> values;
};

// ****************************************************************************
//! \brief A named clip imported from a glTF animation.
// ****************************************************************************
struct AnimationClip
{
    //! \brief Clip name from the asset (glTF animation name).
    std::string name;
    //! \brief Length of the clip in seconds (last key time).
    float duration = 0.0f;
    //! \brief All curves sampled together when the clip plays.
    std::vector<AnimationChannel> channels;
};

} // namespace compages::renderer
