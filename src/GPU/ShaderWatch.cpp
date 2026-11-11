// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/ShaderWatch.hpp"

#include <system_error>

namespace compages::gpu
{

// ----------------------------------------------------------------------------
Status ShaderWatch::filesOf(Watched& p_watched)
{
    p_watched.files.clear();
    for (ShaderSource const* source : { &p_watched.vertex, &p_watched.fragment })
    {
        ResolvedSource resolved;
        COMPAGES_TRY_ASSIGN(resolved, source->resolve());
        for (std::filesystem::path const& file : resolved.files)
        {
            std::error_code error;
            const auto time = std::filesystem::last_write_time(file, error);
            p_watched.files.emplace_back(file, error ? std::filesystem::file_time_type{} : time);
        }
    }
    return success();
}

// ----------------------------------------------------------------------------
Status ShaderWatch::watch(ShaderSource p_vertex, ShaderSource p_fragment, Load p_load)
{
    Watched watched{ .vertex = std::move(p_vertex),
                     .fragment = std::move(p_fragment),
                     .load = std::move(p_load),
                     .files = {} };
    COMPAGES_TRY(watched.load(watched.vertex, watched.fragment));
    COMPAGES_TRY(filesOf(watched));
    m_watched.push_back(std::move(watched));
    return success();
}

// ----------------------------------------------------------------------------
Status ShaderWatch::poll()
{
    Status result = success();
    for (Watched& watched : m_watched)
    {
        bool changed = false;
        for (auto const& [file, time] : watched.files)
        {
            std::error_code error;
            const auto now = std::filesystem::last_write_time(file, error);
            changed = changed || (!error && (now != time));
        }
        if (!changed)
        {
            continue;
        }
        // The times are taken first: a file that does not compile is not
        // loaded again at each poll, only when it is saved again.
        const Status listed = filesOf(watched);
        const Status loaded = listed ? watched.load(watched.vertex, watched.fragment) : listed;
        if (loaded)
        {
            ++m_reloads;
        }
        else if (result)
        {
            result = loaded;
        }
    }
    return result;
}

} // namespace compages::gpu
