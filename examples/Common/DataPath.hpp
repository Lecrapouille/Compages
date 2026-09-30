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

#pragma once

#include "Compages/Core/File.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#if defined(__linux__)
#    include <unistd.h>
#endif

namespace examples
{

namespace detail
{

[[nodiscard]] inline std::vector<std::string> dataRoots()
{
    std::vector<std::string> roots;

    if (const char* env = std::getenv("COMPAGES_DATA_PATH"))
    {
        roots.emplace_back(env);
        if (roots.back().empty() || (roots.back().back() != '/'))
        {
            roots.back() += '/';
        }
    }

    for (const char* relative : { "external/Compages-data/",
                                  "../external/Compages-data/",
                                  "../../external/Compages-data/",
                                  "external/Compages-data/",
                                  "../external/Compages-data/",
                                  "../../external/Compages-data/" })
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
        for (int depth = 0; depth < 6; ++depth)
        {
            const std::filesystem::path current =
                dir / "external" / "Compages-data";
            const std::filesystem::path upstream =
                dir / "external" / "Compages-data";
            if (std::filesystem::is_directory(current))
            {
                roots.emplace_back(current.string() + "/");
                break;
            }
            if (std::filesystem::is_directory(upstream))
            {
                roots.emplace_back(upstream.string() + "/");
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
[[nodiscard]] inline std::string dataPath(std::string const& p_name)
{
    for (std::string const& root : detail::dataRoots())
    {
        const std::string path = root + p_name;
        if (compages::core::File::exist(path))
        {
            return path;
        }
    }
    return {};
}

} // namespace examples
