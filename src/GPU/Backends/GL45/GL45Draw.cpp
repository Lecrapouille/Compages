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

#include "GPU/Backends/GL45/GL45.hpp"

namespace compages::gpu::backend
{

namespace
{

//------------------------------------------------------------------------------
//! \brief What the vertices are taken to draw, as OpenGL names it.
//------------------------------------------------------------------------------
GLenum toGL(Primitive p_primitive)
{
    switch (p_primitive)
    {
        case Primitive::Points:
            return GL_POINTS;
        case Primitive::Lines:
            return GL_LINES;
        case Primitive::LineStrip:
            return GL_LINE_STRIP;
        case Primitive::LineLoop:
            return GL_LINE_LOOP;
        case Primitive::Triangles:
            return GL_TRIANGLES;
        case Primitive::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case Primitive::TriangleFan:
            return GL_TRIANGLE_FAN;
    }
    return GL_TRIANGLES;
}

//------------------------------------------------------------------------------
//! \brief The size of one index, as OpenGL names it.
//------------------------------------------------------------------------------
GLenum toGL(IndexType p_type)
{
    return (p_type == IndexType::UInt16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

} // namespace

//------------------------------------------------------------------------------
void bindVertexBuffer(NativeId p_reader,
                      NativeId p_buffer,
                      std::uint32_t p_stride)
{
    glVertexArrayVertexBuffer(static_cast<GLuint>(p_reader),
                              0u,
                              static_cast<GLuint>(p_buffer),
                              0,
                              static_cast<GLsizei>(p_stride));
}

//------------------------------------------------------------------------------
void bindIndexBuffer(NativeId p_reader, NativeId p_buffer)
{
    glVertexArrayElementBuffer(static_cast<GLuint>(p_reader),
                               static_cast<GLuint>(p_buffer));
}

//------------------------------------------------------------------------------
void beginPass(PassDesc const& p_desc, NativeId p_target)
{
    // Zero is what the driver calls the window. Anything else is a framebuffer
    // of ours, already checked complete when it was made.
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(p_target));

    glViewport(static_cast<GLint>(p_desc.x),
               static_cast<GLint>(p_desc.y),
               static_cast<GLsizei>(p_desc.width),
               static_cast<GLsizei>(p_desc.height));

    // The viewport says where the shape lands; it does not stop a clear, which
    // otherwise wipes the whole target however small the pass is. The scissor is
    // what makes a pass into a region mean what it says, for the clear and for
    // anything drawn afterwards, and it is turned off again when the pass ends so
    // that nothing else inherits it.
    glScissor(static_cast<GLint>(p_desc.x),
              static_cast<GLint>(p_desc.y),
              static_cast<GLsizei>(p_desc.width),
              static_cast<GLsizei>(p_desc.height));
    glEnable(GL_SCISSOR_TEST);

    // Clearing goes through the masks, so a pipeline that turned off depth writing
    // or colour writing would silently prevent the clear. Both are opened here and
    // the next bindPipeline puts back whatever it wants: the pass is where the
    // target is decided, so it is where the target may be written whole.
    GLboolean color_mask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    GLboolean depth_mask = GL_TRUE;
    if (p_desc.clear_color)
    {
        glGetBooleanv(GL_COLOR_WRITEMASK, color_mask);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (p_desc.clear_depth)
    {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask);
        glDepthMask(GL_TRUE);
    }

    GLbitfield what = 0u;
    if (p_desc.clear_color)
    {
        glClearColor(p_desc.color[0],
                     p_desc.color[1],
                     p_desc.color[2],
                     p_desc.color[3]);
        what |= GL_COLOR_BUFFER_BIT;
    }
    if (p_desc.clear_depth)
    {
        glClearDepth(static_cast<GLdouble>(p_desc.depth));
        what |= GL_DEPTH_BUFFER_BIT;
    }
    if (what != 0u)
    {
        glClear(what);
    }

    if (p_desc.clear_color)
    {
        glColorMask(
            color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
    }
    if (p_desc.clear_depth)
    {
        glDepthMask(depth_mask);
    }
}

//------------------------------------------------------------------------------
void endPass()
{
    // The viewport is left as it is: the next pass says where it draws, and a draw
    // outside a pass is refused before it reaches the backend. The scissor is not,
    // because it would go on cutting away whatever anybody else draws into this
    // context, a user interface toolkit included.
    glDisable(GL_SCISSOR_TEST);
}

//------------------------------------------------------------------------------
void draw(Primitive p_primitive, std::size_t p_first, std::size_t p_count)
{
    glDrawArrays(toGL(p_primitive),
                 static_cast<GLint>(p_first),
                 static_cast<GLsizei>(p_count));
}

//------------------------------------------------------------------------------
void drawInstanced(Primitive p_primitive,
                   std::size_t p_first,
                   std::size_t p_count,
                   std::size_t p_instances)
{
    glDrawArraysInstanced(toGL(p_primitive),
                          static_cast<GLint>(p_first),
                          static_cast<GLsizei>(p_count),
                          static_cast<GLsizei>(p_instances));
}

//------------------------------------------------------------------------------
void drawIndirect(Primitive p_primitive, NativeId p_commands)
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLuint>(p_commands));
    glDrawArraysIndirect(toGL(p_primitive), nullptr);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0u);
}

//------------------------------------------------------------------------------
void drawIndexed(Primitive p_primitive,
                 IndexType p_type,
                 std::size_t p_first,
                 std::size_t p_count)
{
    // The offset is in bytes, not in indices, which is the mistake this cast is
    // here to make visible rather than to hide.
    const std::size_t offset = p_first * sizeOf(p_type);

    glDrawElements(toGL(p_primitive),
                   static_cast<GLsizei>(p_count),
                   toGL(p_type),
                   reinterpret_cast<const void*>(offset));
}

//------------------------------------------------------------------------------
void waitForDevice()
{
    glFinish();
}

//------------------------------------------------------------------------------
void readTargetPixels(std::uint32_t p_x,
                      std::uint32_t p_y,
                      std::uint32_t p_width,
                      std::uint32_t p_height,
                      void* p_pixels)
{
    // Rows are handed back packed, whatever alignment was asked for elsewhere.
    // Leaving this at its default of four would pad every row of a width that is
    // not a multiple of four, which reads as an image sheared a little further
    // over on every line.
    GLint alignment = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(static_cast<GLint>(p_x),
                 static_cast<GLint>(p_y),
                 static_cast<GLsizei>(p_width),
                 static_cast<GLsizei>(p_height),
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 p_pixels);

    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
}

} // namespace compages::gpu::backend
