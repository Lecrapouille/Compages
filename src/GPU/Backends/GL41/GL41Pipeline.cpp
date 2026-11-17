// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Vertex array objects and render state, in OpenGL 4.1 / ES 3.0.
//!
//! The GL45 backend shares one vertex array per way of reading a vertex,
//! and gives the buffer at draw time (glVertexArrayVertexBuffer, from the
//! separate vertex formats of 4.3). Here the two are tied: glVertexAttribPointer
//! says how to read a field *and* from which buffer. The sharing is kept, so
//! the contract holds (vertexReadersHeld() is the same on both backends): each
//! vertex array remembers its fields, and bindVertexBuffer() points them at the
//! buffer again when the buffer changes.
// ****************************************************************************

namespace compages::gpu::backend
{

namespace
{

auto tied(VertexAttribute const& p_attribute)
{
    return std::make_tuple(p_attribute.location,
                           p_attribute.offset,
                           p_attribute.per_instance,
                           p_attribute.format.scalar,
                           p_attribute.format.components,
                           p_attribute.format.slots,
                           p_attribute.format.normalized,
                           p_attribute.format.as_integer);
}

struct ReaderKey
{
    std::uint32_t stride = 0u;
    std::vector<VertexAttribute> attributes;

    [[nodiscard]] friend bool operator<(ReaderKey const& p_left, ReaderKey const& p_right)
    {
        if (p_left.stride != p_right.stride)
        {
            return p_left.stride < p_right.stride;
        }
        return std::lexicographical_compare(
            p_left.attributes.begin(),
            p_left.attributes.end(),
            p_right.attributes.begin(),
            p_right.attributes.end(),
            [](VertexAttribute const& p_a, VertexAttribute const& p_b) { return tied(p_a) < tied(p_b); });
    }
};

struct Reader
{
    GLuint vao = 0u;
    std::size_t users = 0u;
    //! \brief The buffer the fields point at, 0 for none yet.
    GLuint vertices = 0u;
    //! \brief The index buffer bound in the vertex array, 0 for none.
    GLuint indices = 0u;
};

std::map<ReaderKey, Reader>& readers()
{
    static std::map<ReaderKey, Reader> instance;
    return instance;
}

//! \brief The reader of a vertex array, nullptr if it is not one of ours.
std::pair<ReaderKey const*, Reader*> readerOf(GLuint p_vao)
{
    for (auto& [key, reader] : readers())
    {
        if (reader.vao == p_vao)
        {
            return { &key, &reader };
        }
    }
    return { nullptr, nullptr };
}

GLuint g_bound_vao = 0u;
GLuint g_used_program = 0u;
RenderState g_applied;
bool g_state_known = false;
bool g_wireframe = false;
bool g_reversed_depth = false;

CompareFunc mirrored(CompareFunc p_func)
{
    switch (p_func)
    {
        case CompareFunc::Less:
            return CompareFunc::Greater;
        case CompareFunc::LessEqual:
            return CompareFunc::GreaterEqual;
        case CompareFunc::Greater:
            return CompareFunc::Less;
        case CompareFunc::GreaterEqual:
            return CompareFunc::LessEqual;
        case CompareFunc::Never:
        case CompareFunc::Equal:
        case CompareFunc::NotEqual:
        case CompareFunc::Always:
            return p_func;
    }
    return p_func;
}

GLenum toGL(ScalarType p_type)
{
    switch (p_type)
    {
        case ScalarType::Float:
            return GL_FLOAT;
        case ScalarType::Half:
            return GL_HALF_FLOAT;
        case ScalarType::Double:
#if defined(COMPAGES_GLES3)
            return GL_FLOAT;
#else
            return GL_DOUBLE;
#endif
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

//! \brief Point every field of a vertex array at the buffer bound to
//! GL_ARRAY_BUFFER. The vertex array must be bound.
void pointFields(ReaderKey const& p_key)
{
    for (VertexAttribute const& attribute : p_key.attributes)
    {
        const GLenum type = toGL(attribute.format.scalar);
        const auto components = static_cast<GLint>(attribute.format.components);
        const auto stride = static_cast<GLsizei>(p_key.stride);
        // A matrix is read one column per slot.
        const std::uint32_t column_bytes =
            static_cast<std::uint32_t>(sizeOf(attribute.format.scalar)) * attribute.format.components;

        for (std::uint8_t slot = 0u; slot < attribute.format.slots; ++slot)
        {
            const auto where = static_cast<GLuint>(attribute.location) + slot;
            const auto* offset = reinterpret_cast<const void*>(
                static_cast<std::uintptr_t>(attribute.offset + (slot * column_bytes)));
            if (attribute.format.as_integer)
            {
                glVertexAttribIPointer(where, components, type, stride, offset);
            }
#if !defined(COMPAGES_GLES3)
            else if (attribute.format.scalar == ScalarType::Double)
            {
                glVertexAttribLPointer(where, components, type, stride, offset);
            }
#endif
            else
            {
                glVertexAttribPointer(
                    where, components, type, attribute.format.normalized ? GL_TRUE : GL_FALSE, stride, offset);
            }
        }
    }
}

#if !defined(COMPAGES_GLES3)
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

//! \brief A forward compatible context (always so on macOS) refuses any
//! line width but one.
bool wideLinesAllowed()
{
    static const bool allowed = []
    {
        GLint flags = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        return (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) == 0;
    }();
    return allowed;
}
#endif

} // namespace

namespace detail
{

void useProgram(GLuint p_program)
{
    if (p_program != g_used_program)
    {
        glUseProgram(p_program);
        g_used_program = p_program;
    }
}

GLuint usedProgram()
{
    return g_used_program;
}

void bindVertexArray(GLuint p_vao)
{
    if (p_vao != g_bound_vao)
    {
        glBindVertexArray(p_vao);
        g_bound_vao = p_vao;
    }
}

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

void forgetBuffer(GLuint p_buffer)
{
    // A name given back may come again for another buffer: no vertex array
    // may believe it still points at this one.
    for (auto& entry : readers())
    {
        if (entry.second.vertices == p_buffer)
        {
            entry.second.vertices = 0u;
        }
        if (entry.second.indices == p_buffer)
        {
            entry.second.indices = 0u;
        }
    }
}

} // namespace detail

Result<NativeId> acquireVertexReader(std::span<const VertexAttribute> p_attributes, std::uint32_t p_stride)
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
    glGenVertexArrays(1, &vao);
    if (vao == 0u)
    {
        return failure("the driver refused to create a vertex array object");
    }

    // What does not depend on the buffer is set once: which slots are read,
    // and per vertex or per instance (all fields share one buffer, so one per
    // instance field makes them all so, as in the GL45 backend).
    bool per_instance = false;
    for (VertexAttribute const& attribute : p_attributes)
    {
        per_instance = per_instance || attribute.per_instance;
    }
    detail::bindVertexArray(vao);
    for (VertexAttribute const& attribute : p_attributes)
    {
        for (std::uint8_t slot = 0u; slot < attribute.format.slots; ++slot)
        {
            const auto where = static_cast<GLuint>(attribute.location) + slot;
            glEnableVertexAttribArray(where);
            glVertexAttribDivisor(where, per_instance ? 1u : 0u);
        }
    }

    readers().emplace(std::move(key), Reader{ vao, 1u, 0u, 0u });
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

void bindVertexBuffer(NativeId p_reader, NativeId p_buffer, std::uint32_t /* stride: in the key */)
{
    const auto vao = static_cast<GLuint>(p_reader);
    const auto buffer = static_cast<GLuint>(p_buffer);
    detail::bindVertexArray(vao);
    auto [key, reader] = readerOf(vao);
    if ((reader == nullptr) || (reader->vertices == buffer))
    {
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    pointFields(*key);
    glBindBuffer(GL_ARRAY_BUFFER, 0u);
    reader->vertices = buffer;
}

void bindIndexBuffer(NativeId p_reader, NativeId p_buffer)
{
    const auto vao = static_cast<GLuint>(p_reader);
    const auto buffer = static_cast<GLuint>(p_buffer);
    detail::bindVertexArray(vao);
    auto [key, reader] = readerOf(vao);
    if ((reader != nullptr) && (reader->indices == buffer))
    {
        return;
    }
    // Part of the state of the bound vertex array.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    if (reader != nullptr)
    {
        reader->indices = buffer;
    }
}

void useReversedDepth(bool p_reversed)
{
    // No glClipControl (4.5): the clip range stays [-1, 1]. A reversed
    // projection (near at 1, far at 0) then lands in [0.5, 1] of the depth
    // buffer. The order is right, so the mirrored comparisons below draw the
    // right image, with the precision of an ordinary depth buffer.
    if (p_reversed != g_reversed_depth)
    {
        g_reversed_depth = p_reversed;
        g_state_known = false;
    }
}

void forgetRenderState()
{
    g_state_known = false;
    g_used_program = 0u;
    g_bound_vao = 0u;
    // A foreign vertex array may have been bound and changed in between.
    for (auto& entry : readers())
    {
        entry.second.vertices = 0u;
        entry.second.indices = 0u;
    }
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

void bindPipeline(NativeId p_program, NativeId p_reader, RenderState const& p_state)
{
    RenderState state = p_state;
    if (g_wireframe && (state.polygon == PolygonMode::Fill))
    {
        state.polygon = PolygonMode::Line;
    }

    detail::useProgram(static_cast<GLuint>(p_program));
    detail::bindVertexArray(static_cast<GLuint>(p_reader));
    // The storage blocks the shaders read, from their copies.
    detail::prepareStorageReads(static_cast<GLuint>(p_program));

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
        glDepthFunc(toGL(g_reversed_depth ? mirrored(state.depth_func) : state.depth_func));
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
            glBlendEquationSeparate(toGL(state.blend.color_equation), toGL(state.blend.alpha_equation));
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
        glFrontFace((state.front_face == FrontFace::CounterClockwise) ? GL_CCW : GL_CW);
    }

    if (everything || (state.polygon != g_applied.polygon))
    {
#if defined(COMPAGES_GLES3)
        if (state.polygon != PolygonMode::Fill)
        {
            detail::warnOnce("draws filled triangles only: OpenGL ES has no glPolygonMode (wireframe)");
        }
#else
        glPolygonMode(GL_FRONT_AND_BACK, toGL(state.polygon));
#endif
    }

    if (everything || !(state.color_mask == g_applied.color_mask))
    {
        glColorMask(state.color_mask.red ? GL_TRUE : GL_FALSE,
                    state.color_mask.green ? GL_TRUE : GL_FALSE,
                    state.color_mask.blue ? GL_TRUE : GL_FALSE,
                    state.color_mask.alpha ? GL_TRUE : GL_FALSE);
    }

    if (everything ||
        (std::bit_cast<std::uint32_t>(state.line_width) != std::bit_cast<std::uint32_t>(g_applied.line_width)))
    {
#if defined(COMPAGES_GLES3)
        glLineWidth(state.line_width);
#else
        glLineWidth(wideLinesAllowed() ? state.line_width : 1.0f);
#endif
    }

#if !defined(COMPAGES_GLES3)
    // OpenGL ES always takes the size of a point from gl_PointSize.
    if (everything || (state.primitive != g_applied.primitive))
    {
        if (state.primitive == Primitive::Points)
        {
            glEnable(GL_PROGRAM_POINT_SIZE);
        }
        else
        {
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }
#endif

    g_applied = state;
    g_state_known = true;
}

void useProgram(NativeId p_program)
{
    detail::useProgram(static_cast<GLuint>(p_program));
}

} // namespace compages::gpu::backend
