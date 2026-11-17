// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"
#include "GPU/Internal/ComputeEmulation.hpp"

#include <algorithm>
#include <map>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Storage blocks and compute shaders on OpenGL 4.1 and OpenGL ES 3.0,
//! which have neither.
//!
//! GPU/Internal/ComputeEmulation.cpp rewrote the shaders: a storage block is
//! read from a texture of 32 bit words holding a copy of its buffer, and a
//! compute shader became a vertex shader whose outputs are captured.
//!
//! - **The copies.** One texture per buffer, made again only when the buffer
//!   changed since: writeBuffer() and a dispatch that writes it give it a new
//!   version. The copy stays on the device (GL_PIXEL_UNPACK_BUFFER). A draw
//!   whose fragment shader reads the clustered lamps then costs a lookup, not
//!   a copy, as long as the lamps do not move.
//! - **A dispatch** brings the copies up to date, draws one point per
//!   invocation with the rasterizer off, and captures the outputs of each
//!   point into the written buffers (transform feedback).
//! - **A draw** (bindPipeline()) brings the copies its program reads up to
//!   date and binds them.
//!
//! The binding points of storage blocks do not exist either: this file keeps
//! a table of them, filled by bindBufferToPoint().
// ****************************************************************************

namespace compages::gpu::backend
{

namespace
{

//! \brief Texture units holding the copies: from 30 down, below the unit the
//! backend configures textures on (31), far above the ones a Drawable uses.
constexpr GLuint FIRST_DATA_UNIT = 30u;

//! \brief The empty fragment shader beside a compute shader (OpenGL ES links
//! no program without one).
struct CompiledCompute
{
    GLuint fragment = 0u;
    gpu::detail::EmulatedCompute meta;
};

//! \brief A storage block of a program, and where its uniforms are.
struct BlockState
{
    gpu::detail::EmulatedBlock block;
    GLint sampler = -1;
    GLint count = -1;
    GLint base = -1;
};

//! \brief A program that reads storage blocks: a compute one, or a drawing
//! one.
struct StorageRun
{
    bool compute = false;
    std::vector<BlockState> blocks;
    std::array<int, 3> local_size{ 1, 1, 1 };
    GLint groups = -1;
    GLint width = -1;
};

//! \brief A buffer bound to a storage binding point.
struct Point
{
    GLuint buffer = 0u;
    std::size_t offset = 0u;
    std::size_t bytes = 0u;
};

//! \brief The copy of a buffer, and the version of the buffer it holds.
struct Copy
{
    GLuint texture = 0u;
    GLsizei rows = 0;
    std::uint64_t version = 0u;
};

std::map<GLuint, CompiledCompute>& computeShaders()
{
    static std::map<GLuint, CompiledCompute> instance;
    return instance;
}

//! \brief The blocks a vertex or fragment shader reads.
std::map<GLuint, std::vector<gpu::detail::EmulatedBlock>>& readingShaders()
{
    static std::map<GLuint, std::vector<gpu::detail::EmulatedBlock>> instance;
    return instance;
}

std::map<GLuint, StorageRun>& storagePrograms()
{
    static std::map<GLuint, StorageRun> instance;
    return instance;
}

std::map<int, Point>& storagePoints()
{
    static std::map<int, Point> instance;
    return instance;
}

std::map<GLuint, std::uint64_t>& versions()
{
    static std::map<GLuint, std::uint64_t> instance;
    return instance;
}

std::map<GLuint, Copy>& copies()
{
    static std::map<GLuint, Copy> instance;
    return instance;
}

GLsizei g_width = 1024;
GLuint g_empty_vao = 0u;
GLuint g_feedback = 0u;

//! \brief The size of a buffer, asked to the driver.
std::size_t bufferBytes(GLuint p_buffer)
{
    GLint size = 0;
    glBindBuffer(GL_COPY_READ_BUFFER, p_buffer);
    glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);
    glBindBuffer(GL_COPY_READ_BUFFER, 0u);
    return static_cast<std::size_t>(size);
}

//! \brief The copy of a buffer, made again if the buffer changed, bound on
//! \c p_unit for the shader to read.
void bindCopy(GLuint p_buffer, GLuint p_unit)
{
    Copy& copy = copies()[p_buffer];
    const std::uint64_t version = versions()[p_buffer];
    glActiveTexture(GL_TEXTURE0 + p_unit);
    if ((copy.texture != 0u) && (copy.version == version))
    {
        glBindTexture(GL_TEXTURE_2D, copy.texture);
        return;
    }

    const std::size_t bytes = bufferBytes(p_buffer);
    const auto words = static_cast<GLsizei>(bytes / 4u);
    const GLsizei rows = std::max<GLsizei>(1, (words + g_width - 1) / g_width);
    if ((copy.texture == 0u) || (copy.rows < rows))
    {
        if (copy.texture == 0u)
        {
            glGenTextures(1, &copy.texture);
        }
        glBindTexture(GL_TEXTURE_2D, copy.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, g_width, rows, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        // An integer texture is complete only with NEAREST and one level.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        copy.rows = rows;
    }
    glBindTexture(GL_TEXTURE_2D, copy.texture);

    // From the buffer to the texture without the CPU: the "pixels" are read
    // from the buffer bound to GL_PIXEL_UNPACK_BUFFER.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, p_buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    const GLsizei full = words / g_width;
    const GLsizei rest = words % g_width;
    if (full > 0)
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_width, full, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    }
    if (rest > 0)
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, full, rest, 1, GL_RED_INTEGER, GL_UNSIGNED_INT,
                        reinterpret_cast<const void*>(std::size_t(full) * std::size_t(g_width) * 4u));
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0u);
    copy.version = version;
}

