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

#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

// ****************************************************************************
//! \file
//! \brief The names, types and positions of the fields of a plain struct,
//! found by the compiler rather than written down.
//!
//! This is what lets a vertex struct feed a shader with no macro and no list of
//! fields:
//!
//! \code
//! struct Vertex
//! {
//!     compages::core::Vector3f position;
//!     compages::core::Vector2f uv;
//! };
//!
//! static_assert(compages::gpu::reflect::fieldCount<Vertex>() == 2u);
//! compages::gpu::reflect::fieldName<Vertex, 0u>();   // "position"
//! compages::gpu::reflect::fieldName<Vertex, 1u>();   // "uv"
//! \endcode
//!
//! C++20 has no reflection, so this uses three facts of the language, the same
//! ones Boost.PFR relies on, written here in a hundred lines rather than taken
//! as a dependency:
//!
//! 1. **How many fields.** An aggregate can be brace initialized with at most
//!    as many values as it has fields. Trying with more and more values that
//!    convert to anything finds the count.
//! 2. **Which types, and where.** A structured binding with that many names
//!    yields a reference to each field, hence its type and its address.
//! 3. **Which names.** A pointer to a field can be a template argument, and
//!    GCC, Clang and MSVC all spell that argument out, field name included, in
//!    the name of the function they are compiling (__PRETTY_FUNCTION__ or
//!    __FUNCSIG__). The name is cut out of that text at compile time.
//!
//! What it asks of the struct, each refused by a static_assert that says so:
//! an aggregate (no constructor), no base class, every field public, no C
//! array and no reference field, at most MAX_FIELDS fields, and declared at
//! namespace or class scope rather than inside a function.
//!
//! \note The day C++26 reflection (std::meta) is everywhere, only this file
//! changes: callers ask for fieldCount(), fieldName() and forEachField() and
//! never see how they are answered.
// ****************************************************************************

