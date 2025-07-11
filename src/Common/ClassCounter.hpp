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

#ifndef CLASS_COUNTER_HPP
#define CLASS_COUNTER_HPP

#include <atomic>
#include <cstddef>

typedef size_t Key;

// *****************************************************************************
//! \brief This class allows to count the number of created instances. It's
//! safer to use this class than using directly a memeber 'static size_t count;'
//! when a failure occured in creator member the number of instances is correct
//! and also for derived classes.
//!
//! Inspired from: http://www.drdobbs.com/cpp/counting-objects-in-c/184403484
// *****************************************************************************
template <class T>
class InstanceCounter
{
public:

    //! \brief Empty constructor. Increase the number of instances.
    InstanceCounter()
    {
        ++m_how_many;
    }
    //! \brief Constructor by copy. Increase the number of instances.
    InstanceCounter(const InstanceCounter&)
    {
        ++m_how_many;
    }
    //! \brief Destructor. Decrease the number of instances.
    ~InstanceCounter()
    {
        --m_how_many;
    }
    //! \brief Static member. Return the number of instances.
    static Key count()
    {
        return m_how_many.load();
    }

private:

    //! \brief Static member saving the number of instances.
    static std::atomic<Key> m_how_many;
};

template <class T>
std::atomic<Key> InstanceCounter<T>::m_how_many(0U);

// *****************************************************************************
//! \brief
// *****************************************************************************
template <class T>
class UniqueID
{
public:

    //! \brief Empty constructor. Increase the unique identifier.
    UniqueID()
    {
        ++m_id_count;
    }
    //! \brief Constructor by copy. Increase the unique identifier.
    UniqueID(const UniqueID&)
    {
        ++m_id_count;
    }
    //! \brief Destructor.
    ~UniqueID() {}
    //! \brief Static member. Reset unique identifier.
    static void resetID()
    {
        m_id_count = -1U;
    }
    //! \brief Return the unique identifier.
    static Key getID()
    {
        return m_id_count.load();
    }

private:

    //! \brief Static member saving the number of instances.
    static std::atomic<Key> m_id_count;
};

template <class T>
std::atomic<Key> UniqueID<T>::m_id_count(0U);

#endif // CLASS_COUNTER_HPP
