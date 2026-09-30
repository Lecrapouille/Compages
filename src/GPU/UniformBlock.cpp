// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/UniformBlock.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Backends/Backend.hpp"
#include "GPU/Internal/Pools.hpp"

#include <cstring>

// ****************************************************************************
//! \file
//! \brief Filling a uniform block, at the offsets the driver gives.
//!
//! \note On matrices, and on why they are copied one column at a time. The
//! std140 rules treat a matrix as an array of its columns, and every element of
//! an array is padded up to 16 bytes. A mat4 is therefore 4 columns of 16
//! bytes, which is what four vectors of four floats already are, so it can be
//! copied whole. A mat3 is 3 columns of 16 bytes of which only 12 are used,
//! that is 48 bytes where C++ gives the same matrix 36. Copying those 36 bytes
//! puts the second column where the shader expects the padding of the first:
//! the shader then reads a matrix that is not far off, which is worse than one
//! that is obviously wrong. The stride is never assumed here, it is the one the
//! driver reported, so a driver packing differently is followed rather than
//! fought.
//!
//! \note On the order of the numbers within a matrix: std140 stores a \c mat4
//! column by column. C++ holds row-major Scilab layout, so \c set() transposes
//! before staging (see \c matrixForStd140).
// ****************************************************************************

namespace compages::gpu
{

namespace
{

//! \brief Are these two types the same thing, as far as writing goes?
//!
//! Not simply equality, because a bool in a block occupies four bytes and is
//! written as four bytes, so a shader declaring `bool enabled` may be given a
//! bool, an int or an unsigned int without any of them being a mistake worth
//! stopping for.
template <typename T, std::size_t Rows, std::size_t Cols>
compages::core::Matrix<T, Rows, Cols> matrixForStd140(
    compages::core::Matrix<T, Rows, Cols> const& p_matrix)
{
    return compages::core::transpose(p_matrix);
}

bool interchangeable(DataType p_declared, DataType p_offered)
{
    if (p_declared == p_offered)
    {
        return true;
    }

    const bool declared_is_bool =
        (p_declared == DataType::Bool) || (p_declared == DataType::BVec2) ||
        (p_declared == DataType::BVec3) || (p_declared == DataType::BVec4);
    if (!declared_is_bool)
    {
        return false;
    }

    // Same number of components, whatever family they are written as.
    return (rowsOf(p_declared) == rowsOf(p_offered)) &&
           (columnsOf(p_offered) == 1u) &&
           ((scalarOf(p_offered) == ScalarType::Int32) ||
            (scalarOf(p_offered) == ScalarType::UInt32));
}

//! \brief How many bytes C++ hands over for a value of this type.
//!
//! What the caller's pointer points at, which is not what the block will hold:
//! the padding of a matrix and of a vector of three is added afterwards, here
//! we are only measuring what may be read from the argument.
std::size_t suppliedBytes(DataType p_type)
{
    return sizeOf(scalarOf(p_type)) * rowsOf(p_type) * columnsOf(p_type);
}

} // namespace

UniformBlock::UniformBlock(Buffer<std::byte>&& p_buffer,
                           BlockInfo p_info,
                           int p_binding)
    : m_buffer(std::move(p_buffer)),
      m_info(std::move(p_info)),
      m_binding(p_binding)
{
    m_bytes.assign(m_buffer.count(), std::byte{ 0 });

    // The whole block is marked from the start, so that a block bound after
    // only some of its members were written sends zeroes for the others rather
    // than whatever the device had in that memory. Uninitialised uniform data
    // is the kind of bug that behaves differently on two machines.
    m_dirty.addAll(m_bytes.size());
}

Result<UniformBlock> UniformBlock::create(Program& p_program,
                                          std::string const& p_name,
                                          int p_binding)
{
    if (!p_program.valid())
    {
        return failure(
            "cannot make a uniform block from a program that did not "
            "link");
    }

    BlockInfo const* found = p_program.reflection().uniformBlock(p_name);
    if (found == nullptr)
    {
        return failure("this program declares no uniform block called '" +
                       p_name + "'. What it does declare:\n" +
                       p_program.reflection().toString());
    }

    if (found->bytes == 0u)
    {
        return failure("the driver says the uniform block '" + p_name +
                       "' occupies no bytes, so there is nothing to fill. A "
                       "block none of whose members are read is removed by the "
                       "linker, which is usually what happened");
    }

    // Copied before anything else may change the reflection, since telling the
    // program which point to read from does exactly that.
    BlockInfo info = *found;

    int binding = info.binding;
    if (p_binding >= 0)
    {
        COMPAGES_TRY(p_program.bindUniformBlock(p_name, p_binding));
        binding = p_binding;
    }

    // Dynamic rather than Immutable: a block whose contents never change would
    // be better made immutable, but it is not what a block is for, and the cost
    // of being wrong the other way round is a buffer that cannot be written at
    // all.
    auto buffer_result = Buffer<std::byte>::create(
        info.bytes, BufferKind::Uniform, BufferUsage::Dynamic);
    if (!buffer_result)
    {
        return failure(buffer_result.error());
    }
    auto buffer = buffer_result.take();

    return UniformBlock(std::move(buffer), std::move(info), binding);
}

void UniformBlock::stage(std::size_t p_offset,
                         const void* p_data,
                         std::size_t p_bytes)
{
    std::memcpy(m_bytes.data() + p_offset, p_data, p_bytes);
    m_dirty.add(p_offset, p_bytes);
}

Status UniformBlock::writeMember(std::string_view p_name,
                                 std::size_t p_index,
                                 DataType p_wanted,
                                 const void* p_data)
{
    if (!valid())
    {
        return failure("there is no uniform block here to write to");
    }

    BlockMember const* member = m_info.find(p_name);
    if (member == nullptr)
    {
        return failure("the uniform block '" + m_info.name +
                       "' has no member called '" + std::string(p_name) +
                       "'. What it holds:\n" + describe());
    }

    if (!interchangeable(member->type, p_wanted))
    {
        return failure("the shader declares '" + std::string(p_name) +
                       "' in the block '" + m_info.name + "' as " +
                       toString(member->type) + ", but it is being set as " +
                       toString(p_wanted));
    }

    const std::size_t elements =
        static_cast<std::size_t>((member->elements > 0) ? member->elements : 1);
    if (p_index >= elements)
    {
        return failure("'" + std::string(p_name) + "' in the block '" +
                       m_info.name + "' holds " + std::to_string(elements) +
                       " element" + ((elements == 1u) ? "" : "s") +
                       ", so there is no element number " +
                       std::to_string(p_index));
    }

    // Every element of an array sits at the stride the driver reported, which
    // under std140 is at least 16 bytes however small the element is. Taking it
    // from the driver rather than computing it is what makes an array of floats
    // land where the shader reads it.
    const std::size_t stride =
        (member->array_stride != 0u)
            ? member->array_stride
            : ((elements > 1u) ? suppliedBytes(member->type) : 0u);
    const std::size_t start = member->offset + (p_index * stride);

    const auto* source = static_cast<const std::byte*>(p_data);
    const std::size_t rows = rowsOf(member->type);
    const std::size_t columns = columnsOf(member->type);
    const std::size_t row_bytes = sizeOf(scalarOf(member->type)) * rows;

    // A matrix whose columns the driver spaced further apart than their
    // contents, which is a mat3 in nearly every case, has to be copied a column
    // at a time.
    const bool padded_columns = isMatrix(member->type) &&
                                (member->matrix_stride != 0u) &&
                                (member->matrix_stride != row_bytes);

    const std::size_t needed =
        padded_columns ? (member->matrix_stride * (columns - 1u)) + row_bytes
                       : suppliedBytes(member->type);
    if ((start + needed) > m_bytes.size())
    {
        return failure(
            "writing '" + std::string(p_name) +
            "' would go past the "
            "end of the block '" +
            m_info.name +
            "', which the driver "
            "says is " +
            std::to_string(m_bytes.size()) +
            " bytes. This is a bug in the library rather than in the "
            "caller: what it was told about the block does not fit "
            "the block");
    }

    if (padded_columns)
    {
        for (std::size_t column = 0u; column < columns; ++column)
        {
            stage(start + (column * member->matrix_stride),
                  source + (column * row_bytes),
                  row_bytes);
        }
        return success();
    }

    stage(start, source, needed);
    return success();
}

Status UniformBlock::setRaw(std::size_t p_offset,
                            std::span<const std::byte> p_bytes)
{
    if (!valid())
    {
        return failure("there is no uniform block here to write to");
    }
    if ((p_offset + p_bytes.size()) > m_bytes.size())
    {
        return failure("writing " + std::to_string(p_bytes.size()) +
                       " bytes at " + std::to_string(p_offset) +
                       " would go past the end of the block '" + m_info.name +
                       "', which the driver says is " +
                       std::to_string(m_bytes.size()) + " bytes");
    }
    if (!p_bytes.empty())
    {
        stage(p_offset, p_bytes.data(), p_bytes.size());
    }
    return success();
}

Status UniformBlock::set(std::string_view p_name, float p_value)
{
    return writeMember(p_name, 0u, DataType::Float, &p_value);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Vector2f const& p_value)
{
    return writeMember(p_name, 0u, DataType::Vec2, &p_value);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Vector3f const& p_value)
{
    return writeMember(p_name, 0u, DataType::Vec3, &p_value);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Vector4f const& p_value)
{
    return writeMember(p_name, 0u, DataType::Vec4, &p_value);
}

Status UniformBlock::set(std::string_view p_name, std::int32_t p_value)
{
    return writeMember(p_name, 0u, DataType::Int, &p_value);
}

Status UniformBlock::set(std::string_view p_name, std::uint32_t p_value)
{
    return writeMember(p_name, 0u, DataType::UInt, &p_value);
}

Status UniformBlock::set(std::string_view p_name, bool p_value)
{
    // Widened here rather than in the backend: a bool in a block is four bytes,
    // and the four bytes have to exist somewhere before they can be copied.
    const std::uint32_t widened = p_value ? 1u : 0u;
    return writeMember(p_name, 0u, DataType::Bool, &widened);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Vector2i const& p_value)
{
    return writeMember(p_name, 0u, DataType::IVec2, &p_value);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Vector3i const& p_value)
{
    return writeMember(p_name, 0u, DataType::IVec3, &p_value);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Vector4i const& p_value)
{
    return writeMember(p_name, 0u, DataType::IVec4, &p_value);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Matrix22f const& p_value)
{
    const compages::core::Matrix22f staged = matrixForStd140(p_value);
    return writeMember(p_name, 0u, DataType::Mat2, &staged);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Matrix33f const& p_value)
{
    const compages::core::Matrix33f staged = matrixForStd140(p_value);
    return writeMember(p_name, 0u, DataType::Mat3, &staged);
}

Status UniformBlock::set(std::string_view p_name,
                         compages::core::Matrix44f const& p_value)
{
    const compages::core::Matrix44f staged = matrixForStd140(p_value);
    return writeMember(p_name, 0u, DataType::Mat4, &staged);
}

Status UniformBlock::update()
{
    if (!valid())
    {
        return failure("there is no uniform block here to send");
    }
    if (m_dirty.empty())
    {
        return success();
    }

    const std::size_t first = m_dirty.begin();
    const std::size_t count = m_dirty.end() - first;
    COMPAGES_TRY(detail::writeBuffer(
        m_buffer.handle(), first, count, m_bytes.data() + first));
    m_dirty.clear();
    return success();
}

Status UniformBlock::bind()
{
    COMPAGES_TRY(update());

    detail::BufferRecord const* record =
        detail::pools().buffers.get(m_buffer.handle());
    if (record == nullptr)
    {
        return failure("the memory of this uniform block no longer exists");
    }

    backend::bindBufferToPoint(
        BufferKind::Uniform, record->native, m_binding, 0u, 0u);
    return success();
}

std::string UniformBlock::describe() const
{
    std::string text = "uniform block '" + m_info.name + "' on binding point " +
                       std::to_string(m_binding) + ", " +
                       std::to_string(m_info.bytes) +
                       " bytes as the driver reports it:";

    if (m_info.members.empty())
    {
        text +=
            "\n  (the driver reports no members, which means the linker kept "
            "none of them)";
        return text;
    }

    for (BlockMember const& member : m_info.members)
    {
        text += "\n  " + std::string(toString(member.type)) + " " + member.name;
        if (member.elements > 1)
        {
            text += "[" + std::to_string(member.elements) + "]";
        }
        text += " at byte " + std::to_string(member.offset);
        if (member.elements > 1)
        {
            text += ", one element every " +
                    std::to_string(member.array_stride) + " bytes";
        }
        if (isMatrix(member.type) && (member.matrix_stride != 0u))
        {
            text += ", one column every " +
                    std::to_string(member.matrix_stride) + " bytes";
        }
    }

    return text;
}

namespace detail
{

// Only the members the driver reports are checked, and this is the whole
// subtlety of the function. A member of a block that no shader stage reads may
// be left out of what the driver reports, while the offsets of those that
// remain are unchanged because the layout of a block is fixed by its
// declaration. Requiring the two lists to match would therefore refuse a
// perfectly good struct as soon as a shader stopped using one of its members,
// which is the sort of check people learn to switch off.
//
// The other direction is a real error: a member the shader reads and the struct
// does not provide means the shader is reading whatever happens to be at that
// offset.
Status checkAgainstDriver(UniformBlock const& p_block,
                          std::size_t p_size,
                          std::span<const std140::Member> p_members)
{
    BlockInfo const& info = p_block.info();

    if (p_size < info.bytes)
    {
        return failure("the struct is " + std::to_string(p_size) +
                       " bytes and the driver wants the block '" + info.name +
                       "' to be " + std::to_string(info.bytes) +
                       " bytes, so the shader would read past the end of it. " +
                       p_block.describe());
    }

    for (BlockMember const& wanted : info.members)
    {
        const std140::Member* given = nullptr;
        for (std140::Member const& candidate : p_members)
        {
            if (wanted.name == candidate.name)
            {
                given = &candidate;
                break;
            }
        }

        if (given == nullptr)
        {
            return failure(
                "the shader reads '" + wanted.name + "' from the block '" +
                info.name +
                "' and the struct has no member of that name. The two are "
                "matched by name, because a name is the only thing the two "
                "sides share; rename the C++ member to '" +
                wanted.name +
                "', or fill the block member by member with "
                "compages::gpu::UniformBlock "
                "instead. " +
                p_block.describe());
        }

        if (given->offset != wanted.offset)
        {
            return failure(
                "the driver puts '" + wanted.name + "' at byte " +
                std::to_string(wanted.offset) + " of the block '" + info.name +
                "' and C++ puts it at byte " + std::to_string(given->offset) +
                ". The struct follows the std140 rules, since it would not "
                "have "
                "compiled otherwise, so this driver is laying the block out "
                "some "
                "other way: check the shader for a layout qualifier other than "
                "std140, and otherwise fill the block member by member with "
                "compages::gpu::UniformBlock. " +
                p_block.describe());
        }
    }

    return success();
}

} // namespace detail

} // namespace compages::gpu
