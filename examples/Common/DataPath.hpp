// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <vector>

#if defined(__linux__)
#    include <unistd.h>
#endif

namespace examples
{

namespace detail
{

[[nodiscard]] inline std::vector<std::filesystem::path> dataRoots()
{
    std::vector<std::filesystem::path> roots;

    if (const char* env = std::getenv("COMPAGES_DATA_PATH"))
    {
        if (env[0] != '\0')
        {
            roots.emplace_back(env);
        }
    }

#if defined(COMPAGES_DATA_DIR)
    // Compages-data fetched by MyMakefile, in its cache.
    roots.emplace_back(COMPAGES_DATA_DIR);
#endif
    for (const char* relative : { "external/Compages-data",
                                  "../external/Compages-data",
                                  "../../external/Compages-data" })
    {
        roots.emplace_back(relative);
    }

#if defined(__linux__)
    char buffer[4096];
    const ssize_t length =
        ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1u);
    if (length > 0)
    {
        buffer[length] = '\0';
        std::filesystem::path dir = std::filesystem::path(buffer).parent_path();
        std::error_code error;
        for (int depth = 0; depth < 6; ++depth)
        {
            const std::filesystem::path candidate =
                dir / "external" / "Compages-data";
            if (std::filesystem::is_directory(candidate, error))
            {
                roots.emplace_back(candidate);
                break;
            }
            if (!dir.has_parent_path())
            {
                break;
            }
            dir = dir.parent_path();
        }
    }
#endif

    return roots;
}

} // namespace detail

// ****************************************************************************
//! \brief Where a file from the Compages-data repository lives.
//!
//! The gallery can be started from the project root, from \c build/, from
//! \c examples/, or from an IDE with another working directory. Set
//! \c COMPAGES_DATA_PATH to override the search path entirely.
// ****************************************************************************
[[nodiscard]] inline std::filesystem::path
dataPath(std::filesystem::path const& p_name)
{
    std::error_code error;
    for (std::filesystem::path const& root : detail::dataRoots())
    {
        const std::filesystem::path path = root / p_name;
        if (std::filesystem::exists(path, error))
        {
            return path;
        }
    }
    return {};
}

} // namespace examples
