// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"
#include "GPU/Internal/GlslDialect.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Shaders, programs and their reflection, in OpenGL 4.1 / ES 3.0.
//!
//! Three differences with the GL45 backend:
//! - the source is translated from GLSL 4.50 first (GlslDialect.hpp), and the
//!   `layout(binding = N)` it removed are applied once the program links;
//! - reflection uses the queries of OpenGL 3.1 (glGetActiveUniform...): the
//!   program interface queries (glGetProgramResourceiv) are 4.3;
//! - a uniform is written to the program in use: glProgramUniform is not in
//!   OpenGL ES 3.0, so detail::useProgram() makes the program current first.
// ****************************************************************************

namespace compages::gpu::backend
{

namespace
{

#if defined(COMPAGES_GLES3)
constexpr gpu::detail::GlslDialect DIALECT = gpu::detail::GlslDialect::Es300;
constexpr char const* DIALECT_NAME = "GLSL ES 3.00";
#else
constexpr gpu::detail::GlslDialect DIALECT = gpu::detail::GlslDialect::Desktop410;
constexpr char const* DIALECT_NAME = "GLSL 4.10 core";
#endif

//! \brief The bindings each compiled shader had in its source, applied by
//! linkProgram() to the programs it goes into.
std::map<GLuint, std::vector<gpu::detail::FixedBinding>>& fixedBindings()
{
    static std::map<GLuint, std::vector<gpu::detail::FixedBinding>> instance;
    return instance;
}

//! \brief The stage, or 0 when this backend has none of that kind.
GLenum toGL(ShaderStage p_stage)
{
    switch (p_stage)
    {
        case ShaderStage::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderStage::Fragment:
            return GL_FRAGMENT_SHADER;
#if defined(COMPAGES_GLES3)
        case ShaderStage::Geometry:
        case ShaderStage::TessellationControl:
        case ShaderStage::TessellationEvaluation:
            return 0u;
#else
        case ShaderStage::Geometry:
            return GL_GEOMETRY_SHADER;
        case ShaderStage::TessellationControl:
            return GL_TESS_CONTROL_SHADER;
        case ShaderStage::TessellationEvaluation:
            return GL_TESS_EVALUATION_SHADER;
#endif
        case ShaderStage::Compute:
            return 0u;
    }
    return 0u;
}

DataType fromGL(GLenum p_type)
{
    switch (p_type)
    {
        case GL_FLOAT:
            return DataType::Float;
        case GL_FLOAT_VEC2:
            return DataType::Vec2;
        case GL_FLOAT_VEC3:
            return DataType::Vec3;
        case GL_FLOAT_VEC4:
            return DataType::Vec4;
        case GL_INT:
            return DataType::Int;
        case GL_INT_VEC2:
            return DataType::IVec2;
        case GL_INT_VEC3:
            return DataType::IVec3;
        case GL_INT_VEC4:
            return DataType::IVec4;
        case GL_UNSIGNED_INT:
            return DataType::UInt;
        case GL_UNSIGNED_INT_VEC2:
            return DataType::UVec2;
        case GL_UNSIGNED_INT_VEC3:
            return DataType::UVec3;
        case GL_UNSIGNED_INT_VEC4:
            return DataType::UVec4;
        case GL_BOOL:
            return DataType::Bool;
        case GL_BOOL_VEC2:
            return DataType::BVec2;
        case GL_BOOL_VEC3:
            return DataType::BVec3;
        case GL_BOOL_VEC4:
            return DataType::BVec4;
        case GL_FLOAT_MAT2:
            return DataType::Mat2;
        case GL_FLOAT_MAT3:
            return DataType::Mat3;
        case GL_FLOAT_MAT4:
            return DataType::Mat4;
        case GL_FLOAT_MAT2x3:
            return DataType::Mat2x3;
        case GL_FLOAT_MAT2x4:
            return DataType::Mat2x4;
        case GL_FLOAT_MAT3x2:
            return DataType::Mat3x2;
        case GL_FLOAT_MAT3x4:
            return DataType::Mat3x4;
        case GL_FLOAT_MAT4x2:
            return DataType::Mat4x2;
        case GL_FLOAT_MAT4x3:
            return DataType::Mat4x3;
        case GL_SAMPLER_2D:
            return DataType::Sampler2D;
        case GL_SAMPLER_3D:
            return DataType::Sampler3D;
        case GL_SAMPLER_CUBE:
            return DataType::SamplerCube;
        case GL_SAMPLER_2D_ARRAY:
            return DataType::Sampler2DArray;
        case GL_SAMPLER_2D_SHADOW:
            return DataType::Sampler2DShadow;
        case GL_SAMPLER_CUBE_SHADOW:
            return DataType::SamplerCubeShadow;
        case GL_INT_SAMPLER_2D:
            return DataType::ISampler2D;
        case GL_INT_SAMPLER_3D:
            return DataType::ISampler3D;
        case GL_UNSIGNED_INT_SAMPLER_2D:
            return DataType::USampler2D;
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return DataType::USampler3D;
#if !defined(COMPAGES_GLES3)
        case GL_DOUBLE:
            return DataType::Double;
        case GL_DOUBLE_VEC2:
            return DataType::DVec2;
        case GL_DOUBLE_VEC3:
            return DataType::DVec3;
        case GL_DOUBLE_VEC4:
            return DataType::DVec4;
        case GL_SAMPLER_1D:
            return DataType::Sampler1D;
        case GL_SAMPLER_1D_ARRAY:
            return DataType::Sampler1DArray;
        case GL_SAMPLER_CUBE_MAP_ARRAY:
            return DataType::SamplerCubeArray;
        case GL_SAMPLER_BUFFER:
            return DataType::SamplerBuffer;
#endif
        default:
            return DataType::Unknown;
    }
}

std::string withoutTrailingZeros(std::string p_text)
{
    while (!p_text.empty() && (p_text.back() == '\0'))
    {
        p_text.pop_back();
    }
    return p_text;
}

std::string shaderLog(GLuint p_shader)
{
    GLint length = 0;
    glGetShaderiv(p_shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1)
    {
        return {};
    }
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetShaderInfoLog(p_shader, length, nullptr, log.data());
    return withoutTrailingZeros(std::move(log));
}

std::string programLog(GLuint p_program)
{
    GLint length = 0;
    glGetProgramiv(p_program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1)
    {
        return {};
    }
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetProgramInfoLog(p_program, length, nullptr, log.data());
    return withoutTrailingZeros(std::move(log));
}

void stripArrayIndices(std::string& p_name)
{
    const std::string subscript = "[0]";
    for (std::size_t at = p_name.find(subscript); at != std::string::npos; at = p_name.find(subscript, at))
    {
        p_name.erase(at, subscript.size());
    }
}

bool isBuiltIn(std::string const& p_name)
{
    return p_name.compare(0u, 3u, "gl_") == 0;
}

void stripBlockPrefix(std::string& p_name, std::string const& p_block)
{
    const std::string prefix = p_block + ".";
    if (p_name.compare(0u, prefix.size(), prefix) == 0)
    {
        p_name.erase(0u, prefix.size());
    }
}

GLint programInt(GLuint p_program, GLenum p_name)
{
    GLint value = 0;
    glGetProgramiv(p_program, p_name, &value);
    return value;
}

//! \brief One active uniform: its name as the driver spells it, its size
//! (array elements) and its type.
struct ActiveUniform
{
    std::string name;
    GLint size = 0;
    GLenum type = 0u;
};

ActiveUniform activeUniform(GLuint p_program, GLuint p_index, GLint p_longest)
{
    ActiveUniform out;
    std::string name(static_cast<std::size_t>(std::max(p_longest, 1)), '\0');
    GLsizei length = 0;
    glGetActiveUniform(p_program, p_index, p_longest, &length, &out.size, &out.type, name.data());
    name.resize(static_cast<std::size_t>(length));
    out.name = std::move(name);
    return out;
}

GLint uniformProperty(GLuint p_program, GLuint p_index, GLenum p_property)
{
    GLint value = 0;
    glGetActiveUniformsiv(p_program, 1, &p_index, p_property, &value);
    return value;
}

void applyFixedBindings(GLuint p_program, std::span<const NativeId> p_shaders)
{
    for (NativeId shader : p_shaders)
    {
        auto found = fixedBindings().find(static_cast<GLuint>(shader));
        if (found == fixedBindings().end())
        {
            continue;
        }
        for (gpu::detail::FixedBinding const& binding : found->second)
        {
            if (binding.block)
            {
                const GLuint index = glGetUniformBlockIndex(p_program, binding.name.c_str());
                if (index != GL_INVALID_INDEX)
                {
                    glUniformBlockBinding(p_program, index, static_cast<GLuint>(binding.binding));
                }
                continue;
            }
            const GLint location = glGetUniformLocation(p_program, binding.name.c_str());
            if (location >= 0)
            {
                detail::useProgram(p_program);
                glUniform1i(location, binding.binding);
            }
        }
    }
}

} // namespace

namespace detail
{

GLuint compileGlsl(GLenum p_stage, std::string_view p_source, std::string& p_log)
{
    const GLuint shader = glCreateShader(p_stage);
    if (shader == 0u)
    {
        p_log = "the driver refused to create a shader";
        return 0u;
    }

    // A vertex or fragment shader reading storage blocks reads copies in
    // textures instead (GL41Compute.cpp).
    std::string rewritten;
    std::vector<gpu::detail::EmulatedBlock> reads;
    if (gpu::detail::declaresStorageBlock(p_source))
    {
#if defined(COMPAGES_GLES3)
        auto emulated = gpu::detail::emulateStorageReads(p_source, { "GL_ES" });
#else
        auto emulated = gpu::detail::emulateStorageReads(p_source, {});
#endif
        if (!emulated)
        {
            glDeleteShader(shader);
            p_log = emulated.error();
            return 0u;
        }
        rewritten = std::move(emulated.value().text);
        reads = std::move(emulated.value().blocks);
        p_source = rewritten;
    }

    gpu::detail::TranslatedGlsl translated = gpu::detail::translateGlsl(p_source, DIALECT);
    const GLchar* source = translated.text.data();
    const auto length = static_cast<GLint>(translated.text.size());
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE)
    {
        std::string log = shaderLog(shader);
        glDeleteShader(shader);
        if (log.empty())
        {
            log = "the shader did not compile and the driver said nothing about why";
        }
        // The reader wrote GLSL 4.50: say it was compiled as something else,
        // since that is the usual reason for a line that works elsewhere.
        if (translated.source_version > 0)
        {
            log += "\n(" + std::string(name()) + " compiles this #version " +
                   std::to_string(translated.source_version) + " source as " + DIALECT_NAME +
                   ": storage blocks read from textures, no image" +
#if defined(COMPAGES_GLES3)
                   "; no implicit int to float conversion, no geometry stage" +
#endif
                   ")";
        }
        p_log = std::move(log);
        return 0u;
    }

