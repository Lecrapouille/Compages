// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Core/DirtyRange.hpp"
#include "Compages/GPU/Core/Std140.hpp"
#include "Compages/GPU/Shader.hpp"

#include <span>
#include <string>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Values that several programs read from the same memory.
//!
//! A uniform set with Program::set() belongs to one program: three programs
//! needing the same projection matrix means three calls, and three copies the
//! driver keeps. A uniform block is one buffer, written once, bound to a numbered
//! point, and read by every program declaring a block on that point. That is what
//! makes a frame of several passes cheap, and it is what 06_MultiPassMesh shows.
//!
//! There are two ways to fill one, and the difference between them is the whole
//! subject of this file.
//!
//! **Member by member, at the offsets the driver reports.** Always correct,
//! because the offsets are the driver's own answer rather than a computation, and
//! every write is checked against the type the shader declares:
//! \code
//! compages::gpu::UniformBlock block;
//! COMPAGES_TRY_ASSIGN(block, compages::gpu::UniformBlock::create(program, "Matrices"));
//! COMPAGES_TRY(block.set("projection", projection));
//! COMPAGES_TRY(block.set("view", view));
//! COMPAGES_TRY(block.bind());
//! \endcode
//!
//! **By copying a C++ struct whole.** One memcpy rather than a lookup per member,
//! which is what a block rewritten every frame wants, and which is only safe if
//! the struct is laid out the way the shading language reads it:
//! \code
//! struct Matrices
//! {
//!     Matrix44f projection;
//!     Matrix44f view;
//! };
//! GPU_STD140(Matrices, projection, view);
//!
//! compages::gpu::TypedUniformBlock<Matrices> block;
//! COMPAGES_TRY_ASSIGN(block,
//!                     compages::gpu::TypedUniformBlock<Matrices>::create(program, "Matrices"));
//! block.modify().projection = projection;
//! COMPAGES_TRY(block.bind());
//! \endcode
//!
//! The typed one is checked twice, and the two checks catch different mistakes.
//! GPU_STD140 refuses at compile time a struct whose members C++ and the std140
//! rules do not put in the same places, which is the mistake of writing the struct
//! carelessly. create() then compares the struct against the offsets this
//! particular driver reports, which is the mistake of assuming the rules were
//! followed. The first names a line of C++; the second names a member and two
//! numbers.
// ****************************************************************************

namespace compages::gpu
{

class UniformBlock;

namespace detail
{

// ----------------------------------------------------------------------------
//! \brief Compare a C++ struct against what the driver said about a block.
//!
//! Declared here rather than written inside the template so that the comparison
//! exists once in the library instead of once per struct.
//!
//! \param[in] p_block the block, already made.
//! \param[in] p_size what sizeof says about the struct.
//! \param[in] p_members what GPU_STD140 recorded about it.
//! \return why the struct cannot be copied into this block, naming the member.
// ----------------------------------------------------------------------------
[[nodiscard]] Status checkAgainstDriver(UniformBlock const& p_block,
                                        std::size_t p_size,
                                        std::span<const std140::Member> p_members);

} // namespace detail

// ****************************************************************************
//! \brief A block filled member by member, at the offsets the driver reports.
//!
//! The safe one, and the one to reach for first. Every set() asks the reflection
//! of the program where the member lives and what it is declared as, so writing a
//! vec3 into a mat4, or writing a member that does not exist, is a sentence rather
//! than a wrong image.
//!
//! Writing marks the bytes it touched and bind() sends those and no more, so a
//! block where one matrix out of eight changed sends 64 bytes.
// ****************************************************************************
class UniformBlock
{
public:

    // ------------------------------------------------------------------------
    //! \brief A block owning nothing.
    // ------------------------------------------------------------------------
    UniformBlock() = default;

