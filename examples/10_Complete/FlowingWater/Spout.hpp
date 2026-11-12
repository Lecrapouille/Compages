// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Assets/MeshAsset.hpp"

// ****************************************************************************
//! \file
//! \brief The water spat by a pipe in 10f_FlowingWater: a tube along the path
//! of a thrown body, and a crown of spray where it falls into the pool. The
//! water in them is only a picture that scrolls (Shaders.hpp, JET_MAIN).
// ****************************************************************************

namespace examples::flowing
{

// ****************************************************************************
//! \brief A jet of water: where it leaves the pipe, how fast, how thick.
// ****************************************************************************
struct Jet
{
    compages::Vector3f mouth{ 0.0f, 0.0f, 0.0f };
    //! \brief Metres per second when it leaves the pipe.
    compages::Vector3f velocity{ 0.0f, 0.0f, 0.0f };
    float radius = 0.2f;

    //! \brief Seconds until it reaches the height \c p_level (gravity alone).
    [[nodiscard]] float fall(float p_level) const;
    //! \brief Where it is after \c p_time seconds.
    [[nodiscard]] compages::Vector3f at(float p_time) const;
};

// ----------------------------------------------------------------------------
//! \brief A tube along the path of the jet down to a little under the water,
//! in world coordinates; U goes round, V is the length along the jet in
//! metres (the shader scrolls the water along it).
// ----------------------------------------------------------------------------
[[nodiscard]] compages::renderer::MeshAsset jetMesh(Jet const& p_jet, float p_level);

// ----------------------------------------------------------------------------
//! \brief An open cone round where the jet falls, wider at the top: the
//! spray thrown up. V goes from 0 at the water to 1 at the top.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::renderer::MeshAsset crownMesh(float p_radius, float p_height);

} // namespace examples::flowing
