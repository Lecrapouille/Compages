// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Browser.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>

#if defined(__linux__)
#    include <spawn.h>
#    include <sys/wait.h>
extern char** environ; // NOSONAR: the environment, for posix_spawnp
#endif

namespace examples::archipelago
{

// ----------------------------------------------------------------------------
Browser::~Browser()
{
    reap();
}

// ----------------------------------------------------------------------------
void Browser::reap()
{
#if defined(__linux__)
    std::erase_if(m_children,
                  [](int p_pid)
                  {
                      int status = 0;
                      return ::waitpid(p_pid, &status, WNOHANG) != 0;
                  });
#endif
}

// ----------------------------------------------------------------------------
std::string Browser::open(std::vector<std::string> const& p_urls)
{
    if (p_urls.empty())
    {
        return "No link to open";
    }
    const char* const asked = std::getenv("BROWSER");
    const std::string browser =
        ((asked != nullptr) && (asked[0] != '\0')) ? asked : "firefox";
    const std::size_t count = std::min(p_urls.size(), MAX_TABS);
#if defined(__linux__)
    // posix_spawn takes char* const*: the strings are not changed.
    std::vector<std::string> arguments = { browser };
    arguments.insert(arguments.end(),
                     p_urls.begin(),
                     p_urls.begin() + std::ptrdiff_t(count));
    std::vector<char*> argv;
    for (std::string& argument : arguments)
    {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);
    pid_t pid = 0;
    if (::posix_spawnp(
            &pid, browser.c_str(), nullptr, nullptr, argv.data(), environ) != 0)
    {
        return std::format("Cannot start {} (set BROWSER)", browser);
    }
    m_children.push_back(pid);
    if (count < p_urls.size())
    {
        return std::format(
            "Opened {} of {} links in {}", count, p_urls.size(), browser);
    }
    return std::format(
        "Opened {} link{} in {}", count, (count > 1u) ? "s" : "", browser);
#else
    return std::format("Opening {} needs Linux", browser);
#endif
}

} // namespace examples::archipelago
