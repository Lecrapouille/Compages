// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Applies a macro to each name in a list.
//!
//! Needed by GPU_STD140, which takes the name of a struct followed by the names
//! of its members. A member name is
//! not a value, so it cannot be passed to a function: turning a list of names
//! into a list of expressions is something only the preprocessor can do.
//!
//! Sixteen names is the limit. A vertex with more than sixteen fields is beyond
//! what any hardware accepts as vertex attributes anyway, and a uniform block
//! with more than sixteen members is better off split.
// ****************************************************************************

//! \brief Glue two tokens together, after letting both be expanded.
#define GPU_PP_CONCAT(a, b) GPU_PP_CONCAT_IMPL(a, b)
#define GPU_PP_CONCAT_IMPL(a, b) a##b

//! \brief How many arguments were given, from 1 to 16.
#define GPU_PP_COUNT(...)                                                  \
    GPU_PP_COUNT_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, \
                      5, 4, 3, 2, 1)
#define GPU_PP_COUNT_IMPL(_1,                                              \
                          _2,                                              \
                          _3,                                              \
                          _4,                                              \
                          _5,                                              \
                          _6,                                              \
                          _7,                                              \
                          _8,                                              \
                          _9,                                              \
                          _10,                                             \
                          _11,                                             \
                          _12,                                             \
                          _13,                                             \
                          _14,                                             \
                          _15,                                             \
                          _16,                                             \
                          N,                                               \
                          ...)                                             \
    N

// ----------------------------------------------------------------------------
// One expansion per possible number of names. Each produces a comma separated
// list, which is what both callers want: the arguments of a function call.
// ----------------------------------------------------------------------------
#define GPU_PP_EACH_1(m, ctx, a) m(ctx, a)
#define GPU_PP_EACH_2(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_1(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_3(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_2(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_4(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_3(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_5(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_4(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_6(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_5(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_7(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_6(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_8(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_7(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_9(m, ctx, a, ...) m(ctx, a), GPU_PP_EACH_8(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_10(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_9(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_11(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_10(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_12(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_11(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_13(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_12(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_14(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_13(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_15(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_14(m, ctx, __VA_ARGS__)
#define GPU_PP_EACH_16(m, ctx, a, ...) \
    m(ctx, a), GPU_PP_EACH_15(m, ctx, __VA_ARGS__)

// ----------------------------------------------------------------------------
//! \brief Expand m(ctx, name) for each name, separated by commas.
//!
//! \param m the macro to apply. It receives the context first, then one name.
//! \param ctx passed unchanged to every expansion, in practice the name of the
//! struct the members belong to.
// ----------------------------------------------------------------------------
#define GPU_PP_FOR_EACH(m, ctx, ...) \
    GPU_PP_CONCAT(GPU_PP_EACH_, GPU_PP_COUNT(__VA_ARGS__))(m, ctx, __VA_ARGS__)
