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

#pragma once

#include "Compages/GPU/Core/Enums.hpp"
#include "Compages/GPU/Core/DirtyRange.hpp"
#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Errors.hpp"

#include <cstddef>
#include <algorithm>
#include <cassert>
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

struct BufferOptions
{
    BufferKind kind = BufferKind::Vertex;
    BufferUsage usage = BufferUsage::Dynamic;
    bool cpu_mirror = true;
};

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
//! \brief A block of memory on the device, holding a known number of a known
//! type.
//!
//! One class covers what the previous layer spread over separate types for
//! vertex data, indices and uniforms: what changes between them is the kind
//! given at creation, not the code. It also covers what the previous layer could
//! not express at all, since it insisted on one buffer per vertex attribute:
//! here a Buffer<Vertex> holds whole interleaved vertices.
//!
//! \code
//! struct Star { Vector3f position; Vector3f velocity; };
//!
//! auto buffer = compages::gpu::Buffer<Star>::create(1000000, compages::gpu::BufferKind::Storage,
//!                                        compages::gpu::BufferUsage::Storage);
//! if (!buffer) { std::cerr << buffer.error(); return; }
//! buffer.value().write(stars);
//! \endcode
//!
//! **A buffer that grows.** Default constructed, a Buffer keeps its elements on
//! the CPU as a std::vector would, and sends what changed at the next draw
//! (or upload()), growing the device memory as needed:
//! \code
//! compages::gpu::Buffer<Vertex> trail;
//! trail.emplace_back(position, color);      // sent at the next draw
//! trail[0].color = Vector3f(1, 0, 0);       // only this element is sent
//! compages::gpu::draw(pipeline, trail);
//! \endcode
//!
//! The memory is released when the Buffer is destroyed. A Buffer can be moved but
//! not copied, since two owners of the same memory would release it twice.
//!
//! \tparam T what one element is. Must be trivially copyable: the bytes are sent
//! to the device as they are, and a type owning memory elsewhere would send a
//! pointer the device cannot follow.
// ****************************************************************************
template <typename T>
class Buffer
{
public:

    static_assert(std::is_trivially_copyable_v<T>,
                  "a buffer sends the bytes of T straight to the device, so T "
                  "must be trivially copyable. A type holding a pointer, a "
                  "std::string or a std::vector would send an address the "
                  "device cannot follow");

    // ------------------------------------------------------------------------
    //! \brief An empty buffer, owning nothing on the device yet, keeping its
    //! elements on the CPU until the first upload().
    //!
    //! \param[in] p_kind what the device memory will be for.
    // ------------------------------------------------------------------------
    explicit Buffer(BufferKind p_kind = BufferKind::Vertex)
        : m_kind(p_kind), m_mirrored(true)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief A buffer holding these elements, sent at the first upload().
    // ------------------------------------------------------------------------
    Buffer(std::initializer_list<T> p_data, BufferKind p_kind = BufferKind::Vertex)
        : m_cpu(p_data), m_kind(p_kind), m_mirrored(true)
    {
        m_dirty.addAll(m_cpu.size());
    }

    // ------------------------------------------------------------------------
    //! \brief Same, copying a span.
    // ------------------------------------------------------------------------
    explicit Buffer(std::span<const T> p_data,
                    BufferKind p_kind = BufferKind::Vertex)
        : m_cpu(p_data.begin(), p_data.end()), m_kind(p_kind), m_mirrored(true)
    {
        m_dirty.addAll(m_cpu.size());
    }

