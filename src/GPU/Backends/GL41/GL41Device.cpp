// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <string>

namespace compages::gpu::backend
{

namespace
{

std::string queryString(GLenum p_name)
{
    const GLubyte* value = glGetString(p_name);
    if (value == nullptr)
    {
        return "unknown";
    }
    return reinterpret_cast<const char*>(value);
}

int queryInt(GLenum p_name)
{
    GLint value = 0;
    glGetIntegerv(p_name, &value);
    return static_cast<int>(value);
}

#if !defined(COMPAGES_GLES3)

// The driver messages (KHR_debug) are core in 4.3: macOS never has them,
// but a newer driver giving a 4.1 context often does (Mesa), and then they
// are worth having.
void GLAD_API_PTR onDriverMessage(GLenum /* source */,
                                  GLenum p_type,
                                  GLuint p_id,
                                  GLenum p_severity,
                                  GLsizei /* length */,
                                  const GLchar* p_message,
                                  const void* /* user */)
{
    if (p_severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }
    const bool hint = (p_type == GL_DEBUG_TYPE_PERFORMANCE) ||
                      ((p_type == GL_DEBUG_TYPE_OTHER) && (p_severity == GL_DEBUG_SEVERITY_LOW));
    if (hint && !driverHintsReported())
    {
        return;
    }
    const bool error = (p_type == GL_DEBUG_TYPE_ERROR) || (p_type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
    log(error ? LogLevel::Error : (hint ? LogLevel::Info : LogLevel::Warning),
        "driver #" + std::to_string(p_id) + ": " + (p_message != nullptr ? p_message : "(no message)"));
}

bool enableDriverMessages()
{
    if (glDebugMessageCallback == nullptr)
    {
        return false;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&onDriverMessage, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    return true;
}

#endif

void queryLimits(DeviceInfo& p_info)
{
    p_info.vendor = queryString(GL_VENDOR);
    p_info.renderer = queryString(GL_RENDERER);
    p_info.version = queryString(GL_VERSION);
    p_info.shading_language_version = queryString(GL_SHADING_LANGUAGE_VERSION);

    p_info.max_vertex_attributes = queryInt(GL_MAX_VERTEX_ATTRIBS);
    p_info.max_texture_size = queryInt(GL_MAX_TEXTURE_SIZE);
    p_info.max_texture_size_3d = queryInt(GL_MAX_3D_TEXTURE_SIZE);
    p_info.max_texture_units = queryInt(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    p_info.max_uniform_block_size = queryInt(GL_MAX_UNIFORM_BLOCK_SIZE);
    p_info.uniform_buffer_offset_alignment = queryInt(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    // No storage block and no compute: the limits stay at zero.
}

} // namespace

Status init([[maybe_unused]] LoadProc p_load, DeviceInfo& p_info)
{
#if defined(COMPAGES_GLES3)
    // The symbols are linked (libGLESv2, or the WebGL functions of
    // Emscripten): nothing to load, but a context must be current.
    if (glGetString(GL_VERSION) == nullptr)
    {
        return failure("no OpenGL ES context is current on this thread. compages::gpu::init() must be "
                       "called after the window (or the WebGL canvas) has been created and made current");
    }
    p_info.version_major = queryInt(GL_MAJOR_VERSION);
    p_info.version_minor = queryInt(GL_MINOR_VERSION);
#else
    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(p_load));
    if (version == 0)
    {
        return failure("the OpenGL symbols could not be loaded. Is a context current on this thread? "
                       "compages::gpu::init() must be called after the window has been created and made "
                       "current");
    }
    p_info.version_major = GLAD_VERSION_MAJOR(version);
    p_info.version_minor = GLAD_VERSION_MINOR(version);
#endif

    const bool too_old = (p_info.version_major < MINIMUM_VERSION_MAJOR) ||
                         ((p_info.version_major == MINIMUM_VERSION_MAJOR) &&
                          (p_info.version_minor < MINIMUM_VERSION_MINOR));
    if (too_old)
    {
        return failure(std::string("this backend (") + name() + ") needs version " +
                       std::to_string(MINIMUM_VERSION_MAJOR) + "." + std::to_string(MINIMUM_VERSION_MINOR) +
                       " but the driver granted " + std::to_string(p_info.version_major) + "." +
                       std::to_string(p_info.version_minor));
    }

#if defined(COMPAGES_GLES3)
    p_info.debug_output = false;
    p_info.geometry_shaders = false;
    p_info.polygon_mode = false;
#else
    p_info.debug_output = enableDriverMessages();
    p_info.geometry_shaders = true;
    p_info.polygon_mode = true;
#endif
    p_info.clip_control = false;
    queryLimits(p_info);

    // Compute is emulated (GL41Compute.cpp): a storage buffer is copied into
    // a texture of 32 bit words, as wide as below and as tall as the device
    // allows; that is the largest buffer a dispatch can read.
    const GLsizei width = std::min(4096, std::max(64, p_info.max_texture_size));
    detail::setDataTextureWidth(width);
    p_info.compute = true;
    p_info.compute_emulated = true;
    p_info.images = false;
    p_info.max_shader_storage_block_size =
        static_cast<int>(std::min<long long>(std::numeric_limits<int>::max(),
                                             4LL * width * std::max(1, p_info.max_texture_size)));
    // One point per invocation: the counts a draw accepts.
    p_info.max_compute_work_group_count = { 65535, 65535, 65535 };
    p_info.max_compute_work_group_size = { 1024, 1024, 64 };
    p_info.max_compute_work_group_invocations = 1024;
    return success();
}

namespace
{

std::array<GLuint, TIMESTAMP_SLOTS> g_timestamps{};
bool g_timestamps_made = false;

} // namespace

#if defined(COMPAGES_GLES3)

// OpenGL ES 3.0 has no timer query (an extension of WebGL 2 has one, not
// always granted): the GPU times read as never ready.
void writeTimestamp(std::uint32_t /* slot */) {}

bool readTimestamp(std::uint32_t /* slot */, std::uint64_t& /* nanoseconds */)
{
    return false;
}

#else

void writeTimestamp(std::uint32_t p_slot)
{
    if (!g_timestamps_made)
    {
        glGenQueries(GLsizei(TIMESTAMP_SLOTS), g_timestamps.data());
        g_timestamps_made = true;
    }
    glQueryCounter(g_timestamps[p_slot % TIMESTAMP_SLOTS], GL_TIMESTAMP);
}

bool readTimestamp(std::uint32_t p_slot, std::uint64_t& p_nanoseconds)
{
    if (!g_timestamps_made)
    {
        return false;
    }
    const GLuint query = g_timestamps[p_slot % TIMESTAMP_SLOTS];
    GLint ready = 0;
    glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &ready);
    if (ready == 0)
    {
        return false;
    }
    GLuint64 value = 0u;
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &value);
    p_nanoseconds = std::uint64_t(value);
    return true;
}

#endif

namespace detail
{

void warnOnce(char const* p_what)
{
    static std::set<std::string> said;
    if (said.insert(p_what).second)
    {
        log(LogLevel::Warning, std::string(name()) + ": " + p_what);
    }
}

} // namespace detail

void shutdown()
{
    detail::releaseTextureReadback();
    detail::releaseStorageCopies();
    detail::clearVertexArrayCache();
    if (g_timestamps_made)
    {
        glDeleteQueries(GLsizei(TIMESTAMP_SLOTS), g_timestamps.data());
        g_timestamps = {};
        g_timestamps_made = false;
    }
#if !defined(COMPAGES_GLES3)
    if (glDebugMessageCallback != nullptr)
    {
        glDebugMessageCallback(nullptr, nullptr);
        glDisable(GL_DEBUG_OUTPUT);
    }
    gladLoaderUnloadGL();
#endif
}

const char* name()
{
#if defined(COMPAGES_GLES3)
    return "OpenGL ES 3.0";
#else
    return "OpenGL 4.1 core";
#endif
}

} // namespace compages::gpu::backend