namespace compages::gpu::reflect
{

//! \brief The most fields a reflected struct may have. A vertex has a
//! handful; sixteen is also the number of attribute slots the hardware
//! guarantees, so a vertex with more could not be read anyway.
constexpr std::size_t MAX_FIELDS = 16u;

namespace detail
{

// ----------------------------------------------------------------------------
//! \brief A value that converts to anything, used to probe how many values a
//! struct accepts between braces. Never evaluated.
// ----------------------------------------------------------------------------
struct AnyField
{
    template <typename T>
    constexpr operator T() const noexcept; // NOLINT: declared only.
};

// ----------------------------------------------------------------------------
//! \brief Can T be brace initialized with N values?
// ----------------------------------------------------------------------------
template <typename T, std::size_t... I>
constexpr bool initializableWith(std::index_sequence<I...>)
{
    return requires { T{ (static_cast<void>(I), AnyField{})... }; };
}

// ----------------------------------------------------------------------------
//! \brief Count by trying one more value until the braces refuse it.
// ----------------------------------------------------------------------------
template <typename T, std::size_t N = 0u>
constexpr std::size_t countFields()
{
    if constexpr (N > MAX_FIELDS)
    {
        return N;
    }
    else if constexpr (initializableWith<T>(std::make_index_sequence<N + 1u>{}))
    {
        return countFields<T, N + 1u>();
    }
    else
    {
        return N;
    }
}

// ----------------------------------------------------------------------------
//! \brief An object of type T that is never defined, only named.
//!
//! Its fields have addresses the compiler can reason about at compile time,
//! which is all a pointer template argument needs. Reading it would fail to
//! link, which is why nothing does.
// ----------------------------------------------------------------------------
template <typename T>
struct Wrapper
{
    T const value;
};

template <typename T>
extern Wrapper<T> const FAKE;

// ----------------------------------------------------------------------------
//! \brief A tuple of references to the fields of p_object, in order.
// ----------------------------------------------------------------------------
template <std::size_t N, typename T>
constexpr auto tie(T& p_object) noexcept
{
    // One branch per count, since a structured binding needs its names
    // written out. Generated rather than typed, to keep the sixteen in step.
#define COMPAGES_REFLECT_TIE(n, ...)                                         \
    if constexpr (N == n)                                                    \
    {                                                                        \
        auto& [__VA_ARGS__] = p_object;                                      \
        return std::tie(__VA_ARGS__);                                        \
    }                                                                        \
    else
    COMPAGES_REFLECT_TIE(1u, a)
    COMPAGES_REFLECT_TIE(2u, a, b)
    COMPAGES_REFLECT_TIE(3u, a, b, c)
    COMPAGES_REFLECT_TIE(4u, a, b, c, d)
    COMPAGES_REFLECT_TIE(5u, a, b, c, d, e)
    COMPAGES_REFLECT_TIE(6u, a, b, c, d, e, f)
    COMPAGES_REFLECT_TIE(7u, a, b, c, d, e, f, g)
    COMPAGES_REFLECT_TIE(8u, a, b, c, d, e, f, g, h)
    COMPAGES_REFLECT_TIE(9u, a, b, c, d, e, f, g, h, i)
    COMPAGES_REFLECT_TIE(10u, a, b, c, d, e, f, g, h, i, j)
    COMPAGES_REFLECT_TIE(11u, a, b, c, d, e, f, g, h, i, j, k)
    COMPAGES_REFLECT_TIE(12u, a, b, c, d, e, f, g, h, i, j, k, l)
    COMPAGES_REFLECT_TIE(13u, a, b, c, d, e, f, g, h, i, j, k, l, m)
    COMPAGES_REFLECT_TIE(14u, a, b, c, d, e, f, g, h, i, j, k, l, m, n_)
    COMPAGES_REFLECT_TIE(15u, a, b, c, d, e, f, g, h, i, j, k, l, m, n_, o)
    COMPAGES_REFLECT_TIE(16u, a, b, c, d, e, f, g, h, i, j, k, l, m, n_, o, p)
    {
        static_assert(N == 0u, "unreachable: fieldCount() is checked first");
        return std::tuple<>{};
    }
#undef COMPAGES_REFLECT_TIE
}

// ----------------------------------------------------------------------------
//! \brief The text of the compiler naming this function, with the field that
//! P points to spelled out somewhere in it.
// ----------------------------------------------------------------------------
template <auto P>
consteval std::string_view signature()
{
#if defined(_MSC_VER) && !defined(__clang__)
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

// ----------------------------------------------------------------------------
//! \brief Is this character part of a C++ identifier?
// ----------------------------------------------------------------------------
consteval bool isIdentifier(char p_char)
{
    return ((p_char >= 'a') && (p_char <= 'z')) ||
           ((p_char >= 'A') && (p_char <= 'Z')) ||
           ((p_char >= '0') && (p_char <= '9')) || (p_char == '_');
}

// ----------------------------------------------------------------------------
//! \brief The last identifier of a text, found by walking back from the end.
// ----------------------------------------------------------------------------
consteval std::string_view lastIdentifier(std::string_view p_text)
{
    std::size_t end = p_text.size();
    while ((end > 0u) && !isIdentifier(p_text[end - 1u]))
    {
        --end;
    }
    std::size_t begin = end;
    while ((begin > 0u) && isIdentifier(p_text[begin - 1u]))
    {
        --begin;
    }
    return p_text.substr(begin, end - begin);
}

// ----------------------------------------------------------------------------
//! \brief Cut the field name out of the signature.
//!
//! The signature holds the pointer argument, and every compiler ends that
//! argument with the field name:
//! - GCC: "[with auto P = (& FAKE<V>.Wrapper<V>::value.V::position); ...]"
//! - Clang: "[P = &FAKE<V>.value.position]"
//! - MSVC: "signature<&FAKE<V>.value->position>(void)"
//!
//! So the argument is isolated first, up to the bracket that closes it or the
//! semicolon GCC puts after it, and the name is its last identifier.
// ----------------------------------------------------------------------------
consteval std::string_view fieldNameIn(std::string_view p_signature)
{
    std::size_t start = p_signature.find("P = ");
    if (start != std::string_view::npos)
    {
        start += 4u;
    }
    else
    {
        start = p_signature.find("signature<");
        start = (start == std::string_view::npos) ? 0u : start + 10u;
    }

    int depth = 0;
    std::size_t end = start;
    for (; end < p_signature.size(); ++end)
    {
        const char c = p_signature[end];
        if ((c == '-') && (end + 1u < p_signature.size()) &&
            (p_signature[end + 1u] == '>'))
        {
            ++end; // "->" is a member access, not a closing bracket.
        }
        else if ((c == '(') || (c == '<') || (c == '['))
        {
            ++depth;
        }
        else if ((c == ')') || (c == '>') || (c == ']'))
        {
            if (depth == 0)
            {
                break;
            }
            --depth;
        }
        else if ((c == ';') && (depth == 0))
        {
            break;
        }
    }
    return lastIdentifier(p_signature.substr(start, end - start));
}

// ----------------------------------------------------------------------------
//! \brief Everything reflection asks of T, each with its own sentence.
// ----------------------------------------------------------------------------
template <typename T>
constexpr bool checkReflectable()
{
    static_assert(std::is_aggregate_v<T>,
                  "this struct cannot be reflected: it must be an aggregate, "
                  "that is have no constructor, no virtual function and no "
                  "private field. A vertex is plain data; give it none of "
                  "those");
    static_assert(std::is_standard_layout_v<T>,
                  "this struct cannot be reflected: it must be standard layout, "
                  "which rules out a base class holding fields and fields of "
                  "mixed access");
    static_assert(countFields<T>() <= MAX_FIELDS,
                  "this struct has more fields than can be reflected: sixteen "
                  "at most, which is also as many attributes as a vertex can "
                  "have");
    return true;
}

} // namespace detail

// ----------------------------------------------------------------------------
//! \brief How many fields T has.
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] consteval std::size_t fieldCount()
{
    static_assert(detail::checkReflectable<T>());
    return detail::countFields<T>();
}

// ----------------------------------------------------------------------------
//! \brief The type of field I of T.
// ----------------------------------------------------------------------------
template <typename T, std::size_t I>
using FieldType = std::remove_cvref_t<std::tuple_element_t<
    I,
    decltype(detail::tie<fieldCount<T>()>(std::declval<T&>()))>>;

// ----------------------------------------------------------------------------
//! \brief The name of field I of T, as written in the struct.
//!
//! \code
//! struct Vertex { compages::core::Vector3f position; compages::core::Vector2f uv; };
//! static_assert(compages::gpu::reflect::fieldName<Vertex, 1u>() == "uv");
//! \endcode
// ----------------------------------------------------------------------------
template <typename T, std::size_t I>
[[nodiscard]] consteval std::string_view fieldName()
{
    static_assert(I < fieldCount<T>(), "there is no field of this index");
    return detail::fieldNameIn(detail::signature<&std::get<I>(
        detail::tie<fieldCount<T>()>(detail::FAKE<T>.value))>());
}

// ----------------------------------------------------------------------------
//! \brief Where field I of p_object starts, in bytes from the start of it.
//!
//! Measured on a real object, since subtracting addresses is not something a
//! compiler agrees to do at compile time.
// ----------------------------------------------------------------------------
template <std::size_t I, typename T>
[[nodiscard]] std::size_t fieldOffset(T const& p_object)
{
    auto const& field = std::get<I>(detail::tie<fieldCount<T>()>(p_object));
    return static_cast<std::size_t>(
        reinterpret_cast<const unsigned char*>(&field) -
        reinterpret_cast<const unsigned char*>(&p_object));
}

// ----------------------------------------------------------------------------
//! \brief Call p_visit once per field of T, in order.
//!
//! The visitor is given the index as a std::integral_constant, from which it
//! can ask for the name, the type and the offset:
//!
//! \code
//! compages::gpu::reflect::forEachField<Vertex>([&](auto p_index) {
//!     constexpr std::size_t I = decltype(p_index)::value;
//!     using Type = compages::gpu::reflect::FieldType<Vertex, I>;
//!     std::cout << compages::gpu::reflect::fieldName<Vertex, I>() << ": "
//!               << sizeof(Type) << " bytes" << std::endl;
//! });
//! \endcode
// ----------------------------------------------------------------------------
template <typename T, typename F>
void forEachField(F&& p_visit)
{
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (p_visit(std::integral_constant<std::size_t, I>{}), ...);
    }(std::make_index_sequence<fieldCount<T>()>{});
}

} // namespace compages::gpu::reflect
