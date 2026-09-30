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

#include "Compages/GPU/Errors.hpp"

#include <cassert>

namespace compages::gpu
{

namespace
{

//! \brief The first failure since the error was last taken. There is one
//! device, driven from one thread, so there is one of these.
std::string g_first;
std::size_t g_count = 0u;
bool g_break = false;

} // namespace

//------------------------------------------------------------------------------
void reportError(std::string p_message)
{
    if (g_count == 0u)
    {
        g_first = std::move(p_message);
    }
    ++g_count;
    assert(!g_break &&
           "compages::gpu::reportError() with setBreakOnError(true): see "
           "compages::gpu::takeFrameError() for the message");
}

//------------------------------------------------------------------------------
bool check(Status const& p_status)
{
    if (!p_status)
    {
        reportError(p_status.error());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
bool hasFrameError()
{
    return g_count > 0u;
}

//------------------------------------------------------------------------------
std::size_t frameErrorCount()
{
    return g_count;
}

//------------------------------------------------------------------------------
std::string takeFrameError()
{
    std::string message = std::move(g_first);
    g_first.clear();
    g_count = 0u;
    return message;
}

//------------------------------------------------------------------------------
void setBreakOnError(bool p_enabled)
{
    g_break = p_enabled;
}

//------------------------------------------------------------------------------
bool breakOnError()
{
    return g_break;
}

} // namespace compages::gpu
