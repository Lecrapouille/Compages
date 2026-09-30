// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Matrix.hpp"
#include "Compages/World/EntityId.hpp"

#include <vector>

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::world
{



// ****************************************************************************
//! \brief Joint Entities of a skinned MeshRenderer, in skin order.
//!
//! Lives on the same EntityId as the MeshRenderer. The MeshAsset holds the
//! inverse-bind matrices and the rest-pose vertices; this component only
//! names which World nodes are the current pose.
//!
//! \code
//! skin.joints = { hip, knee, ankle }; // EntityId per bone
//! \endcode
// ****************************************************************************
struct SkinInstance
{
    std::vector<EntityId> joints;
    //! \brief ibm * jointWorld * inverse(meshWorld), filled by
    //! \c AnimationSystem::pose. The Extractor copies this into the
    //! snapshot; the vertex shader is what deforms the rest pose.
    std::vector<compages::core::Matrix44f> pose;
};

} // namespace compages::world
