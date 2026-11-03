// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief A table of values stored in the memory of the graphics card.
//!
//! The graphics card has its own memory, far from the processor. Everything it
//! draws or computes must first be copied there: the corners of the triangles,
//! the order to join them in, the numbers a compute pass works on. A Buffer is
//! one such table, used much like a std::vector, that knows how to send itself
//! to the card and, when asked, to bring its values back.
//!
//! What it holds:
//! - Buffer<T, Kind>: the table itself, with the elements of type T, and its
//!   three names VertexBuffer<T>, IndexBuffer<T> and StorageBuffer<T>.
//! - liveBuffers(): how many buffers exist, to spot the ones never released.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/DirtyRange.hpp"
#include "Compages/GPU/Enums.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Handle.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace compages::gpu
{

//! \brief Names memory living on the device.
using BufferHandle = Handle<struct BufferTag>;

namespace detail
{

// ----------------------------------------------------------------------------
// The work of a buffer, done once in bytes rather than once per element type.
// A template that only converts counts into bytes keeps the code the compiler
// has to stamp out for every vertex struct down to almost nothing.
// ----------------------------------------------------------------------------

//! \brief Reserve memory on the device. See compages::gpu::Buffer::create().
[[nodiscard]] Result<BufferHandle> createBuffer(std::size_t p_bytes,
                                                const void* p_data,
                                                BufferKind p_kind,
                                                BufferUsage p_usage);

//! \brief Give the memory back. Does nothing on a handle already released.
void destroyBuffer(BufferHandle p_handle);

//! \brief Overwrite part of a buffer, checking the range and the usage first.
[[nodiscard]] Status writeBuffer(BufferHandle p_handle,
                                 std::size_t p_offset,
                                 std::size_t p_bytes,
                                 const void* p_data);

//! \brief Read part of a buffer back.
[[nodiscard]] Status readBuffer(BufferHandle p_handle,
                                std::size_t p_offset,
                                std::size_t p_bytes,
                                void* p_data);

//! \brief Size in bytes, or 0 when the handle is stale.
[[nodiscard]] std::size_t bufferBytes(BufferHandle p_handle);

//! \brief Is the buffer still alive?
[[nodiscard]] bool bufferAlive(BufferHandle p_handle);

//! \brief What the buffer is for.
[[nodiscard]] BufferKind bufferKind(BufferHandle p_handle);

//! \brief How often the buffer may be written.
[[nodiscard]] BufferUsage bufferUsage(BufferHandle p_handle);

//! \brief How many buffers are alive. Used to spot leaks.
[[nodiscard]] std::size_t liveBufferCount();

//! \brief How many bytes of buffers are held, as they were asked for.
[[nodiscard]] std::size_t bufferMemory();

} // namespace detail

// ****************************************************************************
//! \brief A list of values that lives on the graphics card, used like a
//! std::vector.
//!
//! What the card uses it for is part of its type, so that a mistake such as
//! drawing indices from a buffer of vertices does not compile. Use one of the
//! three names: VertexBuffer<T> (corners of shapes), IndexBuffer<T> (the order
//! to join them) and StorageBuffer<T> (data a compute pass reads and writes,
//! which can also be drawn as vertices).
//!
//! How it lives is chosen at creation, with the OpenGL names (BufferUsage):
//! \code
//! namespace gpu = compages::gpu;
//! // Static: written once, never again by the CPU. A mesh from a file.
//! auto mesh = gpu::VertexBuffer<Vertex>::from(corners, gpu::BufferUsage::Static);
//! // Stream: on the card only, rewritten by write(). The CPU keeps its data.
//! auto skin = gpu::VertexBuffer<Vertex>::create(count, gpu::BufferUsage::Stream);
//! skin.write(posed);
//! // Dynamic: a CPU copy too. Edit it like a vector; upload() sends only
//! // what changed. A default constructed buffer is Dynamic and empty.
//! gpu::VertexBuffer<Vertex> trail;
//! trail.emplace_back(Vertex{ position, color });
//! trail[0].color = red;
//! COMPAGES_TRY(trail.upload());
//! \endcode
//!
//! The card memory is released when the Buffer is destroyed. A Buffer can be
//! moved but not copied, since two owners would release the same memory twice.
//!
//! \tparam T one element, often a vertex struct. Must be trivially copyable:
//! its bytes are sent as they are, so a member owning memory elsewhere (a
//! pointer, a std::string) would send an address the card cannot follow.
//! \tparam Kind what the card uses it for.
// ****************************************************************************
template <typename T, BufferKind Kind>
class Buffer: private compages::NotCopiable
{
private:

    Buffer(BufferHandle p_handle, std::size_t p_count, BufferUsage p_usage)
        : m_handle(p_handle), m_count(p_count), m_usage(p_usage)
    {
    }

public:

    static_assert(std::is_trivially_copyable_v<T>,
                  "a buffer sends the bytes of T straight to the device, so T "
                  "must be trivially copyable. A type holding a pointer, a "
                  "std::string or a std::vector would send an address the "
                  "device cannot follow");

    //! \brief What the card uses this buffer for.
    static constexpr BufferKind kind = Kind;

    // ------------------------------------------------------------------------
    //! \brief An empty Dynamic buffer, which grows as needed. No card memory
    //! is reserved until the first upload() or draw, so valid() is false
    //! until then.
    // ------------------------------------------------------------------------
    Buffer() = default;

    // ------------------------------------------------------------------------
    //! \brief Reserve card memory for p_count elements and fill it with
    //! p_data, all in one go.
    //!
    //! \code
    //! auto mesh = compages::gpu::VertexBuffer<Vertex>::from(
    //!     corners, compages::gpu::BufferUsage::Static);
    //! \endcode
    //!
    //! \param[in] p_data the elements to send: any contiguous container of T.
    //! \param[in] p_usage Static (never written again by the CPU), Stream
    //! (rewritten by write(), no CPU copy) or Dynamic (a CPU copy is kept, to
    //! edit and upload()).
    //! \return the buffer, or why the card memory could not be reserved.
    // ------------------------------------------------------------------------
    template <std::ranges::contiguous_range Range>
        requires std::
            same_as<std::remove_cv_t<std::ranges::range_value_t<Range>>, T>
        [[nodiscard]] static Result<Buffer> from(Range const& p_data,
                                                 BufferUsage p_usage)
    {
        const std::span<const T> data(std::ranges::data(p_data),
                                      std::ranges::size(p_data));
        auto handle = detail::createBuffer(
            data.size_bytes(), data.data(), Kind, p_usage);
        if (!handle)
        {
            return failure(handle.error());
        }
        Buffer buffer(handle.take(), data.size(), p_usage);
        if (p_usage == BufferUsage::Dynamic)
        {
            buffer.m_cpu.assign(data.begin(), data.end());
        }
        return buffer;
    }

    // ------------------------------------------------------------------------
    //! \brief Reserve card memory for p_count elements, without data to put
    //! in it yet.
    //!
    //! \code
    //! auto out = compages::gpu::StorageBuffer<float>::create(
    //!     count, compages::gpu::BufferUsage::Static);   // a compute pass fills it
    //! \endcode
    //!
    //! \param[in] p_count how many elements. Not zero: a buffer of nothing is
    //! almost always a count that was computed wrong.
    //! \param[in] p_usage Static for a StorageBuffer the card fills itself
    //! (refused for the other kinds, which nothing could fill), Stream for
    //! data the CPU will write(), Dynamic for a CPU copy of p_count
    //! value-initialised elements.
    //! \return the buffer, or why the card memory could not be reserved.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Buffer> create(std::size_t p_count,
                                               BufferUsage p_usage)
    {
        if (p_usage == BufferUsage::Dynamic)
        {
            return from(std::vector<T>(p_count), p_usage);
        }
        auto handle =
            detail::createBuffer(p_count * sizeof(T), nullptr, Kind, p_usage);
        if (!handle)
        {
            return failure(handle.error());
        }
        return Buffer(handle.take(), p_count, p_usage);
    }

    // ------------------------------------------------------------------------
    //! \brief Take over the card memory and the CPU copy of p_other, which
    //! is left empty and no longer valid().
    //!
    //! \param[in,out] p_other the buffer to empty.
    // ------------------------------------------------------------------------
    Buffer(Buffer&& p_other) noexcept
        : m_handle(p_other.m_handle),
          m_count(p_other.m_count),
          m_cpu(std::move(p_other.m_cpu)),
          m_dirty(p_other.m_dirty),
          m_usage(p_other.m_usage),
          m_reserved(p_other.m_reserved)
    {
        p_other.m_handle = BufferHandle{};
        p_other.m_count = 0u;
        p_other.m_dirty.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Release this buffer's card memory, then take over p_other's.
    //!
    //! \param[in,out] p_other the buffer to empty.
    //! \return this object, now owning what p_other owned.
    // ------------------------------------------------------------------------
    Buffer& operator=(Buffer&& p_other) noexcept
    {
        if (this != &p_other)
        {
            release();
            m_handle = p_other.m_handle;
            m_count = p_other.m_count;
            m_cpu = std::move(p_other.m_cpu);
            m_dirty = p_other.m_dirty;
            m_usage = p_other.m_usage;
            m_reserved = p_other.m_reserved;
            p_other.m_handle = BufferHandle{};
            p_other.m_count = 0u;
            p_other.m_dirty.clear();
        }
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Release the card memory. Safe after a move or a release().
    // ------------------------------------------------------------------------
    ~Buffer()
    {
        release();
    }

    // ------------------------------------------------------------------------
    //! \brief Give the card memory back now rather than at the end of the
    //! scope, and empty the CPU copy.
    //!
    //! The Buffer stays usable: a growable one reserves new memory at its next
    //! upload(). Releasing before gpu::shutdown() is what keeps the leak
    //! counters of --check at zero for a buffer owned by a long-lived object.
    // ------------------------------------------------------------------------
    void release()
    {
        detail::destroyBuffer(m_handle);
        m_handle = BufferHandle{};
        m_count = 0u;
        m_cpu.clear();
        m_dirty.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Send elements to the card now, without waiting for the next
    //! upload().
    //!
    //! The CPU copy, when there is one, is updated too. The buffer does not
    //! grow: a growable buffer that has never been uploaded has no card memory
    //! yet, and the write fails.
    //!
    //! \param[in] p_data the elements to write. Must fit in deviceCapacity().
    //! \param[in] p_first index of the first element to overwrite.
    //!
    //! Too much data, or a Static buffer, is recorded as the frame error (see
    //! takeFrameError()) and nothing is written.
    // ------------------------------------------------------------------------
    void write(std::span<const T> p_data, std::size_t p_first = 0u)
    {
        if (mirrored() && (p_first + p_data.size() <= m_cpu.size()))
        {
            std::copy(p_data.begin(),
                      p_data.end(),
                      m_cpu.begin() + static_cast<std::ptrdiff_t>(p_first));
        }
        check(detail::writeBuffer(
            m_handle, p_first * sizeof(T), p_data.size_bytes(), p_data.data()));
    }

    // ------------------------------------------------------------------------
    //! \brief Send one element to the card now. Same rules as the span
    //! overload.
    //!
    //! \param[in] p_value the new element.
    //! \param[in] p_index which element to overwrite.
    // ------------------------------------------------------------------------
    void write(T const& p_value, std::size_t p_index)
    {
        if (mirrored() && (p_index < m_cpu.size()))
        {
            m_cpu[p_index] = p_value;
        }
        check(detail::writeBuffer(
            m_handle, p_index * sizeof(T), sizeof(T), &p_value));
    }

    // ------------------------------------------------------------------------
    //! \brief One element of the CPU copy, marked as changed: it is sent at
    //! the next upload() even if it is only read.
    //!
    //! Only for a Dynamic buffer, which keeps a CPU copy. An index past the
    //! end stops a debug build; a release build does not check.
    //!
    //! \param[in] p_index which element.
    //! \return the element, which may be changed in place.
    // ------------------------------------------------------------------------
    [[nodiscard]] T& operator[](std::size_t p_index)
    {
        assert(
            mirrored() && (p_index < m_cpu.size()) &&
            "writing past the end of a Buffer, or into one without a CPU copy");
        m_dirty.add(p_index);
        return m_cpu[p_index];
    }

    // ------------------------------------------------------------------------
    //! \brief One element of the CPU copy, read only, so nothing is marked to
    //! send. May differ from the card after a compute pass: see download().
    //! \param[in] p_index which element.
    //! \return the element as the CPU copy holds it.
    // ------------------------------------------------------------------------
    [[nodiscard]] T const& operator[](std::size_t p_index) const
    {
        assert(mirrored() && (p_index < m_cpu.size()) &&
               "reading past the end of a Buffer, or one without a CPU copy");
        return m_cpu[p_index];
    }

    // ------------------------------------------------------------------------
    //! \brief Change one element of the CPU copy; sent at the next upload().
    //! Unlike write(), does not touch the card now.
    //! \param[in] p_index which element.
    //! \param[in] p_value the new element.
    // ------------------------------------------------------------------------
    void set(std::size_t p_index, T const& p_value)
    {
        (*this)[p_index] = p_value;
    }

    // ------------------------------------------------------------------------
    //! \brief True when count() is zero. A buffer can be empty and still hold
    //! card memory, after clear() for instance.
    //! \return true when there is no element.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return count() == 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Add one element at the end, as std::vector::emplace_back does.
    //! Sent at the next upload(), which grows the card memory when needed.
    //! \param[in] p_args the arguments of T's constructor, or its fields.
    //! \return the new element, valid until the next growth of the CPU copy.
    // ------------------------------------------------------------------------
    template <typename... Args>
    T& emplace_back(Args&&... p_args)
    {
        assert(mirrored() && "emplace_back() on a Buffer without a CPU copy");
        T& added = m_cpu.emplace_back(std::forward<Args>(p_args)...);
        m_dirty.add(m_cpu.size() - 1u);
        return added;
    }

    // ------------------------------------------------------------------------
    //! \brief Change the number of elements. New ones are value initialised
    //! and sent at the next upload(); removed ones are no longer drawn, but
    //! the card memory is not shrunk.
    //!
    //! \param[in] p_count the new number of elements.
    // ------------------------------------------------------------------------
    void resize(std::size_t p_count)
    {
        assert(mirrored() && "resize() on a Buffer without a CPU copy");
        const std::size_t before = m_cpu.size();
        m_cpu.resize(p_count);
        if (p_count > before)
        {
            m_dirty.add(before, p_count - before);
        }
        else
        {
            m_dirty.clampTo(p_count);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Say how many elements to expect, so that the card memory is
    //! reserved once rather than grown several times.
    //!
    //! Takes effect at the next growth, in upload(): nothing is reserved on the
    //! card right now.
    //!
    //! \param[in] p_count the number of elements expected.
    // ------------------------------------------------------------------------
    void reserve(std::size_t p_count)
    {
        m_cpu.reserve(p_count);
        m_reserved = std::max(m_reserved, p_count);
    }

    // ------------------------------------------------------------------------
    //! \brief Remove every element, keeping the card memory so that refilling
    //! it, the next frame for instance, reserves nothing.
    // ------------------------------------------------------------------------
    void clear()
    {
        m_cpu.clear();
        m_dirty.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Replace every element; all are sent at the next upload().
    //! \param[in] p_data the new elements, copied.
    // ------------------------------------------------------------------------
    void assign(std::span<const T> p_data)
    {
        assert(mirrored() && "assign() on a Buffer without a CPU copy");
        m_cpu.assign(p_data.begin(), p_data.end());
        m_dirty.addAll(m_cpu.size());
    }

    // ------------------------------------------------------------------------
    //! \brief Same as assign(std::span), from braces.
    //! \param[in] p_data the new elements.
    // ------------------------------------------------------------------------
    void assign(std::initializer_list<T> p_data)
    {
        assign(std::span<const T>(p_data.begin(), p_data.size()));
    }

    // ------------------------------------------------------------------------
    //! \brief Every element of the CPU copy, to change in place. All are
    //! sent at the next upload(), changed or not.
    //!
    //! \code
    //! for (Particle& p : particles.modify())
    //! {
    //!     p.position += p.velocity * dt;
    //! }
    //! \endcode
    //! \return every element of the CPU copy; empty when there is none.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<T> modify()
    {
        m_dirty.addAll(m_cpu.size());
        return { m_cpu.data(), m_cpu.size() };
    }

    // ------------------------------------------------------------------------
    //! \brief Some elements of the CPU copy, to change in place; only they
    //! are sent at the next upload().
    //! \param[in] p_first index of the first element.
    //! \param[in] p_count how many elements, which must not run past the end.
    //! \return the p_count elements from p_first, to change in place.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<T> modify(std::size_t p_first, std::size_t p_count)
    {
        assert(((p_first + p_count) <= m_cpu.size()) &&
               "modifying past the end of a Buffer");
        m_dirty.add(p_first, p_count);
        return { m_cpu.data() + p_first, p_count };
    }

    // ------------------------------------------------------------------------
    //! \brief The CPU copy, read only. Empty when the buffer has none.
    //! \return the elements of the CPU copy, in order.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<const T> elements() const
    {
        return { m_cpu.data(), m_cpu.size() };
    }

    // ------------------------------------------------------------------------
    //! \brief True when the CPU copy holds changes the card has not received.
    //! \return true when the next upload() has something to send.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool dirty() const
    {
        return !m_dirty.empty();
    }

    // ------------------------------------------------------------------------
    //! \brief Which elements the next upload() will send: one range from the
    //! first edit to the last, the unchanged elements between them included.
    //! \return what to send; empty when nothing changed.
    // ------------------------------------------------------------------------
    [[nodiscard]] DirtyRange const& pending() const
    {
        return m_dirty;
    }

    // ------------------------------------------------------------------------
    //! \brief How many elements the card memory has room for. Can exceed
    //! count(), since a growable buffer doubles its memory when it grows.
    //! \return the room on the card, in elements; 0 before the first upload.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t deviceCapacity() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Send what changed on the CPU, growing the card memory first
    //! when it has become too small.
    //!
    //! A draw already calls it on a non const buffer, so call it yourself only
    //! before a compute pass or a read by another program. Growing doubles the
    //! capacity, which keeps a long run of emplace_back() cheap, and resends
    //! everything. Does nothing on a buffer without a CPU copy.
    //!
    //! \return why the card memory could not be grown or written.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status upload()
    {
        if (!mirrored() || m_cpu.empty())
        {
            m_dirty.clear();
            return success();
        }
        if (!valid() || (m_count < m_cpu.size()))
        {
            // Doubling keeps emplace_back cheap: the device memory is replaced
            // a logarithmic number of times, not once per element.
            std::size_t capacity = std::max(m_cpu.size(), m_reserved);
            capacity = std::max(capacity, m_count * 2u);
            auto created = detail::createBuffer(
                capacity * sizeof(T), nullptr, Kind, BufferUsage::Dynamic);
            if (!created)
            {
                return failure(created.error());
            }
            detail::destroyBuffer(m_handle);
            m_handle = created.value();
            m_count = capacity;
            m_dirty.addAll(m_cpu.size());
        }
        m_dirty.clampTo(m_cpu.size());
        if (m_dirty.empty())
        {
            return success();
        }
        COMPAGES_TRY(detail::writeBuffer(m_handle,
                                         m_dirty.begin() * sizeof(T),
                                         m_dirty.count() * sizeof(T),
                                         m_cpu.data() + m_dirty.begin()));
        m_dirty.clear();
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Overwrite the CPU copy with what the card holds, to see the
    //! results of a compute pass.
    //!
    //! CPU changes not yet sent are lost. Waits for the card to finish, so keep
    //! it out of the frame loop. Only on a buffer with a CPU copy: creating one
    //! silently here would hide an accidental read back every frame.
    //!
    //! \return an error on a buffer without a CPU copy.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status download()
    {
        if (!mirrored())
        {
            return failure(
                "Buffer::download() needs a CPU copy: create the buffer "
                "Dynamic, or use read() to get the elements in a vector");
        }
        COMPAGES_TRY(detail::readBuffer(
            m_handle, 0u, m_cpu.size() * sizeof(T), m_cpu.data()));
        m_dirty.clear();
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Copy elements from the card into memory of your own. Works with
    //! or without a CPU copy, which it leaves untouched.
    //!
    //! Waits for the device to finish with the memory, so it belongs in tests
    //! and in debugging rather than in a frame. A compute pass that needs its
    //! result on the CPU every frame wants a second buffer and one frame of
    //! delay instead.
    //!
    //! \param[out] p_into where to copy; its size says how many elements.
    //! \param[in] p_first index of the first element to read.
    //! \return an error when the range runs past the buffer or the buffer is
    //! gone.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status read(std::span<T> p_into,
                              std::size_t p_first = 0u) const
    {
        return detail::readBuffer(
            m_handle, p_first * sizeof(T), p_into.size_bytes(), p_into.data());
    }

    // ------------------------------------------------------------------------
    //! \brief Copy count() elements from the card into a new vector. Waits
    //! for the card, like read(std::span, std::size_t).
    //! \return the elements, or why they could not be read.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<std::vector<T>> read() const
    {
        std::vector<T> out(count());
        COMPAGES_TRY(detail::readBuffer(
            m_handle, 0u, out.size() * sizeof(T), out.data()));
        return out;
    }

    // ------------------------------------------------------------------------
    //! \brief How many elements the buffer holds: those on the CPU when it
    //! keeps them there, which is what a draw reads.
    //! \return the number of elements.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t count() const
    {
        return mirrored() ? m_cpu.size() : m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Same as count(), for code written against std::vector.
    //! \return the number of elements.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const
    {
        return count();
    }

    // ------------------------------------------------------------------------
    //! \brief How many bytes of card memory are reserved: deviceCapacity()
    //! elements, not count().
    //! \return the bytes reserved on the card; 0 when not valid().
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const
    {
        return m_count * sizeof(T);
    }

    // ------------------------------------------------------------------------
    //! \brief True when card memory is reserved. False after a move or a
    //! release(), and for a growable buffer not yet uploaded.
    //! \return true when the buffer can be drawn or bound.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const
    {
        return detail::bufferAlive(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief How the buffer lives, as given at creation.
    //! \return Static, Stream or Dynamic; Dynamic for a default constructed
    //! buffer.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferUsage usage() const
    {
        return m_usage;
    }

    // ------------------------------------------------------------------------
    //! \brief The handle naming this memory, for the parts of the library that
    //! take one, such as a draw call.
    //! \return the handle; empty before the first upload or after release().
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferHandle handle() const
    {
        return m_handle;
    }

private:

    //! \brief A Dynamic buffer keeps a CPU copy; the others do not.
    [[nodiscard]] bool mirrored() const
    {
        return m_usage == BufferUsage::Dynamic;
    }

    //! \brief Names the card memory; empty before the first upload.
    BufferHandle m_handle;
    //! \brief How many elements the card memory has room for.
    std::size_t m_count = 0u;
    //! \brief Dynamic only: the CPU copy, edited by operator[] and the like.
    std::vector<T> m_cpu;
    //! \brief Dynamic only: the elements the card has not received yet.
    DirtyRange m_dirty;
    //! \brief How the buffer lives, given at creation.
    BufferUsage m_usage = BufferUsage::Dynamic;
    //! \brief Dynamic only: the capacity reserve() asked for, used at the
    //! next growth.
    std::size_t m_reserved = 0u;
};

//! \brief The kinds of Buffer a draw can read vertices from: Vertex, and
//! Storage (particles a compute pass moves, drawn where they are).
template <BufferKind Kind>
concept VertexSource = (Kind == BufferKind::Vertex) ||
                       (Kind == BufferKind::Storage);

//! \brief Corners of shapes, read by a vertex shader.
template <typename T>
using VertexBuffer = Buffer<T, BufferKind::Vertex>;
//! \brief The order in which to join vertices: std::uint16_t or
//! std::uint32_t.
template <typename T>
using IndexBuffer = Buffer<T, BufferKind::Index>;
//! \brief Data a compute pass reads and writes. Can also be drawn as
//! vertices, which is how particles moved on the card are drawn.
template <typename T>
using StorageBuffer = Buffer<T, BufferKind::Storage>;

// ----------------------------------------------------------------------------
//! \brief How many buffers hold card memory right now, all types together.
//!
//! What the examples gallery watches: a demo that has been closed and still
//! leaves buffers behind is leaking them.
//! \return the number of live buffers; 0 once every demo has released its own.
// ----------------------------------------------------------------------------
[[nodiscard]] inline std::size_t liveBuffers()
{
    return detail::liveBufferCount();
}

} // namespace compages::gpu