    if (!translated.bindings.empty())
    {
        fixedBindings()[shader] = std::move(translated.bindings);
    }
    if (!reads.empty())
    {
        detail::rememberStorageReads(shader, std::move(reads));
    }
    return shader;
}

} // namespace detail

Result<NativeId> compileShader(ShaderStage p_stage, std::string_view p_source)
{
    if (p_stage == ShaderStage::Compute)
    {
        // No compute shader here: it runs as a vertex shader whose outputs
        // are captured (GL41Compute.cpp).
        return detail::compileCompute(p_source);
    }
    const GLenum stage = toGL(p_stage);
    if (stage == 0u)
    {
        return failure(std::string(name()) + " has no " + toString(p_stage) +
                       " shaders: they need OpenGL 3.2 or OpenGL ES 3.2. Check DeviceInfo::geometry_shaders "
                       "before using one");
    }
    std::string log;
    const GLuint shader = detail::compileGlsl(stage, p_source, log);
    if (shader == 0u)
    {
        return failure(std::move(log));
    }
    return static_cast<NativeId>(shader);
}

void destroyShader(NativeId p_shader)
{
    fixedBindings().erase(static_cast<GLuint>(p_shader));
    if (detail::releaseCompute(static_cast<GLuint>(p_shader)))
    {
        return;
    }
    glDeleteShader(static_cast<GLuint>(p_shader));
}

