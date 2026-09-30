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

#include "Compages/GPU/Compute.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Backends/Backend.hpp"
#include "GPU/Internal/Pools.hpp"
#include "GPU/Internal/Statistics.hpp"

#include <string>

namespace compages::gpu
{

namespace
{

Status ensureCompute(Program const& p_program)
{
    if (!p_program.valid())
    {
        return failure(
            "this compute program no longer exists. Either it was released "
            "while something still referred to it, or it was moved from");
    }
    if (p_program.reflection().work_group_size[0] == 0)
    {
        return failure(
            "this is a graphics program, which is drawn, not dispatched. A "
            "compute program is made with ComputeProgram::fromSource() or "
            "fromFile()");
    }
    return success();
}

} // namespace

//------------------------------------------------------------------------------
Result<ComputeProgram> ComputeProgram::fromSource(std::string_view p_source)
{
    auto program_result = Program::fromComputeSource(p_source);
    if (!program_result)
    {
        return failure(program_result.error());
    }
    auto program = program_result.take();
    if (program.reflection().work_group_size[0] == 0)
    {
        return failure(
            "the shader linked but declared no work group size. A compute "
            "shader needs layout(local_size_x = ...) so the dispatch knows how "
            "many invocations one group is");
    }
    return ComputeProgram(std::move(program));
}

//------------------------------------------------------------------------------
Result<ComputeProgram> ComputeProgram::fromFile(std::string const& p_path)
{
    auto program_result = Program::fromComputeFile(p_path);
    if (!program_result)
    {
        return failure(program_result.error());
    }
    auto program = program_result.take();
    if (program.reflection().work_group_size[0] == 0)
    {
        return failure(
            "the shader linked but declared no work group size. A compute "
            "shader needs layout(local_size_x = ...) so the dispatch knows how "
            "many invocations one group is");
    }
    return ComputeProgram(std::move(program));
}

//------------------------------------------------------------------------------
Status ComputeProgram::bind(std::string_view p_block, BufferHandle p_buffer)
{
    COMPAGES_TRY(ensureCompute(m_program));

    ProgramReflection const& what = m_program.reflection();
    BlockInfo const* block = what.storageBlock(p_block);
    if (block == nullptr)
    {
        return failure("there is no storage block named '" +
                       std::string(p_block) +
                       "'. The program declares: " + what.toString());
    }

    detail::BufferRecord const* record = detail::pools().buffers.get(p_buffer);
    if (record == nullptr)
    {
        return failure(
            "the buffer bound to '" + std::string(p_block) +
            "' has been released, so the compute pass has nothing to read");
    }
    if (record->kind != BufferKind::Storage)
    {
        return failure(
            "'" + std::string(p_block) +
            "' is a storage block, and the buffer was created to "
            "hold " +
            std::string(toString(record->kind)) +
            " data. Create it with BufferKind::Storage: that is also "
            "what lets a later draw read it as vertices");
    }

    backend::bindBufferToPoint(
        BufferKind::Storage, record->native, block->binding, 0u, 0u);
    return success();
}

//------------------------------------------------------------------------------
Status ComputeProgram::dispatch(std::uint32_t p_groups_x,
                                std::uint32_t p_groups_y,
                                std::uint32_t p_groups_z)
{
    if (!initialized())
    {
        return failure(
            "compages::gpu::init() has not been called, so there is no device "
            "to dispatch on");
    }
    COMPAGES_TRY(ensureCompute(m_program));

    if ((p_groups_x == 0u) || (p_groups_y == 0u) || (p_groups_z == 0u))
    {
        return failure(
            "a dispatch of " + std::to_string(p_groups_x) + " x " +
            std::to_string(p_groups_y) + " x " + std::to_string(p_groups_z) +
            " work groups runs nothing. A count of zero is almost always a "
            "size that was never computed");
    }

    DeviceInfo const& info = device();
    const std::uint32_t asked[3] = { p_groups_x, p_groups_y, p_groups_z };
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        if (asked[axis] >
            static_cast<std::uint32_t>(info.max_compute_work_group_count[axis]))
        {
            return failure(
                "a dispatch of " + std::to_string(asked[axis]) +
                " work groups on axis " + std::to_string(axis) +
                " was asked for, and this driver allows at most " +
                std::to_string(info.max_compute_work_group_count[axis]));
        }
    }

    detail::ProgramRecord const* record =
        detail::pools().programs.get(m_program.handle());
    backend::useProgram(record->native);
    backend::dispatch(p_groups_x, p_groups_y, p_groups_z);
    detail::countDispatch();
    return success();
}

//------------------------------------------------------------------------------
Status ComputeProgram::dispatchItems(std::uint32_t p_count)
{
    COMPAGES_TRY(ensureCompute(m_program));
    if (p_count == 0u)
    {
        return failure(
            "a dispatch over zero items runs nothing. Skip it instead");
    }

    const auto local =
        static_cast<std::uint32_t>(m_program.reflection().work_group_size[0]);
    const std::uint32_t groups = (p_count + local - 1u) / local;
    return dispatch(groups, 1u, 1u);
}

//------------------------------------------------------------------------------
void barrier(Barrier p_what)
{
    if (initialized())
    {
        backend::memoryBarrier(p_what);
    }
}

} // namespace compages::gpu
