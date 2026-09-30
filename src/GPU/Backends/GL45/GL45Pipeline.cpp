// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL45/GL45.hpp"

#include <bit>
#include <map>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Vertex array objects and render state, in OpenGL 4.5.
//!
//! A vertex array object holds two things that OpenGL keeps together but which
//! change at completely different rates: how to read a vertex, and which buffer
//! to read it from. Before 4.3 they could not be separated, which is why the
//! previous design ended up with one VAO per mesh per shader, and with a VAO
//! refusing to be used by a second program.
//!
//! Direct State Access splits them. glVertexArrayAttribFormat says how to read
//! a vertex and is set once, here. glVertexArrayVertexBuffer says where the
//! vertices are and is set at draw time. So one of these objects describes a
//! way of reading a vertex, full stop, and every pipeline reading a vertex that
//! way shares it, whatever program it draws with and whatever mesh it draws.
//!
//! That is what the cache below is: a way of reading a vertex, kept once, with
//! a count of how many pipelines want it.
// ****************************************************************************

namespace compages::gpu::backend
{

namespace
{

// ****************************************************************************
//! \brief What makes two ways of reading a vertex the same one.
//!
//! Everything that goes into the vertex array object and nothing else. The
//! buffer is deliberately absent, since it is bound at draw time; that absence
//! is the whole reason the sharing works.
// ****************************************************************************
struct ReaderKey
{
    std::uint32_t stride = 0u;
    std::vector<VertexAttribute> attributes;