    // ------------------------------------------------------------------------
    //! \brief Make a buffer of the size the driver wants for a block of a
    //! program.
    //!
    //! \param[in] p_program the program declaring the block. Not kept: a block
    //! whose program has gone still holds its own memory, and can still be bound,
    //! since a binding point belongs to the device rather than to a program.
    //! \param[in] p_name the name of the block as declared, which is the name in
    //! `uniform Matrices { ... }` and not the name of the instance.
    //! \param[in] p_binding which numbered point to read it from, or -1 to use
    //! the one the shader asked for. Given a number, the program is told to read
    //! the block from there, which is how programs that number their blocks
    //! differently are made to agree.
    //! \return the block, or why it could not be made: a program that never
    //! linked, or one declaring no block of that name, in which case the message
    //! lists what it does declare.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<UniformBlock> create(Program& p_program,
                                                     std::string const& p_name,
                                                     int p_binding = -1);

    UniformBlock(UniformBlock&&) noexcept = default;
    UniformBlock& operator=(UniformBlock&&) noexcept = default;
    UniformBlock(UniformBlock const&) = delete;
    UniformBlock& operator=(UniformBlock const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Write one member, checking that the shader declares it as this
    //! type.
    //!
    //! \return why nothing was written: no member of that name in this block, or
    //! one declared as something else, in which case the message says what the
    //! shader declares and what was offered.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status set(std::string_view p_name, float p_value);
    //! \brief Write a vec2 member.
    [[nodiscard]] Status set(std::string_view p_name, Vector2f const& p_value);
    //! \brief Write a vec3 member. The member after it in a block may well sit in
    //! the four bytes of padding that follow, which is why writing a block by hand
    //! goes wrong so often and why this function exists.
    [[nodiscard]] Status set(std::string_view p_name, Vector3f const& p_value);
    //! \brief Write a vec4 member.
    [[nodiscard]] Status set(std::string_view p_name, Vector4f const& p_value);
    //! \brief Write an int member.
    [[nodiscard]] Status set(std::string_view p_name, std::int32_t p_value);
    //! \brief Write a uint member.
    [[nodiscard]] Status set(std::string_view p_name, std::uint32_t p_value);
    //! \brief Write a bool member, which occupies four bytes in a block.
    [[nodiscard]] Status set(std::string_view p_name, bool p_value);
    //! \brief Write an ivec2 member.
    [[nodiscard]] Status set(std::string_view p_name, Vector2i const& p_value);
    //! \brief Write an ivec3 member.
    [[nodiscard]] Status set(std::string_view p_name, Vector3i const& p_value);
    //! \brief Write an ivec4 member.
    [[nodiscard]] Status set(std::string_view p_name, Vector4i const& p_value);
    //! \brief Write a mat2 member.
    [[nodiscard]] Status set(std::string_view p_name, Matrix22f const& p_value);
    //! \brief Write a mat3 member. Padded to 48 bytes in a block rather than the
    //! 36 a C++ matrix of three by three occupies, each column being rounded up to
    //! 16, so this one is copied a column at a time.
    [[nodiscard]] Status set(std::string_view p_name, Matrix33f const& p_value);
    //! \brief Write a mat4 member.
    [[nodiscard]] Status set(std::string_view p_name, Matrix44f const& p_value);

    // ------------------------------------------------------------------------
    //! \brief Write one element of an array member.
    //!
    //! \param[in] p_name the name of the array, without brackets.
    //! \param[in] p_index which element.
    //! \param[in] p_value what to write there.
    //!
    //! Each element sits at the stride the driver reported, which under std140 is
    //! at least 16 bytes whatever the element is. An array of floats copied from a
    //! packed C++ array is the other classic way of getting a block wrong; going
    //! through this is how not to.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] Status setElement(std::string_view p_name,
                                    std::size_t p_index,
                                    T const& p_value)
    {
        return writeMember(p_name, p_index, TypeOf<T>::value, &p_value);
    }

