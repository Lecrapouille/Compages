// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <utility>

namespace compages
{

// ****************************************************************************
//! \brief An error carrying its message, on its way up to the caller.
//!
//! This type exists so that a failure can be written once and returned from a
//! function whatever its return type:
//! \code
//! compages::Result<Texture> load(std::string const& p_path)
//! {
//!     if (!exists(p_path))
//!         return compages::failure("no such file: " + p_path);
//!     ...
//! }
//! \endcode
// ****************************************************************************
struct Failure
{
    std::string message;
};

// ----------------------------------------------------------------------------
//! \brief Build a failure to be returned as any Result.
//! \param[in] p_message what went wrong, in a form a user can act on.
// ----------------------------------------------------------------------------
[[nodiscard]] inline Failure failure(std::string p_message)
{
    return Failure{ std::move(p_message) };
}

// ****************************************************************************
//! \brief Holds either a value or the reason why it could not be produced.
//!
//! This is the C++20 stand-in for C++23's std::expected, in the spirit of
//! Rust's Result. The library uses it for everything that can fail for a reason
//! outside the programmer's control: compiling a shader, reading a file, asking
//! the driver for a feature it does not have. Exceptions are not used, and
//! mistakes that are the programmer's own fault (using a released handle,
//! reading a value from a failed Result) are assertions instead, because they
//! must be fixed rather than handled.
//!
//! \code
//! auto program = compages::gpu::Program::fromFiles("a.vert", "a.frag");
//! if (!program)
//! {
//!     std::cerr << program.error() << std::endl;
//!     return;
//! }
//! program.value().use();
//! \endcode
//!
//! \tparam T type of the value on success. Use Result<void> when there is
//! nothing to return but the operation can still fail.
// ****************************************************************************
template <typename T>
class [[nodiscard]] Result
{
public:

    Result(T p_value) : m_value(std::move(p_value)) {}

    Result(Failure p_failure) : m_error(std::move(p_failure.message)) {}

    [[nodiscard]] explicit operator bool() const
    {
        return m_value.has_value();
    }

    [[nodiscard]] T& value()
    {
        assert(m_value.has_value() && "Result::value() on a failed Result");
        return *m_value;
    }

    [[nodiscard]] T const& value() const
    {
        assert(m_value.has_value() && "Result::value() on a failed Result");
        return *m_value;
    }

    [[nodiscard]] T take()
    {
        assert(m_value.has_value() && "Result::take() on a failed Result");
        return std::move(*m_value);
    }

    [[nodiscard]] T valueOr(T p_fallback) const
    {
        return m_value.has_value() ? *m_value : std::move(p_fallback);
    }

    [[nodiscard]] std::string const& error() const
    {
        return m_error;
    }

private:

    std::optional<T> m_value;
    std::string m_error;
};

// ****************************************************************************
//! \brief A Result with nothing to return: it either worked or it did not.
// ****************************************************************************
template <>
class [[nodiscard]] Result<void>
{
public:

    Result() = default;

    Result(Failure p_failure)
        : m_error(std::move(p_failure.message)), m_ok(false)
    {
    }

    [[nodiscard]] explicit operator bool() const
    {
        return m_ok;
    }

    [[nodiscard]] std::string const& error() const
    {
        return m_error;
    }

private:

    std::string m_error;
    bool m_ok = true;
};

//! \brief Shorthand for a Result carrying no value.
using Status = Result<void>;

[[nodiscard]] inline Status success()
{
    return Status{};
}

} // namespace compages

// ****************************************************************************
//! \brief Run an expression returning a Result and give up early if it failed,
//! forwarding the error to our own caller.
//!
//! \code
//! COMPAGES_TRY(m_texture.load("wood.png"));
//! \endcode
// ****************************************************************************
#define COMPAGES_TRY(expr)                                          \
    do                                                              \
    {                                                               \
        auto compages_try_result_ = (expr);                         \
        if (!compages_try_result_)                                  \
        {                                                           \
            return compages::failure(compages_try_result_.error()); \
        }                                                           \
    } while (false)

// ****************************************************************************
//! \brief Evaluate a Result and move its value into an existing target, giving
//! up early if it failed.
//!
//! The one way to take a value out of a Result without a temporary and a
//! std::move written by hand:
//! \code
//! compages::Status Demo::setUp()
//! {
//!     COMPAGES_TRY_ASSIGN(m_block,
//!     compages::gpu::UniformBlock::create(m_program, "Camera")); return
//!     compages::success();
//! }
//! \endcode
//!
//! Most objects of the gpu layer have a load() that fills them in place and
//! returns a Status, which reads better still:
//! \code
//! COMPAGES_TRY(m_program.load(VERTEX_SHADER, FRAGMENT_SHADER));
//! \endcode
// ****************************************************************************
#define COMPAGES_TRY_ASSIGN(target, expr)                           \
    do                                                              \
    {                                                               \
        auto compages_try_result_ = (expr);                         \
        if (!compages_try_result_)                                  \
        {                                                           \
            return compages::failure(compages_try_result_.error()); \
        }                                                           \
        (target) = compages_try_result_.take();                     \
    } while (false)
