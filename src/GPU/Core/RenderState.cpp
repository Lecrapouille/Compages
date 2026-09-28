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

#include "Compages/GPU/Core/RenderState.hpp"

namespace compages::gpu
{

//------------------------------------------------------------------------------
const char* toString(CompareFunc p_func)
{
    switch (p_func)
    {
        case CompareFunc::Never:
            return "never";
        case CompareFunc::Less:
            return "less";
        case CompareFunc::Equal:
            return "equal";
        case CompareFunc::LessEqual:
            return "less or equal";
        case CompareFunc::Greater:
            return "greater";
        case CompareFunc::NotEqual:
            return "not equal";
        case CompareFunc::GreaterEqual:
            return "greater or equal";
        case CompareFunc::Always:
            return "always";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
const char* toString(BlendFactor p_factor)
{
    switch (p_factor)
    {
        case BlendFactor::Zero:
            return "zero";
        case BlendFactor::One:
            return "one";
        case BlendFactor::SrcColor:
            return "source colour";
        case BlendFactor::OneMinusSrcColor:
            return "one minus source colour";
        case BlendFactor::DstColor:
            return "destination colour";
        case BlendFactor::OneMinusDstColor:
            return "one minus destination colour";
        case BlendFactor::SrcAlpha:
            return "source alpha";
        case BlendFactor::OneMinusSrcAlpha:
            return "one minus source alpha";
        case BlendFactor::DstAlpha:
            return "destination alpha";
        case BlendFactor::OneMinusDstAlpha:
            return "one minus destination alpha";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
const char* toString(BlendEquation p_equation)
{
    switch (p_equation)
    {
        case BlendEquation::Add:
            return "add";
        case BlendEquation::Subtract:
            return "subtract";
        case BlendEquation::ReverseSubtract:
            return "reverse subtract";
        case BlendEquation::Min:
            return "min";
        case BlendEquation::Max:
            return "max";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
const char* toString(CullMode p_mode)
{
    switch (p_mode)
    {
        case CullMode::None:
            return "none";
        case CullMode::Back:
            return "back faces";
        case CullMode::Front:
            return "front faces";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
const char* toString(FrontFace p_face)
{
    switch (p_face)
    {
        case FrontFace::CounterClockwise:
            return "counter clockwise";
        case FrontFace::Clockwise:
            return "clockwise";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
const char* toString(PolygonMode p_mode)
{
    switch (p_mode)
    {
        case PolygonMode::Fill:
            return "fill";
        case PolygonMode::Line:
            return "lines";
        case PolygonMode::Point:
            return "points";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
const char* toString(Primitive p_primitive)
{
    switch (p_primitive)
    {
        case Primitive::Points:
            return "points";
        case Primitive::Lines:
            return "lines";
        case Primitive::LineStrip:
            return "line strip";
        case Primitive::LineLoop:
            return "line loop";
        case Primitive::Triangles:
            return "triangles";
        case Primitive::TriangleStrip:
            return "triangle strip";
        case Primitive::TriangleFan:
            return "triangle fan";
    }
    return "unknown";
}

//------------------------------------------------------------------------------
std::string Blend::toString() const
{
    if (!enabled)
    {
        return "off";
    }
    // Naming the preset when the numbers match one is worth the comparisons: a
    // reader who wrote Blend::alpha() should read it back, not six factors.
    if (*this == Blend::alpha())
    {
        return "alpha";
    }
    if (*this == Blend::premultiplied())
    {
        return "premultiplied alpha";
    }
    if (*this == Blend::additive())
    {
        return "additive";
    }
    if (*this == Blend::multiply())
    {
        return "multiply";
    }

    return std::string("colour ") + compages::gpu::toString(source_color) + " " +
           compages::gpu::toString(color_equation) + " " +
           compages::gpu::toString(destination_color) + ", alpha " +
           compages::gpu::toString(source_alpha) + " " + compages::gpu::toString(alpha_equation) +
           " " + compages::gpu::toString(destination_alpha);
}

//------------------------------------------------------------------------------
std::string RenderState::toString() const
{
    std::string text = std::string("primitive: ") + compages::gpu::toString(primitive);

    text += "\ndepth test: ";
    if (depth_test)
    {
        text += std::string("on, ") + compages::gpu::toString(depth_func);
        text += depth_write ? ", writing" : ", not writing";
    }
    else
    {
        text += depth_write ? "off" : "off, not writing";
    }

    text += "\nblend: " + blend.toString();
    text += std::string("\ncull: ") + compages::gpu::toString(cull);
    if (cull != CullMode::None)
    {
        text += std::string(", front is ") + compages::gpu::toString(front_face);
    }
    text += std::string("\npolygon: ") + compages::gpu::toString(polygon);

    if (!(color_mask == ColorMask{}))
    {
        text += "\ncolour mask:";
        text += color_mask.red ? " red" : "";
        text += color_mask.green ? " green" : "";
        text += color_mask.blue ? " blue" : "";
        text += color_mask.alpha ? " alpha" : "";
        if (color_mask == ColorMask::none())
        {
            text += " nothing, so this pass writes depth only";
        }
    }

    return text;
}

} // namespace compages::gpu
