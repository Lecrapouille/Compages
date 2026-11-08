// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief Opens web pages in the browser: $BROWSER, else firefox, given all
//! the links at once (one tab each). Started without a shell, so a link
//! cannot run a command; the ended browsers are reaped. Linux only: open()
//! fails elsewhere.
// ****************************************************************************
class Browser
{
public:

    //! \brief At most this many tabs at once.
    static constexpr std::size_t MAX_TABS = 20u;

    Browser() = default;
    Browser(Browser const&) = delete;
    Browser& operator=(Browser const&) = delete;
    ~Browser();

    //! \brief Open the first MAX_TABS links. \return What happened, to show.
    [[nodiscard]] std::string open(std::vector<std::string> const& p_urls);

    //! \brief Reap the browsers that ended. Each frame.
    void reap();

private:

    std::vector<int> m_children;
};

} // namespace examples::archipelago