    // ------------------------------------------------------------------------
    //! \brief Overwrite bytes of the block without asking what they mean.
    //!
    //! What the typed block is built on, and the way out for a member this class
    //! has no set() for: a non square matrix, or a whole array in one go. Nothing
    //! is checked beyond the range fitting, so the offsets have to come from
    //! somewhere trustworthy, which means from the reflection of the program.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status setRaw(std::size_t p_offset,
                                std::span<const std::byte> p_bytes);

    // ------------------------------------------------------------------------
    //! \brief Send what was written since the last time, and no more.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status update();

    // ------------------------------------------------------------------------
    //! \brief Make the block readable by whichever programs read this binding
    //! point, sending first whatever is waiting.
    //!
    //! Both halves in one call on purpose. A block bound without being updated
    //! draws the frame with the values of the previous one, which looks like a lag
    //! of exactly one frame and is hunted for hours.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind();

    // ------------------------------------------------------------------------
    //! \brief Which numbered point this block is read from.
    // ------------------------------------------------------------------------
    [[nodiscard]] int binding() const
    {
        return m_binding;
    }

    // ------------------------------------------------------------------------
    //! \brief How many bytes the driver wants the block to be.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const
    {
        return m_bytes.size();
    }

    // ------------------------------------------------------------------------
    //! \brief The name of the block, as the shader declares it.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& name() const
    {
        return m_info.name;
    }

    // ------------------------------------------------------------------------
    //! \brief What the driver said about the block: its members, their offsets
    //! and their strides.
    // ------------------------------------------------------------------------
    [[nodiscard]] BlockInfo const& info() const
    {
        return m_info;
    }

    // ------------------------------------------------------------------------
    //! \brief Is there a block here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const
    {
        return m_buffer.valid();
    }

    // ------------------------------------------------------------------------
    //! \brief Where the driver put every member, and how wide it made it.
    //!
    //! The answer to "why is my shader reading rubbish out of this block".
    //! Printing this next to the C++ struct usually ends the investigation.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string describe() const;

    // ------------------------------------------------------------------------
    //! \brief The memory itself, for the parts of the library taking a buffer.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferHandle handle() const
    {
        return m_buffer.handle();
    }

private:

    UniformBlock(Buffer<std::byte>&& p_buffer, BlockInfo p_info, int p_binding);

    // ------------------------------------------------------------------------
    //! \brief Find the member, check its declared type, and stage the bytes.
    //!
    //! One function for every type rather than one per type: what differs between
    //! a vec3 and a mat3 is entirely described by the DataType, which the caller
    //! has from its argument, so the knowledge lives in one switch that the
    //! compiler checks is complete.
    //!
    //! \param[in] p_name the name of the member.
    //! \param[in] p_index which element, for an array member. Zero otherwise.
    //! \param[in] p_wanted what the caller is offering.
    //! \param[in] p_data the value, laid out as C++ lays it out.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status writeMember(std::string_view p_name,
                                     std::size_t p_index,
                                     DataType p_wanted,
                                     const void* p_data);

    // ------------------------------------------------------------------------
    //! \brief Copy into the staging memory and remember the bytes must travel.
    // ------------------------------------------------------------------------
    void stage(std::size_t p_offset, const void* p_data, std::size_t p_bytes);

    //! \brief What the buffer is to hold, kept on the CPU so that one member can
    //! be written without reading the device back.
    std::vector<std::byte> m_bytes;
    //! \brief What of it has been written since the last update.
    DirtyRange m_dirty;
    Buffer<std::byte> m_buffer;
    //! \brief What the driver said about this block. Copied rather than pointed
    //! at, because a program may be released while a block still holds its memory.
    BlockInfo m_info;
    int m_binding = 0;
};

// ****************************************************************************
//! \brief A block filled by copying a C++ struct whole.
//!
//! The fast one. Nothing is looked up per member and nothing is checked per
//! write, which is paid for by two checks made once. See the head of this file
//! for what each of them catches.
//!
//! \tparam T the struct, described with GPU_STD140.
// ****************************************************************************
template <typename T>
class TypedUniformBlock
{
public:

    static_assert(std::is_trivially_copyable_v<T>,
                  "a uniform block is filled by copying the bytes of T, so T "
                  "must be trivially copyable");

    static_assert(std140::described<T>(),
                  "this struct was never described with GPU_STD140, so nothing "
                  "has checked that the shader will read its members from where "
                  "C++ put them. Write GPU_STD140(YourStruct, first, second, "
                  "...) after the struct, or fill the block member by member "
                  "with compages::gpu::UniformBlock instead");

