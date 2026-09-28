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

#include "Compages/Core/Vector.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace compages::world
{

// ****************************************************************************
//! \file
//! \brief Logical keys and mouse state for camera controllers and behaviors.
//!
//! Nothing here includes GLFW: \c KeyMap stores native key codes (GLFW in the
//! gallery) and fills \c Input from a \c keyDown(int) callback.
// ****************************************************************************

// ****************************************************************************
//! \brief Keys the examples and controllers understand, independent of layout.
//!
//! \code
//! if (input.down(compages::world::Key::W)) { /* move forward */ }
//! \endcode
// ****************************************************************************
//! \brief Gallery logical keys (WASD, arrows, …), not scancodes.
enum class Key : std::uint8_t
{
    W,
    A,
    S,
    D,
    Q,
    E,
    Shift,
    Space,
    D1,
    D2,
    D3,
    R,
    F,
    L,
    Up,
    Down,
    Left,
    Right,
    Count
};

namespace detail
{

inline constexpr std::size_t keyCount()
{
    return static_cast<std::size_t>(Key::Count);
}

//! \brief Default GLFW key codes in \c Key enum order (W, A, S, …).
inline constexpr std::array<int, keyCount()> defaultNativeKeyCodes()
{
    return { 87,  65,  83,  68,  81,  69,  340, 32,  49,  50,  51,
             82,  70,  76,  265, 264, 263, 262 };
}

inline constexpr int glfwLeftShift = 340;
inline constexpr int glfwRightShift = 344;

} // namespace detail

// ****************************************************************************
//! \brief Mouse and keyboard as they were during one frame.
//!
//! \code
//! compages::world::Input in;
//! in.set(compages::world::Key::Space, true);
//! if (in.mouse_left_pressed) { /* pick */ }
//! \endcode
// ****************************************************************************
struct Input
{
    //! \brief Cursor in pixels, origin bottom-left (OpenGL convention).
    Vector2f mouse{ 0.0f, 0.0f };
    //! \brief Pointer motion this frame.
    Vector2f mouse_delta{ 0.0f, 0.0f };
    //! \brief Scroll wheel delta (positive = zoom out in orbit controls).
    float scroll = 0.0f;
    //! \brief Pointer is over the render viewport.
    bool mouse_over = false;
    bool mouse_left = false;
    bool mouse_right = false;
    //! \brief Left button transitioned to down this frame.
    bool mouse_left_pressed = false;
    //! \brief Relative mode: all motion counts as look.
    bool mouse_captured = false;

    std::array<bool, detail::keyCount()> keys{};

    //! \brief Was \c p_key held this frame?
    [[nodiscard]] bool down(Key p_key) const
    {
        return keys[static_cast<std::size_t>(p_key)];
    }

    //! \brief Set one logical key (usually via \c KeyMap::apply).
    void set(Key p_key, bool p_value)
    {
        keys[static_cast<std::size_t>(p_key)] = p_value;
    }
};

// ****************************************************************************
//! \brief True when \c p_key went down between \c p_before and \c p_now.
//!
//! \code
//! if (compages::world::pressed(frame.input, m_prev, compages::world::Key::Space)) { restart(); }
//! \endcode
// ****************************************************************************
[[nodiscard]] inline bool pressed(Input const& p_now, Input const& p_before, Key p_key)
{
    return p_now.down(p_key) && !p_before.down(p_key);
}

// ****************************************************************************
//! \brief Maps each \c Key to a native key code (GLFW in the gallery).
//!
//! \code
//! compages::world::KeyMap map = compages::world::KeyMap::defaults();
//! map.codes[static_cast<std::size_t>(compages::world::Key::R)] = myReloadKey;
//! map.apply(input, [&](int code) { return window.keyDown(code); });
//! \endcode
// ****************************************************************************
struct KeyMap
{
    //! \brief Native key code per \c Key (GLFW values in \c defaults()).
    std::array<int, detail::keyCount()> codes = detail::defaultNativeKeyCodes();

    //! \brief WASD and gallery bindings for GLFW.
    [[nodiscard]] static KeyMap defaults()
    {
        return KeyMap{};
    }

    //! \brief Fill \c p_input.keys from a \c keyDown(nativeCode) predicate.
    template<typename KeyDownFn>
    void apply(Input& p_input, KeyDownFn&& p_key_down) const
    {
        for (std::size_t i = 0u; i < detail::keyCount(); ++i)
        {
            const Key key = static_cast<Key>(i);
            bool held = p_key_down(codes[i]);
            if (key == Key::Shift)
            {
                held = held || p_key_down(detail::glfwLeftShift) ||
                       p_key_down(detail::glfwRightShift);
            }
            p_input.set(key, held);
        }
    }
};

} // namespace compages::world
