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

#include "00_GettingStarted/06b_ComplexShader.hpp"

#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/RenderPass.hpp"

namespace examples
{

constexpr char const* VERTEX = R"(#version 450 core
in vec2 position;
in vec2 uv;
out vec2 vUv;
void main()
{
    vUv = uv;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// Port of legacy 11_ComplexShader.fs (ShaderFrog Universe Nursery).
constexpr char const* FRAGMENT = R"(#version 450 core
in vec2 vUv;
uniform vec3 color;
uniform float time;
uniform float twinkleSpeed;
uniform float speed;
uniform float brightness;
uniform float distfading;
out vec4 oColor;

void main()
{
    // Drift the sample so the field scrolls with time.
    vec2 uv = vUv + 0.5;
    uv.x += time * speed * 0.1;

    vec3 dir = vec3(uv * 2.0, 1.0);
    float s = 0.1;
    float fade = 0.01;
    vec3 starColor = vec3(0.0);

    // Three layers. The inner loop folds space into a star; fade and s
    // step the next layer further back.
    for (int r = 0; r < 3; ++r)
    {
        vec3 p = (time * speed * twinkleSpeed) + dir * (s * 0.5);
        p = abs(vec3(1.3) - mod(p, vec3(2.6)));

        float prevLen = 0.0;
        float a = 0.0;
        for (int i = 0; i < 17; ++i)
        {
            p = abs(p);
            p = p * (1.0 / dot(p, p)) + vec3(-0.5);
            float len = length(p);
            a += abs(len - prevLen);
            prevLen = len;
        }

        a = a * a * a;
        starColor += (vec3(s, s * s, s * s * s) * a * brightness + 1.0) * fade;
        fade *= distfading;
        s += 0.2;
    }

    starColor = min(starColor, vec3(1.2));

    // Soften the bright edges, or the stars alias into lines.
    float intensity = min(starColor.r + starColor.g + starColor.b, 0.7);
    vec2 sgn = vUv * 2.0 - 1.0;
    vec2 gradient = vec2(dFdx(intensity) * sgn.x, dFdy(intensity) * sgn.y);
    float cutoff = max(max(gradient.x, gradient.y) - 0.1, 0.0);
    starColor *= max(1.0 - cutoff * 6.0, 0.3);

    oColor = vec4(starColor * color, 1.0);
}
)";

//------------------------------------------------------------------------------
std::string ComplexShader::description() const
{
    return "Legacy 11_ComplexShader: ShaderFrog Universe Nursery starfield on "
           "a fullscreen quad.";
}

//------------------------------------------------------------------------------
compages::Status ComplexShader::setUp()
{
    COMPAGES_TRY(m_quad.load(VERTEX, FRAGMENT));

    // A quad covering the window. The starfield is entirely in the fragment
    // shader; these vertices only say where the picture is.
    m_quad["position"] = { { 1, 1 }, { 1, -1 }, { -1, -1 }, { -1, 1 } };
    m_quad["uv"] = { { 1, 1 }, { 1, 0 }, { 0, 0 }, { 0, 1 } };
    m_quad.indices({ 0, 1, 3, 1, 2, 3 });

    // The knobs of the original shader, left as uniforms so a frame can
    // change them. Only time does, below.
    m_quad["color"] = compages::core::Vector3f(1.0f, 1.0f, 1.0f);
    m_quad["speed"] = 0.0001f;
    m_quad["brightness"] = 0.0018f;
    m_quad["distfading"] = 0.7f;
    m_quad["twinkleSpeed"] = 200.0f;
    return m_quad.prepare();
}

//------------------------------------------------------------------------------
void ComplexShader::draw(compages::world::ViewFrame const& p_frame)
{
    // The whole animation is this uniform. The quad itself never moves.
    m_quad["time"] = p_frame.total;
    m_quad.draw();
}

} // namespace examples