    // ------------------------------------------------------------------------
    //! \brief A block owning nothing.
    // ------------------------------------------------------------------------
    TypedUniformBlock() = default;

    // ------------------------------------------------------------------------
    //! \brief Make the buffer, then check the struct against what the driver did.
    //!
    //! \param[in] p_program the program declaring the block.
    //! \param[in] p_name the name of the block as declared.
    //! \param[in] p_binding which numbered point to read it from, or -1 for the
    //! one the shader asked for.
    //! \return the block, or why the struct cannot be copied into it, naming the
    //! member that disagrees.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<TypedUniformBlock> create(
        Program& p_program, std::string const& p_name, int p_binding = -1)
    {
        auto block_result = UniformBlock::create(p_program, p_name, p_binding);
        if (!block_result)
        {
            return compages::failure(block_result.error());
        }
        auto block = block_result.take();
        COMPAGES_TRY(detail::checkAgainstDriver(
            block, sizeof(T), std140::Description<T>::members));
        return TypedUniformBlock(std::move(block));
    }

    TypedUniformBlock(TypedUniformBlock&&) noexcept = default;
    TypedUniformBlock& operator=(TypedUniformBlock&&) noexcept = default;
    TypedUniformBlock(TypedUniformBlock const&) = delete;
    TypedUniformBlock& operator=(TypedUniformBlock const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Read what the block holds.
    // ------------------------------------------------------------------------
    [[nodiscard]] T const& value() const
    {
        return m_value;
    }

    // ------------------------------------------------------------------------
    //! \brief The struct, to be written to.
    //!
    //! \code
    //! block.modify().projection = projection;
    //! \endcode
    //!
    //! The whole struct is marked, not the member written, because a struct handed
    //! out by reference cannot say which of its members was touched. That costs
    //! nothing worth measuring, a block being small, and it is the lookup per
    //! member that this class exists to avoid rather than the copy.
    // ------------------------------------------------------------------------
    [[nodiscard]] T& modify()
    {
        m_pending = true;
        return m_value;
    }

    // ------------------------------------------------------------------------
    //! \brief Replace the whole struct.
    // ------------------------------------------------------------------------
    void assign(T const& p_value)
    {
        m_value = p_value;
        m_pending = true;
    }

    // ------------------------------------------------------------------------
    //! \brief Send the struct, if it has been written since the last time.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status update()
    {
        if (m_pending)
        {
            COMPAGES_TRY(m_block.setRaw(
                0u, std::as_bytes(std::span<const T>(&m_value, 1u))));
            m_pending = false;
        }
        return m_block.update();
    }

    // ------------------------------------------------------------------------
    //! \brief Make the block readable, sending the struct first if it changed.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind()
    {
        COMPAGES_TRY(update());
        return m_block.bind();
    }

    //! \brief Which numbered point this block is read from.
    [[nodiscard]] int binding() const
    {
        return m_block.binding();
    }

    //! \brief How many bytes the driver wants the block to be.
    [[nodiscard]] std::size_t bytes() const
    {
        return m_block.bytes();
    }

    //! \brief Is there a block here?
    [[nodiscard]] bool valid() const
    {
        return m_block.valid();
    }

    //! \brief Where the driver put every member.
    [[nodiscard]] std::string describe() const
    {
        return m_block.describe();
    }

    //! \brief The memory itself, for the parts of the library taking a buffer.
    [[nodiscard]] BufferHandle handle() const
    {
        return m_block.handle();
    }

    //! \brief The block underneath, for the odd write that has to go by name.
    [[nodiscard]] UniformBlock& untyped()
    {
        return m_block;
    }

private:

    explicit TypedUniformBlock(UniformBlock&& p_block)
        : m_block(std::move(p_block))
    {
    }

    UniformBlock m_block;
    T m_value{};
    //! \brief Has the struct been written since it was last sent? True to begin
    //! with, so that the first bind() sends it even if nobody wrote anything: a
    //! block of zeroes is what the caller asked for, and whatever the device
    //! happened to have in that memory is not.
    bool m_pending = true;
};

} // namespace compages::gpu
