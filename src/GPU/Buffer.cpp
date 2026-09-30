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

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Internal/Pools.hpp"

#include <cassert>
#include <string>

namespace compages::gpu::detail
{

namespace
{

//------------------------------------------------------------------------------
//! \brief The message given when a handle names nothing alive.
//!
//! Worth spelling out rather than saying "invalid handle": the two ways to get
//! here are a buffer released too early and a Buffer that was moved from, and
//! naming both saves the reader a debugging session.
//------------------------------------------------------------------------------
std::string staleHandleMessage()
{
    return "this buffer no longer exists. Either it was released while "
           "something "
           "still referred to it, or the Buffer object was moved from and the "
           "old one is being used";
}

} // namespace

//------------------------------------------------------------------------------
Result<BufferHandle> createBuffer(std::size_t p_bytes,
                                  const void* p_data,
                                  BufferKind p_kind,
                                  BufferUsage p_usage)
{
    if (!initialized())
    {
        return failure(
            "compages::gpu::init() has not been called, so there is no device "
            "to put a buffer on");
    }

    if (p_bytes == 0u)
    {
        return failure("a buffer of zero bytes cannot be created. An element "
                       "count of zero is almost always a count computed wrong");
    }

    if ((p_usage == BufferUsage::Immutable) && (p_data == nullptr))
    {
        return failure(
            "an immutable buffer can never be written, so it must be "
            "given its contents when created");
    }

    // A storage buffer is what a compute pass writes into, and the driver has
    // its own ceiling on how big one may be. Saying which limit was passed is
    // more useful than letting the driver refuse without explanation.
    if (p_kind == BufferKind::Storage)
    {
        const auto limit =
            static_cast<std::size_t>(device().max_shader_storage_block_size);
        if (p_bytes > limit)
        {
            return failure(
                "a storage buffer of " + std::to_string(p_bytes) +
                " bytes was asked for but this driver allows at most " +
                std::to_string(limit));
        }
    }
    if (p_kind == BufferKind::Uniform)
    {
        const auto limit =
            static_cast<std::size_t>(device().max_uniform_block_size);
        if (p_bytes > limit)
        {
            return failure(
                "a uniform buffer of " + std::to_string(p_bytes) +
                " bytes was asked for but this driver allows at most " +
                std::to_string(limit) +
                ". Uniform blocks are meant to be small; put large data in a "
                "storage buffer instead");
        }
    }

    auto native_result =
        backend::createBuffer(p_bytes, p_data, p_kind, p_usage);
    if (!native_result)
    {
        return failure(native_result.error());
    }
    auto native = native_result.take();

    auto added =
        pools().buffers.add(BufferRecord{ native, p_bytes, p_kind, p_usage });
    if (!added)
    {
        // The pool is full, so the memory we just reserved has nowhere to be
        // remembered. Give it back rather than leak it.
        backend::destroyBuffer(native);
        return failure(added.error());
    }
    return added.take();
}

//------------------------------------------------------------------------------
void destroyBuffer(BufferHandle p_handle)
{
    BufferRecord* record = pools().buffers.get(p_handle);
    if (record == nullptr)
    {
        // Releasing an empty or already released handle is not a mistake: it is
        // what a moved-from Buffer does when it goes out of scope.
        return;
    }

    // After compages::gpu::shutdown() the driver has nothing left to free the
    // memory on, and the pool has already been emptied, so this branch means
    // the device is gone while a Buffer object is still around.
    if (initialized())
    {
        backend::destroyBuffer(record->native);
    }
    (void)pools().buffers.remove(p_handle);
}

//------------------------------------------------------------------------------
Status writeBuffer(BufferHandle p_handle,
                   std::size_t p_offset,
                   std::size_t p_bytes,
                   const void* p_data)
{
    BufferRecord* record = pools().buffers.get(p_handle);
    if (record == nullptr)
    {
        return failure(staleHandleMessage());
    }

    if (record->usage == BufferUsage::Immutable)
    {
        return failure(
            "this buffer was created immutable, which is a promise to the "
            "driver "
            "that it would never be written again in exchange for the fastest "
            "memory available. Create it with BufferUsage::Dynamic to write "
            "it");
    }

    if (p_bytes == 0u)
    {
        return success();
    }

    if (p_offset + p_bytes > record->bytes)
    {
        return failure("writing " + std::to_string(p_bytes) +
                       " bytes at byte " + std::to_string(p_offset) +
                       " would run past the end of a buffer of " +
                       std::to_string(record->bytes) + " bytes");
    }

    if (p_data == nullptr)
    {
        return failure("writing a buffer needs data to write");
    }

    backend::writeBuffer(record->native, p_offset, p_bytes, p_data);
    return success();
}

//------------------------------------------------------------------------------
Status readBuffer(BufferHandle p_handle,
                  std::size_t p_offset,
                  std::size_t p_bytes,
                  void* p_data)
{
    BufferRecord* record = pools().buffers.get(p_handle);
    if (record == nullptr)
    {
        return failure(staleHandleMessage());
    }

    if (p_bytes == 0u)
    {
        return success();
    }

    if (p_offset + p_bytes > record->bytes)
    {
        return failure("reading " + std::to_string(p_bytes) +
                       " bytes at byte " + std::to_string(p_offset) +
                       " would run past the end of a buffer of " +
                       std::to_string(record->bytes) + " bytes");
    }

    if (p_data == nullptr)
    {
        return failure("reading a buffer needs somewhere to put the data");
    }

    backend::readBuffer(record->native, p_offset, p_bytes, p_data);
    return success();
}

//------------------------------------------------------------------------------
std::size_t bufferBytes(BufferHandle p_handle)
{
    BufferRecord const* record = pools().buffers.get(p_handle);
    return (record == nullptr) ? 0u : record->bytes;
}

//------------------------------------------------------------------------------
bool bufferAlive(BufferHandle p_handle)
{
    return pools().buffers.valid(p_handle);
}

//------------------------------------------------------------------------------
BufferKind bufferKind(BufferHandle p_handle)
{
    BufferRecord const* record = pools().buffers.get(p_handle);
    assert((record != nullptr) && "kind() of a buffer that no longer exists");
    return (record == nullptr) ? BufferKind::Vertex : record->kind;
}

//------------------------------------------------------------------------------
BufferUsage bufferUsage(BufferHandle p_handle)
{
    BufferRecord const* record = pools().buffers.get(p_handle);
    assert((record != nullptr) && "usage() of a buffer that no longer exists");
    return (record == nullptr) ? BufferUsage::Dynamic : record->usage;
}

//------------------------------------------------------------------------------
std::size_t liveBufferCount()
{
    return pools().buffers.size();
}

//------------------------------------------------------------------------------
std::size_t bufferMemory()
{
    std::size_t total = 0u;
    pools().buffers.forEach([&total](BufferHandle, BufferRecord const& p_record)
                            { total += p_record.bytes; });
    return total;
}

} // namespace compages::gpu::detail
