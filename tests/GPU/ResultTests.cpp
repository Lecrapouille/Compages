//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/Core/Result.hpp"
#include "Compages/GPU/Core/Handle.hpp"

#include <memory>
#include <string>
#include <type_traits>

namespace
{

//! \brief A value that cannot be copied, as most resource facades will be.
using Unique = std::unique_ptr<int>;

compages::gpu::Result<int> half(int p_value)
{
    if ((p_value % 2) != 0)
    {
        return compages::gpu::failure("not an even number: " + std::to_string(p_value));
    }
    return p_value / 2;
}

compages::gpu::Status check(bool p_ok)
{
    if (!p_ok)
    {
        return compages::gpu::failure("the check did not pass");
    }
    return compages::gpu::success();
}

//! \brief Gives up as soon as one step fails, forwarding its message.
compages::gpu::Status runSteps(bool p_first, bool p_second)
{
    COMPAGES_TRY(check(p_first));
    COMPAGES_TRY(check(p_second));
    return compages::gpu::success();
}

//! \brief Same, but needs the value of the step that succeeded.
compages::gpu::Result<int> quarter(int p_value)
{
    int once = 0;
    int twice = 0;
    COMPAGES_TRY_ASSIGN(once, half(p_value));
    COMPAGES_TRY_ASSIGN(twice, half(once));
    return twice;
}

} // namespace

//------------------------------------------------------------------------------
TEST(Result, CarriesAValueOnSuccess)
{
    auto result = half(10);

    ASSERT_TRUE(bool(result));
    ASSERT_EQ(result.value(), 5);
    ASSERT_TRUE(result.error().empty());
}

//------------------------------------------------------------------------------
TEST(Result, CarriesAReasonOnFailure)
{
    auto result = half(7);

    ASSERT_FALSE(bool(result));
    ASSERT_EQ(result.error(), "not an even number: 7");
}

//------------------------------------------------------------------------------
// The message is what a user reads, so it must say what was wrong and with what,
// not merely that something failed.
//------------------------------------------------------------------------------
TEST(Result, NamesWhatWentWrong)
{
    ASSERT_THAT(half(3).error(), HasSubstr("3"));
}

//------------------------------------------------------------------------------
TEST(Result, HandsBackAFallbackWhenItFailed)
{
    ASSERT_EQ(half(8).valueOr(-1), 4);
    ASSERT_EQ(half(9).valueOr(-1), -1);
}

//------------------------------------------------------------------------------
// Resource facades own something and cannot be copied, so a Result must be able
// to hand its value over rather than lend it.
//------------------------------------------------------------------------------
TEST(Result, HandsOverAValueThatCannotBeCopied)
{
    compages::gpu::Result<Unique> result{ std::make_unique<int>(42) };

    ASSERT_TRUE(bool(result));
    Unique taken = result.take();
    ASSERT_NE(taken, nullptr);
    ASSERT_EQ(*taken, 42);
}

//------------------------------------------------------------------------------
TEST(Status, SaysNothingMoreThanWhetherItWorked)
{
    ASSERT_TRUE(bool(compages::gpu::success()));
    ASSERT_TRUE(compages::gpu::success().error().empty());

    auto failed = check(false);
    ASSERT_FALSE(bool(failed));
    ASSERT_EQ(failed.error(), "the check did not pass");
}

//------------------------------------------------------------------------------
TEST(Status, StopsAtTheFirstStepThatFails)
{
    ASSERT_TRUE(bool(runSteps(true, true)));
    ASSERT_FALSE(bool(runSteps(false, true)));
    ASSERT_FALSE(bool(runSteps(true, false)));
}

//------------------------------------------------------------------------------
// A failure deep in a chain must reach the caller with the message of the step
// that actually failed, not a message invented on the way up.
//------------------------------------------------------------------------------
TEST(Result, ForwardsTheMessageOfTheStepThatFailed)
{
    auto result = quarter(7);
    ASSERT_FALSE(bool(result));
    ASSERT_EQ(result.error(), "not an even number: 7");

    // 10 halves to 5, which is the step that then fails.
    result = quarter(10);
    ASSERT_FALSE(bool(result));
    ASSERT_EQ(result.error(), "not an even number: 5");

    result = quarter(12);
    ASSERT_TRUE(bool(result));
    ASSERT_EQ(result.value(), 3);
}

//------------------------------------------------------------------------------
// A Result that is dropped without being looked at is a failure nobody handled,
// which is exactly the bug this type exists to prevent.
//------------------------------------------------------------------------------
TEST(Result, CannotBeIgnoredSilently)
{
    ASSERT_TRUE((std::is_same_v<compages::gpu::Result<int>, compages::Result<int>>));
    ASSERT_TRUE((std::is_same_v<decltype(half(2)), compages::gpu::Result<int>>));
    // [[nodiscard]] cannot be observed at runtime; what can be checked is that
    // reading a Result needs an explicit test, since the conversion to bool is
    // explicit and will not happen by accident in an arithmetic expression.
    ASSERT_FALSE((std::is_convertible_v<compages::gpu::Result<int>, bool>));
}
