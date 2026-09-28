// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL45/GL45.hpp"

#include <string>

namespace compages::gpu::backend
{

namespace
{

//! \brief Read a driver string, tolerating a driver that returns nothing.
std::string queryString(GLenum p_name)
{
    const GLubyte* value = glGetString(p_name);
    if (value == nullptr)
    {
        return "unknown";
    }
    return reinterpret_cast<const char*>(value);
}

//! \brief Read one integer limit.
int queryInt(GLenum p_name)
{
    GLint value = 0;
    glGetIntegerv(p_name, &value);
    return static_cast<int>(value);
}

//! \brief Read one axis of an indexed integer limit, as the compute limits are.
int queryIndexedInt(GLenum p_name, GLuint p_index)
{
    GLint value = 0;
    glGetIntegeri_v(p_name, p_index, &value);
    return static_cast<int>(value);
}

//! \brief Turn a driver message source into words.
const char* sourceName(GLenum p_source)
{
    switch (p_source)
    {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "window system";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "shader compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "third party";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "application";
        default:
            return "other";
    }
}

//! \brief Turn a driver message type into words.
const char* typeName(GLenum p_type)
{
    switch (p_type)
    {
        case GL_DEBUG_TYPE_ERROR:
            return "error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "deprecated behaviour";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "undefined behaviour";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "portability";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "performance";
        case GL_DEBUG_TYPE_MARKER:
            return "marker";
        default:
            return "message";
    }
}

//! \brief Called by the driver whenever it has something to say.
//!
//! This one function replaces the old habit of asking glGetError() after every
//! single OpenGL call. It is a strictly better deal: the driver tells us what
//! went wrong in a sentence, rather than us learning that something, somewhere,
//! returned GL_INVALID_OPERATION. Under a debug context the message even arrives
//! while the offending call is still on the stack, so a breakpoint here shows
//! the culprit.
void GLAD_API_PTR onDriverMessage(GLenum p_source,
                                  GLenum p_type,
                                  GLuint p_id,
                                  GLenum p_severity,
                                  GLsizei /* length */,
                                  const GLchar* p_message,
                                  const void* /* user */)
{
    // Reported by drivers for things such as "buffer will use video memory".
    // Useful when hunting a performance problem, noise the rest of the time.
    if (p_severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    // "Shader recompiled based on state", "buffer moved to host memory", and
    // remarks of low severity such as "a clear of part of the target made the
    // driver fall back from one antialiasing method to another": what to read
    // when hunting a slowdown, not a mistake in the frame.
    const bool hint = (p_type == GL_DEBUG_TYPE_PERFORMANCE) ||
                      ((p_type == GL_DEBUG_TYPE_OTHER) &&
                       (p_severity == GL_DEBUG_SEVERITY_LOW));
    if (hint && !driverHintsReported())
    {
        return;
    }

    LogLevel level = LogLevel::Warning;
    if ((p_type == GL_DEBUG_TYPE_ERROR) ||
        (p_type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR))
    {
        level = LogLevel::Error;
    }
    else if (hint)
    {
        level = LogLevel::Info;
    }

    char const* severity = (p_severity == GL_DEBUG_SEVERITY_HIGH)     ? "high"
                           : (p_severity == GL_DEBUG_SEVERITY_MEDIUM) ? "medium"
                                                                      : "low";
    std::string text = std::string(sourceName(p_source)) + " " +
                       typeName(p_type) + " #" + std::to_string(p_id) + " (" +
                       severity + "): " +
                       (p_message != nullptr ? p_message : "(no message)");
    log(level, text);
}

//! \brief Ask the driver to report its own errors, if it can.
//! \return true when the driver accepted, false when we are on our own.
bool enableDriverMessages()
{
    // Present since 4.3 as core, but a driver only honours it when the context
    // was created with the debug flag, which is why this is a request and not an
    // assumption.
    if (glDebugMessageCallback == nullptr)
    {
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT);
    // Without this the driver is free to report asynchronously, long after the
    // guilty call has returned, which loses the one thing that makes the
    // messages worth having: the stack that produced them.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&onDriverMessage, nullptr);
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    return true;
}

//! \brief Fill the limits the library validates against.
void queryLimits(DeviceInfo& p_info)
{
    p_info.vendor = queryString(GL_VENDOR);
    p_info.renderer = queryString(GL_RENDERER);
    p_info.version = queryString(GL_VERSION);
    p_info.shading_language_version =
        queryString(GL_SHADING_LANGUAGE_VERSION);

    p_info.max_vertex_attributes = queryInt(GL_MAX_VERTEX_ATTRIBS);
    p_info.max_texture_size = queryInt(GL_MAX_TEXTURE_SIZE);
    p_info.max_texture_size_3d = queryInt(GL_MAX_3D_TEXTURE_SIZE);
    p_info.max_texture_units =
        queryInt(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    p_info.max_uniform_block_size = queryInt(GL_MAX_UNIFORM_BLOCK_SIZE);
    p_info.uniform_buffer_offset_alignment =
        queryInt(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    p_info.max_shader_storage_block_size =
        queryInt(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);

    for (GLuint axis = 0u; axis < 3u; ++axis)
    {
        p_info.max_compute_work_group_count[axis] =
            queryIndexedInt(GL_MAX_COMPUTE_WORK_GROUP_COUNT, axis);
        p_info.max_compute_work_group_size[axis] =
            queryIndexedInt(GL_MAX_COMPUTE_WORK_GROUP_SIZE, axis);
    }
    p_info.max_compute_work_group_invocations =
        queryInt(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
}

} // namespace

Status init(LoadProc p_load, DeviceInfo& p_info)
{
    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(p_load));
    if (version == 0)
    {
        return failure(
            "the OpenGL symbols could not be loaded. Is a context current on "
            "this thread? compages::gpu::init() must be called after the window has been "
            "created and made current");
    }

    p_info.version_major = GLAD_VERSION_MAJOR(version);
    p_info.version_minor = GLAD_VERSION_MINOR(version);

    const bool too_old =
        (p_info.version_major < MINIMUM_VERSION_MAJOR) ||
        ((p_info.version_major == MINIMUM_VERSION_MAJOR) &&
         (p_info.version_minor < MINIMUM_VERSION_MINOR));
    if (too_old)
    {
        return failure(
            "this backend needs OpenGL " +
            std::to_string(MINIMUM_VERSION_MAJOR) + "." +
            std::to_string(MINIMUM_VERSION_MINOR) +
            " for Direct State Access but the driver granted " +
            std::to_string(p_info.version_major) + "." +
            std::to_string(p_info.version_minor) +
            ". Ask the window for a " +
            std::to_string(MINIMUM_VERSION_MAJOR) + "." +
            std::to_string(MINIMUM_VERSION_MINOR) +
            " core profile context, or run on a machine whose driver supports "
            "it. macOS never went past 4.1 and needs a different backend");
    }

    p_info.debug_output = enableDriverMessages();
    queryLimits(p_info);

    return success();
}

void shutdown()
{
    detail::clearVertexArrayCache();

    if (glDebugMessageCallback != nullptr)
    {
        glDebugMessageCallback(nullptr, nullptr);
        glDisable(GL_DEBUG_OUTPUT);
    }
    gladLoaderUnloadGL();
}

const char* name()
{
    return "OpenGL 4.5 core";
}

} // namespace compages::gpu::backend
