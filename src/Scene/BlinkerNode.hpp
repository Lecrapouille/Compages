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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_BLINKER_NODE_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_BLINKER_NODE_HPP

#include "Scene/SwitchNode.hpp"
#include <chrono>

using namespace std::chrono_literals;
using namespace std::chrono;

// *****************************************************************************
//! \brief A special scene node cycling its children at a given frequency.
//!
//! Each child are enabled one after another during a given duration (same
//! duration for each child).
//!
//! \note this node has been inspired by OpenInventor soBlinker
//! https://grey.colorado.edu/coin3d/classSoBlinker.html
//! \fixme: Can we define a generic Blinker with template function and then
//! define a Blinker = IBlinker<ShitftChild>
// *****************************************************************************
class BlinkerNode: public SwitchNode
{
    using Clock = steady_clock;
    using Time = Clock::time_point;

public:

    //--------------------------------------------------------------------------
    //! \brief Create a blinker node with a given name and a given activity
    //! duration.
    //!
    //! \param name the name of the node. It is used mainly for the debug.
    //! \param the duration (in ms) of each child activity (before switch shift
    //! to the next child).
    //--------------------------------------------------------------------------
    BlinkerNode(std::string const& name, milliseconds const duration)
        : SwitchNode(name), m_duration(duration)
    {
    }

    //! \brief Check if animation has been started at least once.
    inline bool isStarted() const
    {
        return m_started;
    }

    //! \brief Check if animation is running.
    inline bool isRunning() const
    {
        return m_running;
    }

    //! \brief Start or restart animation with given time.
    //! \param duration milliseconds
    void start(milliseconds const duration)
    {
        if (m_started)
            return;

        m_started = true;
        m_duration = std::min(1ms, duration);
    }

    //! \brief if and only if duration has been set
    bool start()
    {
        if (0ms == m_duration)
        {
            std::cerr << "Duration has not been initialized" << std::endl;
            return false;
        }
        m_started = true;
        return true;
    }

    //! \brief Stop or pause animation.
    inline void stop()
    {
        m_started = false;
    }

    //! \brief Reset animation, i.e., set all variables to their initial states.
    inline void reset(milliseconds const duration)
    {
        m_started = false;
        m_running = false;
        m_duration = std::min(1ms, duration);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief When the scene is traversed we update the elapsed time and when
    //! the duration of the switch expired we select the next child.
    //! \param dt: delta time from the previous update.
    //--------------------------------------------------------------------------
    virtual void onUpdate(float const /*dt*/) override
    {
        if (unlikely(!m_started))
            return;

        if (likely(m_running))
        {
            Time currentTime = Clock::now();
            milliseconds dt =
                duration_cast<milliseconds>(currentTime - m_startedTime);
            if (dt > m_duration)
            {
                m_startedTime = currentTime;
                next();
            }
        }
        else
        {
            if (unlikely(0ms == m_duration))
            {
                std::cerr << "duration has not been set" << std::endl;
                return;
            }

            m_running = true;
            m_startedTime = Clock::now();
            next();
        }
    }

private:

    //! \brief true if animation has been started (may be stopped temporarily)
    //! \fixme TBD atomic bool for start()/stop() from different threads ???
    bool m_started = false;
    //! \brief true if animation is running
    bool m_running = false;
    //! \brief time at start (milliseconds)
    Time m_startedTime;
    //! \brief
    milliseconds m_duration;
};

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_BLINKER_NODE_HPP
