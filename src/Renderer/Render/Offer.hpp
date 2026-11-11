// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Shader.hpp"

#include <string_view>
#include <utility>

namespace compages::renderer
{

// ----------------------------------------------------------------------------
//! \brief Set a uniform that the Renderer gives to every material of a family,
//! only when the shader reads it: a main() given to a hook may leave the
//! lights, the fog or the shadows out, and the compiler then removes them.
// ----------------------------------------------------------------------------
template <typename Value>
void offer(compages::gpu::Program& p_program,
           std::string_view p_name,
           Value&& p_value)
{
    if (p_program.has(p_name))
    {
        p_program.set(p_name, std::forward<Value>(p_value));
    }
}

} // namespace compages::renderer
