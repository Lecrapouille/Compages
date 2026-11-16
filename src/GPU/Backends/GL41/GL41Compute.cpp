// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

// ****************************************************************************
// No compute, no storage block and no image on OpenGL 4.1 and OpenGL ES 3.0.
// ComputeProgram::load() already fails before anything reaches here
// (DeviceInfo::compute is false); these keep the contract and say so if a
// call gets through anyway.
// ****************************************************************************

namespace compages::gpu::backend
{

void bindStorageBlock(NativeId /* program */, int /* block_index */, int /* binding */)
{
    reportError(std::string(name()) + " has no storage blocks (they need OpenGL 4.3 or OpenGL ES 3.1)");
}

void dispatch(std::uint32_t /* groups_x */, std::uint32_t /* groups_y */, std::uint32_t /* groups_z */)
{
    reportError(std::string(name()) + " cannot run compute shaders (they need OpenGL 4.3 or OpenGL ES 3.1)");
}

void memoryBarrier(Barrier /* what */)
{
    // Without compute, images or storage, every write is a draw, and the
    // driver orders draws itself: nothing to wait for.
}

} // namespace compages::gpu::backend
