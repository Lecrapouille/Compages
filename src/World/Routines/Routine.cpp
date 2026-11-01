// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Routines/Routine.hpp"

#include <algorithm>

namespace compages::world
{

Routine::TimerId Routine::after(float p_seconds, std::function<void()> p_call)
{
    const TimerId id = m_next_timer++;
    m_timers.push_back(Timer{ id, p_seconds, 0.0f, std::move(p_call) });
    return id;
}

Routine::TimerId Routine::every(float p_seconds, std::function<void()> p_call)
{
    const TimerId id = m_next_timer++;
    // A period of zero would call it at every step; keep it a period.
    const float period = std::max(p_seconds, 1.0e-6f);
    m_timers.push_back(Timer{ id, period, period, std::move(p_call) });
    return id;
}

void Routine::cancel(TimerId p_timer)
{
    std::erase_if(m_timers, [p_timer](Timer const& p_each) { return p_each.id == p_timer; });
}

} // namespace compages::world
