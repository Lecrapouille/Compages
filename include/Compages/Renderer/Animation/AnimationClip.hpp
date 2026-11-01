// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief One sampled curve that writes a local TRS channel of an compages::world::EntityId.
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
