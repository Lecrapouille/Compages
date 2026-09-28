// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Device.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// ****************************************************************************
//! \file
//! \brief What the library, the driver and the examples said, kept for the
//! Console panel of the gallery.
// ****************************************************************************

namespace examples
{

// ****************************************************************************
//! \brief Every message of the run, the same message said twice kept once.
//!
//! A driver repeating one warning sixty times a second is one line with a
//! count, not a terminal scrolling too fast to read. Messages are attributed
//! to the example that was running when they came, so that "texture unit 0
//! has nothing bound" can be traced to the example that caused it.
//!
//! \code
//! Console console;
//! console.add(compages::gpu::LogLevel::Warning, "slow path taken", "01b_Triangle");
//! console.add(compages::gpu::LogLevel::Warning, "slow path taken", "01b_Triangle");
//! console.messages().front().count;   // 2
//! \endcode
// ****************************************************************************
class Console
{
public:

    // ************************************************************************
    //! \brief One message, and how many times it was said.
    // ************************************************************************
    struct Message
    {
        compages::gpu::LogLevel level = compages::gpu::LogLevel::Info;
        std::string text;
        //! \brief The example running when it was said, empty for the gallery.
        std::string example;
        std::size_t count = 1u;
    };

    // ------------------------------------------------------------------------
    //! \brief Keep a message, or count it once more if it was already said.
    //!
    //! \return true the first time this message is said, which is when it is
    //! worth echoing somewhere else, such as the terminal.
    // ------------------------------------------------------------------------
    bool add(compages::gpu::LogLevel p_level,
             std::string_view p_text,
             std::string const& p_example)
    {
        for (Message& message : m_messages)
        {
            if ((message.level == p_level) && (message.text == p_text) &&
                (message.example == p_example))
            {
                ++message.count;
                return false;
            }
        }
        m_messages.emplace_back(Message{ p_level, std::string(p_text), p_example, 1u });
        ++m_counts[index(p_level)];
        return true;
    }

    //! \brief Forget every message.
    void clear()
    {
        m_messages.clear();
        m_counts = { 0u, 0u, 0u };
    }

    //! \brief Every message, oldest first.
    [[nodiscard]] std::vector<Message> const& messages() const
    {
        return m_messages;
    }

    //! \brief How many different messages of this level were said.
    [[nodiscard]] std::size_t count(compages::gpu::LogLevel p_level) const
    {
        return m_counts[index(p_level)];
    }

private:

    [[nodiscard]] static std::size_t index(compages::gpu::LogLevel p_level)
    {
        return static_cast<std::size_t>(p_level);
    }

    std::vector<Message> m_messages;
    std::array<std::size_t, 3u> m_counts{ 0u, 0u, 0u };
};

} // namespace examples
