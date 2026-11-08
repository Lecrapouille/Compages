// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <chrono>
#include <exception>
#include <future>
#include <thread>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief A piece of work running in a thread of its own, and its result.
//! The frame asks ready() and takes the result; a Job still running when it
//! is destroyed is waited for.
// ****************************************************************************
template <typename T>
class Job
{
public:

    //! \brief Start \p p_work (a callable returning a T) in a new thread.
    //! The previous result, if any, is dropped.
    template <typename Work>
    void start(Work p_work)
    {
        wait();
        std::promise<T> promise;
        m_result = promise.get_future();
        m_thread = std::jthread(
            [promise = std::move(promise), work = std::move(p_work)]() mutable
            {
                try
                {
                    promise.set_value(work());
                }
                catch (...)
                {
                    promise.set_exception(std::current_exception());
                }
            });
    }

    //! \brief A result is coming (ready or not yet).
    [[nodiscard]] bool busy() const
    {
        return m_result.valid();
    }

    //! \brief The result can be taken without waiting.
    [[nodiscard]] bool ready() const
    {
        return m_result.valid() &&
               (m_result.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready);
    }

    //! \brief The result (waits for it); the Job is then free again.
    [[nodiscard]] T take()
    {
        T result = m_result.get();
        m_thread.join();
        return result;
    }

    //! \brief Wait for the work and drop its result.
    void wait()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        m_result = {};
    }

private:

    std::future<T> m_result;
    std::jthread m_thread;
};

} // namespace examples::archipelago
