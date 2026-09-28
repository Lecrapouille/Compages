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

#include <algorithm>
#include <cstddef>

namespace compages::gpu
{

// ****************************************************************************
//! \brief Which elements of a container have changed since the device was last
//! given a copy.
//!
//! Only the outermost bounds are kept, so touching element 1 and element 5000
//! means everything between them is sent too. That is on purpose: one write of
//! five thousand elements costs far less than five thousand writes of one, and
//! keeping a list of individual holes would cost more to maintain than it saves.
//!
//! The range is half open, [begin(), end()), the same convention as every
//! standard container. This type exists mostly to make that convention
//! impossible to get wrong. Its ancestor in this library, class Pending, was
//! ambiguous about it: setPending(pos) recorded start == end == pos, meaning one
//! element in an inclusive reading and nothing at all in an exclusive one, and
//! the code that sent the bytes computed its length as end - start. Changing a
//! single vertex therefore sent zero bytes, so it never reached the screen. The
//! same code also always read from the front of the container while writing at
//! the offset of the range, so a change to anything but the first element landed
//! in the wrong place. Both mistakes were invisible in a review and expensive to
//! find in a running program, hence one small type with one convention and its
//! own tests.
// ****************************************************************************
class DirtyRange
{
public:

    // ------------------------------------------------------------------------
    //! \brief Nothing has changed.
    // ------------------------------------------------------------------------
    DirtyRange() = default;

    // ------------------------------------------------------------------------
    //! \brief Record that one element has changed.
    // ------------------------------------------------------------------------
    void add(std::size_t p_index)
    {
        add(p_index, 1u);
    }

    // ------------------------------------------------------------------------
    //! \brief Record that a run of elements has changed.
    //!
    //! \param[in] p_first index of the first one.
    //! \param[in] p_count how many. Zero changes nothing, which is what makes
    //! it safe to call with the size of an empty edit.
    // ------------------------------------------------------------------------
    void add(std::size_t p_first, std::size_t p_count)
    {
        if (p_count == 0u)
        {
            return;
        }
        const std::size_t last = p_first + p_count;
        if (empty())
        {
            m_begin = p_first;
            m_end = last;
        }
        else
        {
            m_begin = std::min(m_begin, p_first);
            m_end = std::max(m_end, last);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Record that everything has changed, in a container of that size.
    // ------------------------------------------------------------------------
    void addAll(std::size_t p_size)
    {
        m_begin = 0u;
        m_end = p_size;
    }

    // ------------------------------------------------------------------------
    //! \brief Forget everything, once the device has been given the bytes.
    // ------------------------------------------------------------------------
    void clear()
    {
        m_begin = 0u;
        m_end = 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Drop whatever falls outside a container of that size.
    //!
    //! Called after the container has shrunk: a range still naming elements that
    //! no longer exist would send bytes from past the end of the vector.
    // ------------------------------------------------------------------------
    void clampTo(std::size_t p_size)
    {
        if (m_begin >= p_size)
        {
            clear();
            return;
        }
        m_end = std::min(m_end, p_size);
    }

    // ------------------------------------------------------------------------
    //! \brief Has nothing changed?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return m_begin == m_end;
    }

    // ------------------------------------------------------------------------
    //! \brief Index of the first changed element. Meaningless when empty.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t begin() const
    {
        return m_begin;
    }

    // ------------------------------------------------------------------------
    //! \brief Index just past the last changed element. Meaningless when empty.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t end() const
    {
        return m_end;
    }

    // ------------------------------------------------------------------------
    //! \brief How many elements have to be sent.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t count() const
    {
        return m_end - m_begin;
    }

private:

    std::size_t m_begin = 0u;
    std::size_t m_end = 0u;
};

} // namespace compages::gpu
