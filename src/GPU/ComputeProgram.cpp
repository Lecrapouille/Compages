// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/ComputeProgram.hpp"
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
            "compute program is made with ComputeProgram::loadSource() or "
            "loadFile()");
    }
    return success();
}

} // namespace

//! \brief A compute program from a linked one, refused without a work
//! group size.
Result<ComputeProgram> ComputeProgram::fromLinked(Result<Program> p_program)
{
    if (!p_program)
    {
        return failure(p_program.error());
    }
    auto program = p_program.take();
    if (program.reflection().work_group_size[0] == 0)
    {
        return failure(
            "the shader linked but declared no work group size. A compute "
            "shader needs layout(local_size_x = ...) so the dispatch knows how "
            "many invocations one group is");
    }
    return ComputeProgram(std::move(program));
}

Result<ComputeProgram> ComputeProgram::fromSource(std::string_view p_source)
{
    return fromLinked(Program::fromComputeSource(p_source));
}

Result<ComputeProgram>
ComputeProgram::fromFile(std::filesystem::path const& p_path)
{
    return fromLinked(Program::fromComputeFile(p_path));
}

Result<ComputeProgram> ComputeProgram::fromSource(ShaderSource const& p_source)
{
    return fromLinked(Program::fromComputeSource(p_source));
}

Status ComputeProgram::bind(std::string_view p_block, BufferHandle p_buffer)
{
    COMPAGES_TRY(ensureCompute(m_program));
    return m_program.bind(p_block, p_buffer);
}

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

void barrier(Barrier p_what)
{
    if (initialized())
    {
        backend::memoryBarrier(p_what);
    }
}

} // namespace compages::gpu