//! \brief Bind what the blocks of a program read: the copies, how many
//! elements, where the bound range starts. A block with no buffer bound reads
//! zeros (no texture), so that a shader that does not reach the read is fine.
//! \return the number of elements of each block, in order.
std::vector<std::size_t> bindBlocks(StorageRun& p_run)
{
    std::vector<std::size_t> counts;
    counts.reserve(p_run.blocks.size());
    for (std::size_t k = 0u; k < p_run.blocks.size(); ++k)
    {
        BlockState& state = p_run.blocks[k];
        const GLuint unit = FIRST_DATA_UNIT - static_cast<GLuint>(k);
        auto point = storagePoints().find(state.block.binding);
        if ((point == storagePoints().end()) || (point->second.buffer == 0u))
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, 0u);
            glUniform1i(state.count, 0);
            counts.push_back(0u);
            continue;
        }
        Point const& at = point->second;
        const std::size_t bytes = (at.bytes != 0u) ? at.bytes : bufferBytes(at.buffer) - at.offset;
        const std::size_t elements = bytes / state.block.stride;
        bindCopy(at.buffer, unit);
        glUniform1i(state.count, static_cast<GLint>(elements));
        glUniform1i(state.base, static_cast<GLint>(at.offset / 4u));
        counts.push_back(elements);
    }
    glUniform1i(p_run.width, g_width);
    return counts;
}

//! \brief Keep what a program needs to read its blocks, and give each
//! sampler its unit, once.
void recordRun(GLuint p_program, StorageRun p_run, std::vector<gpu::detail::EmulatedBlock> const& p_blocks)
{
    p_run.width = glGetUniformLocation(p_program, "compages_width");
    detail::useProgram(p_program);
    for (gpu::detail::EmulatedBlock const& block : p_blocks)
    {
        BlockState state;
        state.block = block;
        state.sampler = glGetUniformLocation(p_program, ("compages_data_" + block.array).c_str());
        state.count = glGetUniformLocation(p_program, ("compages_count_" + block.array).c_str());
        state.base = glGetUniformLocation(p_program, ("compages_base_" + block.array).c_str());
        const GLuint unit = FIRST_DATA_UNIT - static_cast<GLuint>(p_run.blocks.size());
        glUniform1i(state.sampler, static_cast<GLint>(unit));
        p_run.blocks.push_back(std::move(state));
    }
    storagePrograms()[p_program] = std::move(p_run);
}

} // namespace

