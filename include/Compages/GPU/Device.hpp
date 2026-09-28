// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Core/Handle.hpp"

#include <array>
#include <string>
#include <string_view>

namespace compages::gpu
{

// ----------------------------------------------------------------------------
//! \brief Function resolving the address of a graphics driver symbol.
//!
//! The library never creates a window and never opens the driver itself: the
//! caller does, then hands over the loader its windowing library provides. With
//! GLFW that is glfwGetProcAddress, with SDL it is SDL_GL_GetProcAddress. This
//! is what keeps compages::gpu:: usable with any windowing system, and testable with an
//! invisible window.
// ----------------------------------------------------------------------------
using LoadProc = void* (*)(const char*);

// ----------------------------------------------------------------------------
//! \brief How serious a message from the library is.
// ----------------------------------------------------------------------------
enum class LogLevel
{
    //! \brief Something worth knowing, such as the name of the GPU in use.
    Info,
    //! \brief Something suspicious that did not stop the frame, such as the
    //! driver reporting slow behaviour.
    Warning,
    //! \brief Something went wrong, typically reported by the driver itself.
    Error,
};

// ----------------------------------------------------------------------------
//! \brief Where the library writes its messages.
//!
//! Replace it to route driver errors into the log of a host application. The
//! default writes to stderr, so that a mistake is visible without any setup.
// ----------------------------------------------------------------------------
using LogCallback = void (*)(LogLevel, std::string_view);

// ****************************************************************************
//! \brief What the driver is and what it allows.
//!
//! Everything a caller may need in order to decide what it can afford, without
//! having to ask the graphics API directly. All the limits are those the
//! library itself checks against, so a validation error can name both the value
//! asked for and the maximum allowed.
// ****************************************************************************
struct DeviceInfo
{
    //! \brief Who makes the driver, for instance "NVIDIA Corporation".
    std::string vendor;
    //! \brief Which GPU, for instance "NVIDIA GeForce RTX 3050".
    std::string renderer;
    //! \brief Version string as the driver spells it.
    std::string version;
    //! \brief Shading language version string as the driver spells it.
    std::string shading_language_version;

    //! \brief Major version actually granted, at least 4 for this backend.
    int version_major = 0;
    //! \brief Minor version actually granted, at least 5 when major is 4.
    int version_minor = 0;

    //! \brief How many vertex attributes a pipeline may declare. A vertex
    //! layout with more fields than this is rejected when the pipeline is
    //! created rather than silently dropping attributes.
    int max_vertex_attributes = 0;
    //! \brief Largest width or height a 2D texture may have.
    int max_texture_size = 0;
    //! \brief Largest depth a 3D texture may have.
    int max_texture_size_3d = 0;
    //! \brief How many textures a single draw may sample from at once.
    int max_texture_units = 0;
    //! \brief Largest uniform block, in bytes.
    int max_uniform_block_size = 0;
    //! \brief Alignment a uniform buffer binding offset must respect. Needed
    //! when several blocks share one buffer.
    int uniform_buffer_offset_alignment = 0;
    //! \brief Largest shader storage block, in bytes. This is what caps the
    //! number of particles a compute pass can hold in one buffer.
    int max_shader_storage_block_size = 0;

    //! \brief Largest number of work groups a dispatch may ask for, per axis.
    std::array<int, 3> max_compute_work_group_count{ 0, 0, 0 };
    //! \brief Largest size of a work group, per axis.
    std::array<int, 3> max_compute_work_group_size{ 0, 0, 0 };
    //! \brief Largest total number of invocations inside one work group.
    int max_compute_work_group_invocations = 0;

    //! \brief Is the driver able to report its own errors to us? When true the
    //! library relies on it instead of asking for an error code after every
    //! single call.
    bool debug_output = false;
};

// ----------------------------------------------------------------------------
//! \brief Start the library on the graphics context that is current.
//!
//! A context must already exist and be current on the calling thread: create a
//! window first, then call this.
//!
//! \param[in] p_load the symbol loader of the windowing library in use, for
//! instance glfwGetProcAddress.
//! \return success, or the reason the device cannot be used, typically a
//! graphics API too old for this backend.
// ----------------------------------------------------------------------------
[[nodiscard]] Status init(LoadProc p_load);

// ----------------------------------------------------------------------------
//! \brief Release everything the library still holds on the GPU.
//!
//! Must be called while the context is still current, otherwise the driver has
//! nothing left to free the resources on. Calling it twice is harmless.
// ----------------------------------------------------------------------------
void shutdown();

// ----------------------------------------------------------------------------
//! \brief Has init() succeeded and shutdown() not been called since?
// ----------------------------------------------------------------------------
[[nodiscard]] bool initialized();

// ----------------------------------------------------------------------------
//! \brief What the driver is and what it allows.
//! \note Asserts when the library is not initialized.
// ----------------------------------------------------------------------------
[[nodiscard]] DeviceInfo const& device();

// ----------------------------------------------------------------------------
//! \brief Choose where the library writes its messages.
//! \param[in] p_callback the new destination, or nullptr to go back to stderr.
// ----------------------------------------------------------------------------
void logger(LogCallback p_callback);

// ----------------------------------------------------------------------------
//! \brief Write a message through the current logger.
// ----------------------------------------------------------------------------
void log(LogLevel p_level, std::string_view p_message);

// ----------------------------------------------------------------------------
//! \brief Pass on, or not, what the driver says about speed rather than about
//! mistakes: "the vertex shader is recompiled based on the state", "this
//! buffer moved to host memory", "a partial clear fell back from CSAA to
//! MSAA". Off by default: they repeat, they are not errors, and they are
//! only worth reading while hunting a slowdown.
// ----------------------------------------------------------------------------
void reportDriverHints(bool p_report);

// ----------------------------------------------------------------------------
//! \brief Is reportDriverHints() on?
// ----------------------------------------------------------------------------
[[nodiscard]] bool driverHintsReported();

} // namespace compages::gpu
