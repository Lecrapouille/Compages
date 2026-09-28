// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Statistics.hpp"

// ****************************************************************************
//! \file
//! \brief How the counters of the frame are written to.
//!
//! Internal, so that the numbers a caller reads can only be changed by the calls
//! that really did the work.
// ****************************************************************************

namespace compages::gpu::detail
{

// ----------------------------------------------------------------------------
//! \brief Record one draw call.
//!
//! \param[in] p_vertices how many vertices or indices it asked for.
//! \param[in] p_instances how many objects it drew.
// ----------------------------------------------------------------------------
void countDraw(std::size_t p_vertices, std::size_t p_instances);

// ----------------------------------------------------------------------------
//! \brief Record one pass being opened.
// ----------------------------------------------------------------------------
void countPass();

// ----------------------------------------------------------------------------
//! \brief Record one compute dispatch.
// ----------------------------------------------------------------------------
void countDispatch();

} // namespace compages::gpu::detail