Result<NativeId> linkProgram(std::span<const NativeId> p_shaders)
{
    const GLuint program = glCreateProgram();
    if (program == 0u)
    {
        return failure("the driver refused to create a program");
    }

    for (NativeId shader : p_shaders)
    {
        glAttachShader(program, static_cast<GLuint>(shader));
    }
    detail::prepareComputeLink(program, p_shaders);
    glLinkProgram(program);
    for (NativeId shader : p_shaders)
    {
        glDetachShader(program, static_cast<GLuint>(shader));
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    detail::finishStorageLink(program, p_shaders, linked != GL_FALSE);
    if (linked == GL_FALSE)
    {
        std::string log = programLog(program);
        glDeleteProgram(program);
        if (log.empty())
        {
            log = "the stages did not link and the driver said nothing about why";
        }
        return failure(std::move(log));
    }

    // Before the reflection, so that it reports the bindings of the source.
    applyFixedBindings(program, p_shaders);
    return static_cast<NativeId>(program);
}

void destroyProgram(NativeId p_program)
{
    const auto program = static_cast<GLuint>(p_program);
    // A deleted program in use stays alive until another one is used:
    // forget it, so that a new program given the same name is bound again.
    detail::useProgram(0u);
    detail::forgetStorageProgram(program);
    glDeleteProgram(program);
}

void reflectProgram(NativeId p_program, std::span<const ShaderStage> /* stages */, ProgramReflection& p_reflection)
{
    const auto program = static_cast<GLuint>(p_program);

    // The `in` variables of the vertex stage.
    const GLint attribute_count = programInt(program, GL_ACTIVE_ATTRIBUTES);
    const GLint attribute_longest = programInt(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
    p_reflection.attributes.reserve(static_cast<std::size_t>(attribute_count));
    for (GLint i = 0; i < attribute_count; ++i)
    {
        std::string raw(static_cast<std::size_t>(std::max(attribute_longest, 1)), '\0');
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0u;
        glGetActiveAttrib(program, static_cast<GLuint>(i), attribute_longest, &length, &size, &type, raw.data());
        raw.resize(static_cast<std::size_t>(length));
        if (isBuiltIn(raw))
        {
            continue;
        }
        AttributeInfo attribute;
        attribute.location = glGetAttribLocation(program, raw.c_str());
        attribute.name = raw;
        stripArrayIndices(attribute.name);
        attribute.type = fromGL(type);
        attribute.elements = size;
        attribute.format = attributeFormatOf(attribute.type);
        p_reflection.attributes.emplace_back(std::move(attribute));
    }

    // The uniforms outside any block (the others are reported with it).
    const GLint uniform_count = programInt(program, GL_ACTIVE_UNIFORMS);
    const GLint uniform_longest = programInt(program, GL_ACTIVE_UNIFORM_MAX_LENGTH);
    for (GLint i = 0; i < uniform_count; ++i)
    {
        const auto index = static_cast<GLuint>(i);
        if (uniformProperty(program, index, GL_UNIFORM_BLOCK_INDEX) != -1)
        {
            continue;
        }
        ActiveUniform active = activeUniform(program, index, uniform_longest);
        if (isBuiltIn(active.name))
        {
            continue;
        }
        UniformInfo uniform;
        uniform.location = glGetUniformLocation(program, active.name.c_str());
        uniform.name = std::move(active.name);
        stripArrayIndices(uniform.name);
        uniform.type = fromGL(active.type);
        uniform.elements = active.size;
        p_reflection.uniforms.emplace_back(std::move(uniform));
    }

    // The uniform blocks. No storage blocks and no work group on this backend.
    const GLint block_count = programInt(program, GL_ACTIVE_UNIFORM_BLOCKS);
    const GLint block_longest = programInt(program, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH);
    p_reflection.uniform_blocks.reserve(static_cast<std::size_t>(block_count));
    for (GLint b = 0; b < block_count; ++b)
    {
        const auto block_index = static_cast<GLuint>(b);
        BlockInfo block;
        std::string name(static_cast<std::size_t>(std::max(block_longest, 1)), '\0');
        GLsizei length = 0;
        glGetActiveUniformBlockName(program, block_index, block_longest, &length, name.data());
        name.resize(static_cast<std::size_t>(length));
        block.name = std::move(name);

        GLint value = 0;
        glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_BINDING, &value);
        block.binding = value;
        glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &value);
        block.bytes = static_cast<std::size_t>(value);
        GLint member_count = 0;
        glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &member_count);
        std::vector<GLint> members(static_cast<std::size_t>(member_count), 0);
        if (member_count > 0)
        {
            glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, members.data());
        }

        block.members.reserve(members.size());
        for (GLint member_index : members)
        {
            const auto index = static_cast<GLuint>(member_index);
            ActiveUniform active = activeUniform(program, index, uniform_longest);
            BlockMember member;
            member.name = std::move(active.name);
            stripArrayIndices(member.name);
            stripBlockPrefix(member.name, block.name);
            member.type = fromGL(active.type);
            member.elements = active.size;
            member.offset = static_cast<std::uint32_t>(uniformProperty(program, index, GL_UNIFORM_OFFSET));
            member.array_stride = static_cast<std::uint32_t>(uniformProperty(program, index, GL_UNIFORM_ARRAY_STRIDE));
            member.matrix_stride =
                static_cast<std::uint32_t>(uniformProperty(program, index, GL_UNIFORM_MATRIX_STRIDE));
            block.members.emplace_back(std::move(member));
        }
        p_reflection.uniform_blocks.emplace_back(std::move(block));
    }

    detail::reflectStorage(program, p_reflection);
}