    // ------------------------------------------------------------------------
    //! \brief Reserve room for a number of elements, without filling it.
    //!
    //! \param[in] p_count how many elements. Not zero: a buffer of nothing is
    //! almost always a count that was computed wrong.
    //! \param[in] p_kind what the buffer is for.
    //! \param[in] p_usage how often it will be written from the CPU. Immutable is
    //! refused here, since an immutable buffer can never be filled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Buffer> create(std::size_t p_count,
                                               BufferKind p_kind,
                                               BufferUsage p_usage)
    {
        if (p_usage == BufferUsage::Immutable)
        {
            return failure(
                "an immutable buffer can never be written, so it must be given "
                "its contents when created. Use Buffer::from() with the data, "
                "or ask for BufferUsage::Dynamic");
        }
        auto handle_result = detail::createBuffer(
                           p_count * sizeof(T), nullptr, p_kind, p_usage);
        if (!handle_result)
        {
            return compages::failure(handle_result.error());
        }
        auto handle = handle_result.take();
        return Buffer(handle, p_count);
    }

    // ------------------------------------------------------------------------
    //! \brief Reserve room and fill it in one go.
    //!
    //! The only way to make an immutable buffer, and the right one for geometry
    //! read from a file: the driver may then place it in the memory the device
    //! reads fastest, knowing it will never be written again.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Buffer> from(std::span<const T> p_data,
                                             BufferKind p_kind,
                                             BufferUsage p_usage)
    {
        auto handle_result = detail::createBuffer(p_data.size_bytes(),
                                            p_data.data(),
                                            p_kind,
                                            p_usage);
        if (!handle_result)
        {
            return compages::failure(handle_result.error());
        }
        auto handle = handle_result.take();
        return Buffer(handle, p_data.size());
    }

    [[nodiscard]] static Result<Buffer>
    from(std::span<const T> p_data, BufferOptions p_options = {})
    {
        auto buffer_result = from(p_data, p_options.kind, p_options.usage);
        if (!buffer_result)
        {
            return compages::failure(buffer_result.error());
        }
        auto buffer = buffer_result.take();
        buffer.m_kind = p_options.kind;
        if (p_options.cpu_mirror)
        {
            buffer.m_cpu.assign(p_data.begin(), p_data.end());
            buffer.m_mirrored = true;
        }
        return buffer;
    }

    template <std::ranges::contiguous_range Range>
        requires std::same_as<
            std::remove_cv_t<std::ranges::range_value_t<Range>>, T>
    [[nodiscard]] static Result<Buffer>
    from(Range const& p_data, BufferOptions p_options = {})
    {
        return from(
            std::span<const T>(std::ranges::data(p_data),
                               std::ranges::size(p_data)),
            p_options);
    }

    [[nodiscard]] static Result<Buffer> indices(std::span<const T> p_data)
    {
        return from(p_data, BufferKind::Index, BufferUsage::Immutable);
    }

    [[nodiscard]] static Result<Buffer>
    storage(std::span<const T> p_data,
            BufferUsage p_usage = BufferUsage::Storage)
    {
        return from(p_data, BufferKind::Storage, p_usage);
    }

    // ------------------------------------------------------------------------
    //! \brief Hand the memory over to another Buffer.
    // ------------------------------------------------------------------------
    Buffer(Buffer&& p_other) noexcept
        : m_handle(p_other.m_handle),
          m_count(p_other.m_count),
          m_cpu(std::move(p_other.m_cpu)),
          m_dirty(p_other.m_dirty),
          m_kind(p_other.m_kind),
          m_mirrored(p_other.m_mirrored),
          m_reserved(p_other.m_reserved)
    {
        p_other.m_handle = BufferHandle{};
        p_other.m_count = 0u;
        p_other.m_dirty.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Take over another Buffer's memory, releasing our own first.
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
            m_kind = p_other.m_kind;
            m_mirrored = p_other.m_mirrored;
            m_reserved = p_other.m_reserved;
            p_other.m_handle = BufferHandle{};
            p_other.m_count = 0u;
            p_other.m_dirty.clear();
        }
        return *this;
    }

    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Release the memory.
    // ------------------------------------------------------------------------
    ~Buffer()
    {
        release();
    }

    // ------------------------------------------------------------------------
    //! \brief Give the memory back now rather than at the end of the scope.
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
    //! \brief Overwrite the beginning of the buffer.
    //!
    //! \param[in] p_data the elements to write. Must fit.
    //! \param[in] p_first which element to start at.
    //!
    //! Too much data, or a buffer created immutable, is recorded as the frame
    //! error (see Errors.hpp) and nothing is written.
    // ------------------------------------------------------------------------
    void write(std::span<const T> p_data, std::size_t p_first = 0u)
    {
        if (m_mirrored && (p_first + p_data.size() <= m_cpu.size()))
        {
            std::copy(p_data.begin(), p_data.end(), m_cpu.begin() +
                      static_cast<std::ptrdiff_t>(p_first));
        }
        check(detail::writeBuffer(m_handle,
                                  p_first * sizeof(T),
                                  p_data.size_bytes(),
                                  p_data.data()));
    }

    // ------------------------------------------------------------------------
    //! \brief Overwrite one element.
    // ------------------------------------------------------------------------
    void write(T const& p_value, std::size_t p_index)
    {
        if (m_mirrored && (p_index < m_cpu.size()))
        {
            m_cpu[p_index] = p_value;
        }
        check(detail::writeBuffer(
            m_handle, p_index * sizeof(T), sizeof(T), &p_value));
    }

    // ------------------------------------------------------------------------
    //! \brief One element, to be changed on the CPU. Sent at the next upload.
    //!
    //! Only for a buffer keeping its elements on the CPU (see mirrored()).
    // ------------------------------------------------------------------------
    [[nodiscard]] T& operator[](std::size_t p_index)
    {
        assert(m_mirrored && (p_index < m_cpu.size()) &&
               "writing past the end of a Buffer, or into one without a CPU copy");
        m_dirty.add(p_index);
        return m_cpu[p_index];
    }

    //! \brief One element, read from the CPU copy.
    [[nodiscard]] T const& operator[](std::size_t p_index) const
    {
        assert(m_mirrored && (p_index < m_cpu.size()) &&
               "reading past the end of a Buffer, or one without a CPU copy");
        return m_cpu[p_index];
    }

    //! \brief Change one element on the CPU. Sent at the next upload.
    void set(std::size_t p_index, T const& p_value)
    {
        (*this)[p_index] = p_value;
    }

    // ------------------------------------------------------------------------
    //! \brief Does this buffer keep its elements on the CPU too?
    //!
    //! True for one default constructed, or made with BufferOptions::cpu_mirror.
    //! Only such a buffer can grow, be changed element by element, or be
    //! read back with download().
    // ------------------------------------------------------------------------
    [[nodiscard]] bool mirrored() const
    {
        return m_mirrored;
    }

    //! \brief Is there nothing in the buffer?
    [[nodiscard]] bool empty() const
    {
        return count() == 0u;
    }

    //! \brief Add one element at the end, built from \c p_args. Sent at the
    //! next upload.
    template <typename... Args>
    T& emplace_back(Args&&... p_args)
    {
        assert(m_mirrored && "emplace_back() on a Buffer without a CPU copy");
        T& added = m_cpu.emplace_back(std::forward<Args>(p_args)...);
        m_dirty.add(m_cpu.size() - 1u);
        return added;
    }

    // ------------------------------------------------------------------------
    //! \brief Change the number of elements. New ones are value initialised
    //! and sent at the next upload; removed ones are simply no longer drawn.
    // ------------------------------------------------------------------------
    void resize(std::size_t p_count)
    {
        assert(m_mirrored && "resize() on a Buffer without a CPU copy");
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
    //! \brief Say how many elements to expect, so that the device memory is
    //! reserved once rather than grown several times.
    // ------------------------------------------------------------------------
    void reserve(std::size_t p_count)
    {
        m_cpu.reserve(p_count);
        m_reserved = std::max(m_reserved, p_count);
    }

    //! \brief Remove every element, keeping the device memory for later.
    void clear()
    {
        m_cpu.clear();
        m_dirty.clear();
    }

    //! \brief Replace every element. Sent at the next upload.
    void assign(std::span<const T> p_data)
    {
        assert(m_mirrored && "assign() on a Buffer without a CPU copy");
        m_cpu.assign(p_data.begin(), p_data.end());
        m_dirty.addAll(m_cpu.size());
    }

    //! \brief Same, from braces.
    void assign(std::initializer_list<T> p_data)
    {
        assign(std::span<const T>(p_data.begin(), p_data.size()));
    }

    // ------------------------------------------------------------------------
    //! \brief Every element, to be changed on the CPU. All are sent at the
    //! next upload.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<T> modify()
    {
        m_dirty.addAll(m_cpu.size());
        return { m_cpu.data(), m_cpu.size() };
    }

    //! \brief Some elements, to be changed on the CPU; only they are sent.
    [[nodiscard]] std::span<T> modify(std::size_t p_first, std::size_t p_count)
    {
        assert(((p_first + p_count) <= m_cpu.size()) &&
               "modifying past the end of a Buffer");
        m_dirty.add(p_first, p_count);
        return { m_cpu.data() + p_first, p_count };
    }

    //! \brief The elements, as the CPU holds them.
    [[nodiscard]] std::span<const T> elements() const
    {
        return { m_cpu.data(), m_cpu.size() };
    }

    //! \brief Has anything changed on the CPU since the last upload?
    [[nodiscard]] bool dirty() const
    {
        return !m_dirty.empty();
    }

    //! \brief Which elements the next upload will send.
    [[nodiscard]] DirtyRange const& pending() const
    {
        return m_dirty;
    }

    //! \brief How many elements the device memory has room for.
    [[nodiscard]] std::size_t deviceCapacity() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Send what changed on the CPU, growing the device memory first
    //! when it has become too small.
    //!
    //! A draw calls it on a non const buffer, so it rarely needs calling.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status upload()
    {
        if (!m_mirrored || m_cpu.empty())
        {
            m_dirty.clear();
            return success();
        }
        if (!valid() || (m_count < m_cpu.size()))
        {
            // Doubling keeps emplace_back cheap: the device memory is replaced a
            // logarithmic number of times, not once per element.
            std::size_t capacity = std::max(m_cpu.size(), m_reserved);
            capacity = std::max(capacity, m_count * 2u);
            auto created = detail::createBuffer(capacity * sizeof(T), nullptr,
                                                m_kind, BufferUsage::Dynamic);
            if (!created)
            {
                return compages::failure(created.error());
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
        const std::size_t first = m_dirty.begin();
        const std::size_t count = m_dirty.count();
        COMPAGES_TRY(detail::writeBuffer(m_handle,
                                         first * sizeof(T),
                                         count * sizeof(T),
                                         m_cpu.data() + first));
        m_dirty.clear();
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Replace the existing CPU mirror with the device contents.
    //!
    //! This is deliberately available only on mirrored buffers: silently
    //! allocating a mirror here would make an accidental readback in a frame
    //! loop hard to spot.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status download()
    {
        if (!m_mirrored)
        {
            return failure(
                "Buffer::download() needs a CPU mirror. Create the buffer with "
                "BufferOptions::cpu_mirror = true (the default)");
        }
        COMPAGES_TRY(detail::readBuffer(
            m_handle, 0u, m_cpu.size() * sizeof(T), m_cpu.data()));
        m_dirty.clear();
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Read elements back into CPU memory.
    //!
    //! Waits for the device to finish with the memory, so it belongs in tests and
    //! in debugging rather than in a frame. A compute pass that needs its result
    //! on the CPU every frame wants a second buffer and one frame of delay
    //! instead.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status read(std::span<T> p_into, std::size_t p_first = 0u) const
    {
        return detail::readBuffer(m_handle,
                                  p_first * sizeof(T),
                                  p_into.size_bytes(),
                                  p_into.data());
    }

    // ------------------------------------------------------------------------
    //! \brief Read the whole buffer back into a vector.
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
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t count() const
    {
        return m_mirrored ? m_cpu.size() : m_count;
    }

    //! \brief Same as count(), for code written against std::vector.
    [[nodiscard]] std::size_t size() const
    {
        return count();
    }

    // ------------------------------------------------------------------------
    //! \brief How many bytes the buffer occupies on the device.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const
    {
        return m_count * sizeof(T);
    }

    // ------------------------------------------------------------------------
    //! \brief Does this Buffer own anything?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const
    {
        return detail::bufferAlive(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief What the buffer is for.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferKind kind() const
    {
        return detail::bufferKind(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief How often the buffer may be written.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferUsage usage() const
    {
        return detail::bufferUsage(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief The handle naming this memory, for the parts of the library that
    //! take one, such as a draw call.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferHandle handle() const
    {
        return m_handle;
    }

private:

    Buffer(BufferHandle p_handle, std::size_t p_count)
        : m_handle(p_handle), m_count(p_count), m_mirrored(false)
    {
    }

    BufferHandle m_handle;
    //! \brief How many elements the device memory has room for.
    std::size_t m_count = 0u;
    std::vector<T> m_cpu;
    DirtyRange m_dirty;
    BufferKind m_kind = BufferKind::Vertex;
    bool m_mirrored = true;
    std::size_t m_reserved = 0u;
};

// ----------------------------------------------------------------------------
//! \brief How many buffers are alive right now.
//!
//! What the examples gallery watches: a demo that has been closed and still
//! leaves buffers behind is leaking them.
// ----------------------------------------------------------------------------
[[nodiscard]] inline std::size_t liveBuffers()
{
    return detail::liveBufferCount();
}

} // namespace compages::gpu
