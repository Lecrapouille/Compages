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

#include "Compages/GPU/Drawable.hpp"

#include <algorithm>
#include <span>

// ****************************************************************************
//! \file
//! \brief A few numbers drawn as bars, for the examples whose data is a
//! handful of integers: what the GPU holds, shown rather than printed.
// ****************************************************************************

namespace examples
{

// ****************************************************************************
//! \brief Bars rising from the bottom of the picture, one per value.
// ****************************************************************************
class BarChart
{
public:

    //! \brief Compile the shader. Once, in the setUp() of the example.
    [[nodiscard]] compages::Status setUp()
    {
        return m_bars.load(VERTEX_SHADER, FRAGMENT_SHADER);
    }

    // ------------------------------------------------------------------------
    //! \brief Draw \c p_values, the tallest bar being \c p_top high. A bar
    //! whose index is in [p_first_hot, p_first_hot + p_hot_count) is drawn
    //! orange: what an example wants the eye to go to.
    // ------------------------------------------------------------------------
    void draw(std::span<const int> p_values,
              int p_top,
              std::size_t p_first_hot = 0u,
              std::size_t p_hot_count = 0u)
    {
        // Two triangles per bar, rebuilt every frame: a few dozen vertices,
        // cheaper to send again than to keep track of.
        m_bars.clear();
        const float width =
            1.8f / float(std::max<std::size_t>(p_values.size(), 1u));
        for (std::size_t i = 0u; i < p_values.size(); ++i)
        {
            const bool hot =
                (i >= p_first_hot) && (i < p_first_hot + p_hot_count);
            const compages::core::Vector3f color =
                hot ? compages::core::Vector3f(1.0f, 0.62f, 0.25f)
                    : compages::core::Vector3f(0.35f, 0.62f, 0.95f);
            const float left = -0.9f + (float(i) * width) + (width * 0.1f);
            const float right = left + (width * 0.8f);
            const float top = -0.8f + (1.6f * float(std::max(p_values[i], 0)) /
                                       float(std::max(p_top, 1)));
            m_bars.emplace_back(Vertex{ { left, -0.8f }, color });
            m_bars.emplace_back(Vertex{ { right, -0.8f }, color });
            m_bars.emplace_back(Vertex{ { right, top }, color });
            m_bars.emplace_back(Vertex{ { left, -0.8f }, color });
            m_bars.emplace_back(Vertex{ { right, top }, color });
            m_bars.emplace_back(Vertex{ { left, top }, color });
        }
        compages::gpu::clear({ 0.08f, 0.09f, 0.11f });
        if (m_bars.count() != 0u)
        {
            m_bars.draw();
        }
    }

private:

    struct Vertex
    {
        compages::core::Vector2f position;
        compages::core::Vector3f color;
    };

    static constexpr char const* VERTEX_SHADER = R"(#version 450 core
in vec2 position;
in vec3 color;
out vec3 vColor;
void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

    static constexpr char const* FRAGMENT_SHADER = R"(#version 450 core
in vec3 vColor;
out vec4 oColor;
void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

    compages::gpu::Drawable m_bars;
};

} // namespace examples
