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

#include "Compages/GPU/Device.hpp"
#include "GPU/Backends/Backend.hpp"
#include "GPU/Internal/Pools.hpp"

#include <iostream>

namespace compages::gpu
{

namespace
{

//! \brief The one device of the process. The public API takes no device
//! argument, which is what makes it as short as `compages::gpu::draw(...)`; the
//! price is one graphics context per process, which is what a game or a viewer
//! wants anyway.
struct State
{
    DeviceInfo info;
    LogCallback logger = nullptr;
    bool initialized = false;
    bool driver_hints = false;
};

State& state()
{
    static State instance;
    return instance;
}

//! \brief Where messages go until the caller says otherwise. Printing to stderr
//! by default means a driver error is never silently lost.
void defaultLogger(LogLevel p_level, std::string_view p_message)
{
    const char* prefix = "[gpu] ";
    switch (p_level)
    {
        case LogLevel::Info:
            prefix = "[gpu] ";
            break;
        case LogLevel::Warning:
            prefix = "[gpu] warning: ";
            break;
        case LogLevel::Error:
            prefix = "[gpu] error: ";
            break;
    }
    std::cerr << prefix << p_message << std::endl;
}

} // namespace

//------------------------------------------------------------------------------
Status init(LoadProc p_load)
{
    if (p_load == nullptr)
    {
        return failure(
            "compages::gpu::init() needs a symbol loader, for instance "
            "glfwGetProcAddress");
    }

    State& s = state();
    if (s.initialized)
    {
        return failure("compages::gpu::init() called twice "
                       "without compages::gpu::shutdown()");
    }

    s.info = DeviceInfo{};
    COMPAGES_TRY(backend::init(p_load, s.info));
    s.initialized = true;

    log(LogLevel::Info,
        std::string(backend::name()) + " on " + s.info.renderer + ", " +
            s.info.version);
    if (!s.info.debug_output)
    {
        log(LogLevel::Warning,
            "the driver cannot report its own errors, so mistakes will show up "
            "as wrong images rather than as messages");
    }

    return success();
}

//------------------------------------------------------------------------------
void shutdown()
{
    State& s = state();
    if (!s.initialized)
    {
        return;
    }

    // While the context is still current, and before the backend lets go of the
    // driver: this is the last moment at which the device can free anything.
    detail::releaseAllResources();

    backend::shutdown();
    s.info = DeviceInfo{};
    s.initialized = false;
}

//------------------------------------------------------------------------------
bool initialized()
{
    return state().initialized;
}

//------------------------------------------------------------------------------
DeviceInfo const& device()
{
    assert(state().initialized &&
           "compages::gpu::device() before compages::gpu::init()");
    return state().info;
}

//------------------------------------------------------------------------------
void logger(LogCallback p_callback)
{
    state().logger = p_callback;
}

//------------------------------------------------------------------------------
void log(LogLevel p_level, std::string_view p_message)
{
    LogCallback callback = state().logger;
    if (callback == nullptr)
    {
        callback = &defaultLogger;
    }
    callback(p_level, p_message);
}

//------------------------------------------------------------------------------
void reportDriverHints(bool p_report)
{
    state().driver_hints = p_report;
}

//------------------------------------------------------------------------------
bool driverHintsReported()
{
    return state().driver_hints;
}

} // namespace compages::gpu