void setUniform(NativeId p_program, int p_location, DataType p_type, const void* p_data)
{
    detail::useProgram(static_cast<GLuint>(p_program));
    const auto* floats = static_cast<const GLfloat*>(p_data);
    const auto* ints = static_cast<const GLint*>(p_data);
    const auto* uints = static_cast<const GLuint*>(p_data);

    switch (p_type)
    {
        case DataType::Float:
            glUniform1fv(p_location, 1, floats);
            break;
        case DataType::Vec2:
            glUniform2fv(p_location, 1, floats);
            break;
        case DataType::Vec3:
            glUniform3fv(p_location, 1, floats);
            break;
        case DataType::Vec4:
            glUniform4fv(p_location, 1, floats);
            break;
        case DataType::Int:
        case DataType::Bool:
            glUniform1iv(p_location, 1, ints);
            break;
        case DataType::IVec2:
        case DataType::BVec2:
            glUniform2iv(p_location, 1, ints);
            break;
        case DataType::IVec3:
        case DataType::BVec3:
            glUniform3iv(p_location, 1, ints);
            break;
        case DataType::IVec4:
        case DataType::BVec4:
            glUniform4iv(p_location, 1, ints);
            break;
        case DataType::UInt:
            glUniform1uiv(p_location, 1, uints);
            break;
        case DataType::UVec2:
            glUniform2uiv(p_location, 1, uints);
            break;
        case DataType::UVec3:
            glUniform3uiv(p_location, 1, uints);
            break;
        case DataType::UVec4:
            glUniform4uiv(p_location, 1, uints);
            break;
        // Row-major in C++, column-major in GLSL: the transpose flag (allowed
        // from OpenGL ES 3.0 on).
        case DataType::Mat2:
            glUniformMatrix2fv(p_location, 1, GL_TRUE, floats);
            break;
        case DataType::Mat3:
            glUniformMatrix3fv(p_location, 1, GL_TRUE, floats);
            break;
        case DataType::Mat4:
            glUniformMatrix4fv(p_location, 1, GL_TRUE, floats);
            break;
        // As in the GL45 backend: no set() writes these.
        case DataType::Double:
        case DataType::DVec2:
        case DataType::DVec3:
        case DataType::DVec4:
        case DataType::Mat2x3:
        case DataType::Mat2x4:
        case DataType::Mat3x2:
        case DataType::Mat3x4:
        case DataType::Mat4x2:
        case DataType::Mat4x3:
        case DataType::Sampler1D:
        case DataType::Sampler2D:
        case DataType::Sampler3D:
        case DataType::SamplerCube:
        case DataType::Sampler1DArray:
        case DataType::Sampler2DArray:
        case DataType::SamplerCubeArray:
        case DataType::Sampler2DShadow:
        case DataType::SamplerCubeShadow:
        case DataType::SamplerBuffer:
        case DataType::ISampler2D:
        case DataType::ISampler3D:
        case DataType::USampler2D:
        case DataType::USampler3D:
        case DataType::Image1D:
        case DataType::Image2D:
        case DataType::Image3D:
        case DataType::ImageCube:
        case DataType::Image2DArray:
        case DataType::IImage2D:
        case DataType::UImage2D:
        case DataType::Unknown:
            break;
    }
}

void setUniformArray(NativeId p_program, int p_location, DataType p_type, const void* p_data, int p_count)
{
    if ((p_count <= 0) || (p_type != DataType::Mat4))
    {
        // As in the GL45 backend: only `uniform mat4 name[N]` is written.
        return;
    }
    detail::useProgram(static_cast<GLuint>(p_program));
    glUniformMatrix4fv(p_location, p_count, GL_TRUE, static_cast<const GLfloat*>(p_data));
}

void bindUniformBlock(NativeId p_program, int p_block_index, int p_binding)
{
    glUniformBlockBinding(
        static_cast<GLuint>(p_program), static_cast<GLuint>(p_block_index), static_cast<GLuint>(p_binding));
}

} // namespace compages::gpu::backend