    [[nodiscard]] friend bool operator<(ReaderKey const& p_left,
                                        ReaderKey const& p_right)
    {
        if (p_left.stride != p_right.stride)
        {
            return p_left.stride < p_right.stride;
        }
        if (p_left.attributes.size() != p_right.attributes.size())
        {
            return p_left.attributes.size() < p_right.attributes.size();
        }
        for (std::size_t i = 0u; i < p_left.attributes.size(); ++i)
        {
            VertexAttribute const& one = p_left.attributes[i];
            VertexAttribute const& other = p_right.attributes[i];

            if (one.location != other.location)
            {
                return one.location < other.location;
            }
            if (one.offset != other.offset)
            {
                return one.offset < other.offset;
            }
            if (one.per_instance != other.per_instance)
            {
                return static_cast<int>(one.per_instance) <
                       static_cast<int>(other.per_instance);
            }
            if (one.format.scalar != other.format.scalar)
            {
                return one.format.scalar < other.format.scalar;
            }
            if (one.format.components != other.format.components)
            {
                return one.format.components < other.format.components;
            }
            if (one.format.slots != other.format.slots)
            {
                return one.format.slots < other.format.slots;
            }
            if (one.format.normalized != other.format.normalized)
            {
                return static_cast<int>(one.format.normalized) <
                       static_cast<int>(other.format.normalized);
            }
            if (one.format.as_integer != other.format.as_integer)
            {
                return static_cast<int>(one.format.as_integer) <
                       static_cast<int>(other.format.as_integer);
            }
        }
        return false;
    }
};

// ****************************************************************************
//! \brief One vertex array object and how many pipelines want it.
// ****************************************************************************
struct Reader
{
    GLuint vao = 0u;
    std::size_t users = 0u;
};

//! \brief Every way of reading a vertex the backend currently holds.
std::map<ReaderKey, Reader>& readers()
{
    static std::map<ReaderKey, Reader> instance;
    return instance;
}

//! \brief Which vertex array object was bound last, so that binding the same
//! one twice costs nothing.
GLuint g_bound_vao = 0u;

//! \brief Which program was used last, for the same reason.
GLuint g_used_program = 0u;

//! \brief What state was applied last, so that only what differs is set again.
//! A frame drawing a hundred objects with two pipelines then makes two state
//! changes rather than a hundred.
RenderState g_applied;

//! \brief Has anything been applied yet? Until then nothing may be skipped,
//! since what the driver starts with is not what g_applied says.
bool g_state_known = false;

//! \brief Are filled polygons drawn as their edges, whatever the pipelines say?
bool g_wireframe = false;

//! \brief The kind of number, as OpenGL names it.
GLenum toGL(ScalarType p_type)
{
    switch (p_type)
    {
        case ScalarType::Float:
            return GL_FLOAT;
        case ScalarType::Half:
            return GL_HALF_FLOAT;
        case ScalarType::Double:
            return GL_DOUBLE;
        case ScalarType::Int8:
            return GL_BYTE;
        case ScalarType::UInt8:
            return GL_UNSIGNED_BYTE;
        case ScalarType::Int16:
            return GL_SHORT;
        case ScalarType::UInt16:
            return GL_UNSIGNED_SHORT;
        case ScalarType::Int32:
            return GL_INT;
        case ScalarType::UInt32:
            return GL_UNSIGNED_INT;
    }
    return GL_FLOAT;
}

//! \brief The comparison, as OpenGL names it.
GLenum toGL(CompareFunc p_func)
{
    switch (p_func)
    {
        case CompareFunc::Never:
            return GL_NEVER;
        case CompareFunc::Less:
            return GL_LESS;
        case CompareFunc::Equal:
            return GL_EQUAL;
        case CompareFunc::LessEqual:
            return GL_LEQUAL;
        case CompareFunc::Greater:
            return GL_GREATER;
        case CompareFunc::NotEqual:
            return GL_NOTEQUAL;
        case CompareFunc::GreaterEqual:
            return GL_GEQUAL;
        case CompareFunc::Always:
            return GL_ALWAYS;
    }
    return GL_LESS;
}

//! \brief The blend factor, as OpenGL names it.
GLenum toGL(BlendFactor p_factor)
{
    switch (p_factor)
    {
        case BlendFactor::Zero:
            return GL_ZERO;
        case BlendFactor::One:
            return GL_ONE;
        case BlendFactor::SrcColor:
            return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:
            return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:
            return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:
            return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
    }
    return GL_ONE;
}

//! \brief The blend equation, as OpenGL names it.
GLenum toGL(BlendEquation p_equation)
{
    switch (p_equation)
    {
        case BlendEquation::Add:
            return GL_FUNC_ADD;
        case BlendEquation::Subtract:
            return GL_FUNC_SUBTRACT;
        case BlendEquation::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case BlendEquation::Min:
            return GL_MIN;
        case BlendEquation::Max:
            return GL_MAX;
    }
    return GL_FUNC_ADD;
}

//! \brief The polygon mode, as OpenGL names it.
GLenum toGL(PolygonMode p_mode)
{
    switch (p_mode)
    {
        case PolygonMode::Fill:
            return GL_FILL;
        case PolygonMode::Line:
            return GL_LINE;
        case PolygonMode::Point:
            return GL_POINT;
    }
    return GL_FILL;
}

//! \brief Describe one field to the vertex array object.
//!
//! Three flavours of the same call, and choosing between them is the whole of
//! what ScalarType and the normalized flag are for. The wrong one does not
//! fail: it reads the same bytes and delivers different numbers, which is why
//! the check in Pipeline.cpp exists.
void describeField(GLuint p_vao, VertexAttribute const& p_attribute)
{
    const GLenum type = toGL(p_attribute.format.scalar);
    const GLuint base = static_cast<GLuint>(p_attribute.location);

    // A matrix is read one column per slot, each column sitting one column's
    // worth of bytes after the previous one.
    const std::uint32_t column_bytes =
        static_cast<std::uint32_t>(sizeOf(p_attribute.format.scalar)) *
        p_attribute.format.components;

    for (std::uint8_t slot = 0u; slot < p_attribute.format.slots; ++slot)
    {
        const GLuint where = base + slot;
        const GLuint offset = p_attribute.offset + (slot * column_bytes);

        glEnableVertexArrayAttrib(p_vao, where);

        if (p_attribute.format.as_integer)
        {
            // Whole numbers reaching the shader as whole numbers.
            glVertexArrayAttribIFormat(
                p_vao, where, p_attribute.format.components, type, offset);
        }
        else if (p_attribute.format.scalar == ScalarType::Double)
        {
            // Double precision has a call of its own: the others would silently
            // narrow it to a float.
            glVertexArrayAttribLFormat(
                p_vao, where, p_attribute.format.components, type, offset);
        }
        else
        {
            glVertexArrayAttribFormat(p_vao,
                                      where,
                                      p_attribute.format.components,
                                      type,
                                      p_attribute.format.normalized ? GL_TRUE
                                                                    : GL_FALSE,
                                      offset);
        }

        // Every field reads from the same buffer binding, since they are
        // interleaved in one buffer. That is the point of describing a vertex
        // as a struct.
        glVertexArrayAttribBinding(p_vao, where, 0u);
    }
}

} // namespace

namespace detail
{

void clearVertexArrayCache()
{
    for (auto const& entry : readers())
    {
        GLuint vao = entry.second.vao;
        glDeleteVertexArrays(1, &vao);
    }
    readers().clear();
    g_bound_vao = 0u;
    g_used_program = 0u;
    g_state_known = false;
}

} // namespace detail

Result<NativeId>
acquireVertexReader(std::span<const VertexAttribute> p_attributes,
                    std::uint32_t p_stride)
{
    ReaderKey key;
    key.stride = p_stride;
    key.attributes.assign(p_attributes.begin(), p_attributes.end());

    auto found = readers().find(key);
    if (found != readers().end())
    {
        ++found->second.users;
        return static_cast<NativeId>(found->second.vao);
    }

    GLuint vao = 0u;
    glCreateVertexArrays(1, &vao);
    if (vao == 0u)
    {
        return failure("the driver refused to create a vertex array object");
    }

    for (VertexAttribute const& attribute : p_attributes)
    {
        describeField(vao, attribute);
    }

    // A per instance field is read once per object drawn rather than once per
    // corner. Since every field shares binding zero, one field being per
    // instance makes them all so, which is why a per instance layout is
    // described on its own rather than mixed with a per vertex one.
    bool per_instance = false;
    for (VertexAttribute const& attribute : p_attributes)
    {
        per_instance = per_instance || attribute.per_instance;
    }
    glVertexArrayBindingDivisor(vao, 0u, per_instance ? 1u : 0u);

    readers().emplace(std::move(key), Reader{ vao, 1u });
    return static_cast<NativeId>(vao);
}

void releaseVertexReader(NativeId p_reader)
{
    const auto vao = static_cast<GLuint>(p_reader);

    for (auto it = readers().begin(); it != readers().end(); ++it)
    {
        if (it->second.vao != vao)
        {
            continue;
        }
        --it->second.users;
        if (it->second.users == 0u)
        {
            glDeleteVertexArrays(1, &vao);
            if (g_bound_vao == vao)
            {
                g_bound_vao = 0u;
            }
            readers().erase(it);
        }
        return;
    }
}

std::size_t vertexReadersHeld()
{
    return readers().size();
}

void forgetRenderState()
{
    // Not restoring anything, and not reading the device to find out what it
    // holds either. Both would cost more than the one thing that is needed: the
    // next pipeline must send all of its state instead of trusting these
    // variables.
    g_state_known = false;
    g_used_program = 0u;
    g_bound_vao = 0u;
}

void showWireframe(bool p_enabled)
{
    g_wireframe = p_enabled;
    g_state_known = false;
}

bool wireframeShown()
{
    return g_wireframe;
}

void bindPipeline(NativeId p_program,
                  NativeId p_reader,
                  RenderState const& p_state)
{
    RenderState state = p_state;
    if (g_wireframe && (state.polygon == PolygonMode::Fill))
    {
        state.polygon = PolygonMode::Line;
    }

    const auto program = static_cast<GLuint>(p_program);
    if (program != g_used_program)
    {
        glUseProgram(program);
        g_used_program = program;
    }

    const auto vao = static_cast<GLuint>(p_reader);
    if (vao != g_bound_vao)
    {
        glBindVertexArray(vao);
        g_bound_vao = vao;
    }

    // Everything below is skipped when it already holds, but only once
    // something has been applied at all: the state the driver starts a frame
    // with is not ours to assume.
    const bool everything = !g_state_known;

    if (everything || (state.depth_test != g_applied.depth_test))
    {
        if (state.depth_test)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }
    if (everything || (state.depth_write != g_applied.depth_write))
    {
        glDepthMask(state.depth_write ? GL_TRUE : GL_FALSE);
    }
    if (everything || (state.depth_func != g_applied.depth_func))
    {
        glDepthFunc(toGL(state.depth_func));
    }

    if (everything || !(state.blend == g_applied.blend))
    {
        if (state.blend.enabled)
        {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(toGL(state.blend.source_color),
                                toGL(state.blend.destination_color),
                                toGL(state.blend.source_alpha),
                                toGL(state.blend.destination_alpha));
            glBlendEquationSeparate(toGL(state.blend.color_equation),
                                    toGL(state.blend.alpha_equation));
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    if (everything || (state.cull != g_applied.cull))
    {
        switch (state.cull)
        {
            case CullMode::None:
                glDisable(GL_CULL_FACE);
                break;
            case CullMode::Back:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                break;
            case CullMode::Front:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                break;
        }
    }
    if (everything || (state.front_face != g_applied.front_face))
    {
        glFrontFace((state.front_face == FrontFace::CounterClockwise) ? GL_CCW
                                                                      : GL_CW);
    }

    if (everything || (state.polygon != g_applied.polygon))
    {
        glPolygonMode(GL_FRONT_AND_BACK, toGL(state.polygon));
    }

    if (everything || !(state.color_mask == g_applied.color_mask))
    {
        glColorMask(state.color_mask.red ? GL_TRUE : GL_FALSE,
                    state.color_mask.green ? GL_TRUE : GL_FALSE,
                    state.color_mask.blue ? GL_TRUE : GL_FALSE,
                    state.color_mask.alpha ? GL_TRUE : GL_FALSE);
    }

    if (everything || (std::bit_cast<std::uint32_t>(state.line_width) !=
                       std::bit_cast<std::uint32_t>(g_applied.line_width)))
    {
        // A core profile driver is allowed to refuse any width but one, and
        // several do. Asked for anyway, because it works on most desktop
        // drivers and a wireframe view is where it matters; the driver's own
        // message says so when it does not.
        glLineWidth(state.line_width);
    }

    if (everything || (state.primitive != g_applied.primitive))
    {
        // A point whose size comes from the vertex shader is invisible until
        // this is on: the default size is one pixel, and gl_PointSize is
        // ignored.
        if (state.primitive == Primitive::Points)
        {
            glEnable(GL_PROGRAM_POINT_SIZE);
        }
        else
        {
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    g_applied = state;
    g_state_known = true;
}

void useProgram(NativeId p_program)
{
    const auto program = static_cast<GLuint>(p_program);
    if (program != g_used_program)
    {
        glUseProgram(program);
        g_used_program = program;
    }
}

} // namespace compages::gpu::backend