namespace detail
{

void setDataTextureWidth(GLsizei p_width)
{
    g_width = p_width;
}

Result<NativeId> compileCompute(std::string_view p_source)
{
    auto emulated = gpu::detail::emulateCompute(p_source);
    if (!emulated)
    {
        return failure(emulated.error());
    }

    std::string log;
    const GLuint vertex = compileGlsl(GL_VERTEX_SHADER, emulated.value().text, log);
    if (vertex == 0u)
    {
        return failure(log + "\n(" + name() +
                       " has no compute shaders: this one was rewritten as a vertex shader for transform "
                       "feedback, and that did not compile. The line numbers are those of your shader)");
    }
    const GLuint fragment = compileGlsl(GL_FRAGMENT_SHADER, "#version 450 core\nvoid main() {}\n", log);
    if (fragment == 0u)
    {
        glDeleteShader(vertex);
        return failure("the empty fragment shader of an emulated compute shader did not compile: " + log);
    }
    computeShaders()[vertex] = CompiledCompute{ fragment, emulated.take() };
    return static_cast<NativeId>(vertex);
}

void rememberStorageReads(GLuint p_shader, std::vector<gpu::detail::EmulatedBlock> p_blocks)
{
    readingShaders()[p_shader] = std::move(p_blocks);
}

bool releaseCompute(GLuint p_shader)
{
    readingShaders().erase(p_shader);
    auto found = computeShaders().find(p_shader);
    if (found == computeShaders().end())
    {
        return false;
    }
    glDeleteShader(found->second.fragment);
    glDeleteShader(p_shader);
    computeShaders().erase(found);
    return true;
}

bool prepareComputeLink(GLuint p_program, std::span<const NativeId> p_shaders)
{
    for (NativeId shader : p_shaders)
    {
        auto found = computeShaders().find(static_cast<GLuint>(shader));
        if (found == computeShaders().end())
        {
            continue;
        }
        gpu::detail::EmulatedCompute const& meta = found->second.meta;
        glAttachShader(p_program, found->second.fragment);

        // The outputs captured, in the order of the memory: one record of the
        // only written block, or one output per written block.
        std::vector<char const*> outputs;
        for (gpu::detail::EmulatedBlock const& block : meta.blocks)
        {
            for (std::string const& output : block.outputs)
            {
                outputs.push_back(output.c_str());
            }
        }
        glTransformFeedbackVaryings(p_program,
                                    static_cast<GLsizei>(outputs.size()),
                                    outputs.data(),
                                    meta.separate ? GL_SEPARATE_ATTRIBS : GL_INTERLEAVED_ATTRIBS);
        return true;
    }
    return false;
}

void finishStorageLink(GLuint p_program, std::span<const NativeId> p_shaders, bool p_linked)
{
    // A compute program.
    for (NativeId shader : p_shaders)
    {
        auto found = computeShaders().find(static_cast<GLuint>(shader));
        if (found == computeShaders().end())
        {
            continue;
        }
        glDetachShader(p_program, found->second.fragment);
        if (!p_linked)
        {
            return;
        }
        StorageRun run;
        run.compute = true;
        run.local_size = found->second.meta.local_size;
        run.groups = glGetUniformLocation(p_program, "compages_groups");
        recordRun(p_program, std::move(run), found->second.meta.blocks);
        return;
    }

    // A drawing program: the blocks its stages read, each once (a vertex and
    // a fragment shader may read the same one).
    if (!p_linked)
    {
        return;
    }
    std::vector<gpu::detail::EmulatedBlock> blocks;
    for (NativeId shader : p_shaders)
    {
        auto found = readingShaders().find(static_cast<GLuint>(shader));
        if (found == readingShaders().end())
        {
            continue;
        }
        for (gpu::detail::EmulatedBlock const& block : found->second)
        {
            const bool known = std::any_of(blocks.begin(), blocks.end(),
                                           [&](gpu::detail::EmulatedBlock const& p_other)
                                           { return p_other.block == block.block; });
            if (!known)
            {
                blocks.push_back(block);
            }
        }
    }
    if (!blocks.empty())
    {
        recordRun(p_program, StorageRun{}, blocks);
    }
}

void forgetStorageProgram(GLuint p_program)
{
    if (storagePrograms().erase(p_program) == 0u)
    {
        return;
    }
    const bool compute = std::any_of(storagePrograms().begin(), storagePrograms().end(),
                                     [](auto const& p_entry) { return p_entry.second.compute; });
    if (!compute)
    {
        // The last compute program: what dispatches share goes too, so that a
        // check at the end finds the device as it was. Unbound through the
        // cache first: a new vertex array may get the same name.
        if (g_empty_vao != 0u)
        {
            detail::bindVertexArray(0u);
            glDeleteVertexArrays(1, &g_empty_vao);
            g_empty_vao = 0u;
        }
        if (g_feedback != 0u)
        {
            glDeleteTransformFeedbacks(1, &g_feedback);
            g_feedback = 0u;
        }
    }
}

void reflectStorage(GLuint p_program, ProgramReflection& p_reflection)
{
    // What the emulation added is not the user's business.
    std::erase_if(p_reflection.uniforms,
                  [](UniformInfo const& p_uniform) { return p_uniform.name.starts_with("compages_"); });

    auto found = storagePrograms().find(p_program);
    if (found == storagePrograms().end())
    {
        return;
    }
    StorageRun const& run = found->second;
    if (run.compute)
    {
        p_reflection.work_group_size = run.local_size;
    }
    // As a device with storage blocks reports them: one member per field of
    // an element, an array of unknown size with the stride of an element.
    static const std::map<std::string_view, DataType> types = {
        { "float", DataType::Float }, { "vec2", DataType::Vec2 },   { "vec3", DataType::Vec3 },
        { "vec4", DataType::Vec4 },   { "int", DataType::Int },     { "ivec2", DataType::IVec2 },
        { "ivec3", DataType::IVec3 }, { "ivec4", DataType::IVec4 }, { "uint", DataType::UInt },
        { "uvec2", DataType::UVec2 }, { "uvec3", DataType::UVec3 }, { "uvec4", DataType::UVec4 },
    };
    for (BlockState const& state : run.blocks)
    {
        BlockInfo block;
        block.name = state.block.block;
        block.binding = state.block.binding;
        for (gpu::detail::EmulatedBlock::Field const& field : state.block.fields)
        {
            BlockMember member;
            member.name = field.name;
            auto type = types.find(field.type);
            member.type = (type == types.end()) ? DataType::Unknown : type->second;
            member.elements = 0;
            member.offset = field.offset;
            member.array_stride = state.block.stride;
            block.members.push_back(std::move(member));
        }
        p_reflection.storage_blocks.push_back(std::move(block));
    }
}

void bindStoragePoint(GLuint p_buffer, int p_binding, std::size_t p_offset, std::size_t p_bytes)
{
    storagePoints()[p_binding] = Point{ p_buffer, p_offset, p_bytes };
}

void touchBuffer(GLuint p_buffer)
{
    ++versions()[p_buffer];
}

void dropStorageCopy(GLuint p_buffer)
{
    versions().erase(p_buffer);
    auto copy = copies().find(p_buffer);
    if (copy != copies().end())
    {
        glDeleteTextures(1, &copy->second.texture);
        copies().erase(copy);
    }
    for (auto& [binding, point] : storagePoints())
    {
        if (point.buffer == p_buffer)
        {
            point = Point{};
        }
    }
}

void prepareStorageReads(GLuint p_program)
{
    auto found = storagePrograms().find(p_program);
    if ((found != storagePrograms().end()) && !found->second.compute)
    {
        (void)bindBlocks(found->second);
    }
}

void releaseStorageCopies()
{
    for (auto& [buffer, copy] : copies())
    {
        glDeleteTextures(1, &copy.texture);
    }
    copies().clear();
    versions().clear();
    storagePoints().clear();
}

} // namespace detail

