// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/PingPong.hpp"

namespace compages::gpu
{

// ----------------------------------------------------------------------------
Result<TexturePingPong> TexturePingPong::create(TextureDesc const& p_desc)
{
    TexturePingPong pair;
    for (int i = 0; i < 2; ++i)
    {
        COMPAGES_TRY_ASSIGN(pair.m_texture[i], Texture::create(p_desc));
        COMPAGES_TRY(pair.m_target[i].attach(pair.m_texture[i]));
    }
    return pair;
}

} // namespace compages::gpu
