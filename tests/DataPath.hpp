// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <filesystem>
#include <system_error>

namespace tests
{

// ****************************************************************************
//! \brief Where a file of Compages-data is.
//!
//! The tests can be started from the project root or from \c tests/.
// ****************************************************************************
[[nodiscard]] inline std::filesystem::path
dataPath(std::filesystem::path const& p_name)
{
    std::error_code error;
    for (const char* root :
         { "external/Compages-data", "../external/Compages-data" })
    {
        const std::filesystem::path path = std::filesystem::path(root) / p_name;
        if (std::filesystem::exists(path, error))
        {
            return path;
        }
    }
    return {};
}

} // namespace tests
