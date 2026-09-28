// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

namespace compages::renderer::shaders
{

// ****************************************************************************
//! \file
//! \brief GLSL sources grouped by feature, not by backend.
//!
//! ShaderLib only returns source strings. Primitives and loaders turn them
//! into \c compages::gpu::Program and \c compages::gpu::Pipeline through the AssetManager.
// ****************************************************************************

inline constexpr int MAX_POINT_LIGHTS = 4;
//! \brief Joint palette uploaded each draw. Mixamo Soldier sits under 60.
inline constexpr int MAX_JOINTS = 64;

[[nodiscard]] char const* litVertex();
[[nodiscard]] char const* litFragment();

[[nodiscard]] char const* pbrVertex();
[[nodiscard]] char const* pbrFragment();

[[nodiscard]] char const* depthVertex();
[[nodiscard]] char const* depthFragment();

[[nodiscard]] char const* normalsVertex();
[[nodiscard]] char const* normalsFragment();

} // namespace compages::renderer::shaders