void bindStorageBlock(NativeId p_program, int p_block_index, int p_binding)
{
    auto found = storagePrograms().find(static_cast<GLuint>(p_program));
    if ((found == storagePrograms().end()) || (p_block_index < 0) ||
        (static_cast<std::size_t>(p_block_index) >= found->second.blocks.size()))
    {
        reportError(std::string(name()) + ": this program declares no storage block " + std::to_string(p_block_index));
        return;
    }
    found->second.blocks[static_cast<std::size_t>(p_block_index)].block.binding = p_binding;
}

void dispatch(std::uint32_t p_groups_x, std::uint32_t p_groups_y, std::uint32_t p_groups_z)
{
    const GLuint program = detail::usedProgram();
    auto found = storagePrograms().find(program);
    if ((found == storagePrograms().end()) || !found->second.compute)
    {
        reportError(std::string(name()) + ": the program dispatched is not a compute program");
        return;
    }
    StorageRun& run = found->second;

    // One point per invocation, but never more records than a written
    // buffer holds: OpenGL ES refuses a capture that does not fit.
    std::uint64_t records = std::uint64_t(p_groups_x) * p_groups_y * p_groups_z * std::uint64_t(run.local_size[0]) *
                            std::uint64_t(run.local_size[1]) * std::uint64_t(run.local_size[2]);

    const std::vector<std::size_t> counts = bindBlocks(run);
    std::vector<Point> written;
    for (std::size_t k = 0u; k < run.blocks.size(); ++k)
    {
        BlockState const& state = run.blocks[k];
        auto point = storagePoints().find(state.block.binding);
        if ((point == storagePoints().end()) || (point->second.buffer == 0u))
        {
            reportError("the storage block '" + state.block.block + "' has no buffer: bind() one before dispatch()");
            return;
        }
        if (state.block.written)
        {
            records = std::min<std::uint64_t>(records, counts[k]);
            Point capture = point->second;
            capture.bytes = counts[k] * state.block.stride;
            written.push_back(capture);
        }
    }
    glUniform3ui(run.groups, p_groups_x, p_groups_y, p_groups_z);
    if (records == 0u)
    {
        return;
    }

    if (g_empty_vao == 0u)
    {
        glGenVertexArrays(1, &g_empty_vao);
        glGenTransformFeedbacks(1, &g_feedback);
    }
    detail::bindVertexArray(g_empty_vao);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, g_feedback);
    for (std::size_t i = 0u; i < written.size(); ++i)
    {
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,
                          static_cast<GLuint>(i),
                          written[i].buffer,
                          static_cast<GLintptr>(written[i].offset),
                          static_cast<GLsizeiptr>(written[i].bytes));
    }
    glEnable(GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(records));
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);
    // WebGL 2 refuses a draw while a buffer is bound for capture and read
    // as vertices: leave none bound. The written buffers changed: their
    // copies are out of date.
    for (std::size_t i = 0u; i < written.size(); ++i)
    {
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, static_cast<GLuint>(i), 0u);
        detail::touchBuffer(written[i].buffer);
    }
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0u);
}

void memoryBarrier(Barrier /* what */)
{
    // The capture and the copies are ordinary commands: the driver orders
    // them with the draws that follow. Nothing to wait for.
}

} // namespace compages::gpu::backend
