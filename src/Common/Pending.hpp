//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_PENDING_DATA_HPP
#define OPENGLCPPWRAPPER_PENDING_DATA_HPP

#include <algorithm>

// *****************************************************************************
//! \brief Class tracking the smallest block of contiguous elements that have
//! been modified (aka dirty) inside a container. A dirty block is defined by
//! the index of its first and its last dirty element. This class is used in
//! this project for knowing dirty CPU memory block that needs to be updated
//! into the GPU. This class cannot be used alone and shall be used be the
//! parent of another class.
//!
//! Example: let suppose a non dirty initial block of memory (ie array):
//! \code
//! |---|---|---|---|---|---|---|---|---|-----------|
//! | 0 | 0 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | dirty={,} |
//! |---|---|---|---|---|---|---|---|---|-----------|
//! \endcode
//!
//! Let change the 1st element with the value 42 which is now "dirty":
//! \code
//! |---|----|---|---|---|----|---|---|---|------------|
//! | 0 | 42 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | dirty={1,1} |
//! |---|----|---|---|---|---|---|---|---|-------------|
//! \endcode
//!
//! Pending data is now referring to this first element of the array. Now let
//! change the 5th element with the value 43 which is now "dirty":
//! \code
//! |---|----|---|---|---|----|---|---|---|-------------|
//! | 0 | 42 | 0 | 1 | 1 | 43 | 2 | 2 | 2 | dirty={1,5} |
//! |---|----|---|---|---|----|---|---|---|-------------|
//! \endcode
//!
//! Now all data from position 1 to 5 are considered as dirty. Pending data is
//! now referring to the first and fifth position of the array.  Now let change
//! the 0th element with the value 44 which is now "dirty":
//! \code
//! |----|----|---|---|---|----|---|---|---|-------------|
//! | 44 | 42 | 0 | 1 | 1 | 43 | 2 | 2 | 2 | dirty={0,5} |
//! |----|----|---|---|---|----|---|---|---|-------------|
//! \endcode
//!
//! Now, all data from index 0 to 5 are considered as dirty. Pending data is now
//! referring to the zeroth and fifth position of the array. Let suppose that
//! VBO::update() is now called. All dirty data (position 0 to 5) are flushed to
//! the GPU, there is no more dirty data and pending data indices are cleared.
//! \code
//! |----|----|---|---|---|----|---|---|---|-----------|
//! | 44 | 42 | 0 | 1 | 1 | 43 | 2 | 2 | 2 | dirty={,} |
//! |----|----|---|---|---|----|---|---|---|-----------|
//! \endcode
//!
//! Flushing bulk of data, with in our case, with some data not set as dirty,
//! seems not an optimized way of doing things. This is true but this allows to
//! reduce the number of OpenGL routines to call. In addition, we can imagine a
//! more sophisticated method in the case where order of element is not
//! important: in this case you can simply swap position of the dirty element
//! with the first non dirty element.
// *****************************************************************************
class Pending
{
public:

    //--------------------------------------------------------------------------
    //! \brief When the container is empty, the range of the area with dirty
    //! elements is not defined. So use -1 instead of 0 as equivalent of
    //! std::string::npos.
    //--------------------------------------------------------------------------
    static constexpr size_t npos = static_cast<size_t>(-1);

protected:

    //--------------------------------------------------------------------------
    //! \brief Make this class abstract to forbid to use it as standalone.
    //! Derivate this class to use it.
    //--------------------------------------------------------------------------
    Pending() = default;

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor with an already known number of dirty elements
    //! (starting from the first index).
    //--------------------------------------------------------------------------
    Pending(size_t const nb_elt)
    {
        clearPending(nb_elt);
    }

    //--------------------------------------------------------------------------
    //! \brief Return a boolean indicating if at least one element of the
    //! container is pending (aka dirty / modified / pendinfg for update).
    //!
    //! \return true if some data are pending.
    //--------------------------------------------------------------------------
    inline bool isPending() const
    {
        return (npos != m_pending_start);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the smallest contiguous range of the container having
    //! dirty elements.
    //!
    //! In the case there is no pending data, the return range bound is logicaly
    //! undefined but returned pos_start and pos_end are set to \c npos. You
    //! can call isPending() before calling this method.
    //--------------------------------------------------------------------------
    void getPending(size_t& pos_start, size_t& pos_end) const
    {
        pos_start = m_pending_start;
        pos_end = m_pending_end;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the smallest contiguous range of the container having
    //! dirty elements.
    //!
    //! In the case there is no pending data, the return range bound is logicaly
    //! undefined but we return pos_start and pos_end are set to \c npos. You
    //! can call isPending() before calling this method.
    //!
    //! \return {pos_start, pos_end}
    //--------------------------------------------------------------------------
    std::pair<size_t, size_t> getPending() const
    {
        return { m_pending_start, m_pending_end };
    }

    //--------------------------------------------------------------------------
    //! \brief Reset the smallest contiguous range of the container having
    //! dirty elements.
    //!
    //! Call this the smallest contiguous range have been updated and therefore
    //! when there is no more dirty data.
    //--------------------------------------------------------------------------
    void clearPending()
    {
        m_pending_start = npos;
        m_pending_end = npos;
    }

    //--------------------------------------------------------------------------
    //! \brief Reset the smallest contiguous range of the container having dirty
    //! elements and tag the \c nb_elt first elements as dirty. Use this method
    //! from constructors.
    //--------------------------------------------------------------------------
    void clearPending(size_t const nb_elt)
    {
        if (0u == nb_elt)
        {
            m_pending_start = npos;
            m_pending_end = npos;
        }
        else
        {
            m_pending_start = 0u;
            m_pending_end = nb_elt;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Update the range of dirty elements with a new range.
    //--------------------------------------------------------------------------
    void setPending(size_t const pos_start, size_t const pos_end)
    {
        if (isPending())
        {
            m_pending_start = std::min(m_pending_start, pos_start);
            m_pending_end = std::max(m_pending_end, pos_end);
        }
        else
        {
            m_pending_start = pos_start;
            m_pending_end = pos_end;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Update the range of dirty elements by passing the newly dirty
    //! element.
    //--------------------------------------------------------------------------
    inline void setPending(size_t const pos)
    {
        if (isPending())
        {
            m_pending_start = std::min(m_pending_start, pos);
            m_pending_end = std::max(m_pending_end, pos);
        }
        else
        {
            m_pending_start = pos;
            m_pending_end = pos;
        }
    }

protected:

    //! Tracks the staring bound of elementsthat  have been changed.
    size_t m_pending_start = npos;

    //! Tracks the ending bound of elementsthat  have been changed.
    size_t m_pending_end = npos;
};

#endif // OPENGLCPPWRAPPER_PENDING_DATA_HPP
