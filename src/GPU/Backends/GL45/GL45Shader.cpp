// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL45/GL45.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace compages::gpu::backend
{

namespace
{

GLenum toGL(ShaderStage p_stage)
{
    switch (p_stage)
    {
        case ShaderStage::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderStage::Fragment:
            return GL_FRAGMENT_SHADER;
        case ShaderStage::Geometry:
            return GL_GEOMETRY_SHADER;
        case ShaderStage::TessellationControl:
            return GL_TESS_CONTROL_SHADER;
        case ShaderStage::TessellationEvaluation:
            return GL_TESS_EVALUATION_SHADER;
        case ShaderStage::Compute:
            return GL_COMPUTE_SHADER;
    }
    return GL_VERTEX_SHADER;
}

//! \brief What the driver calls the type of a variable, in our own vocabulary.
//!
//! Anything not listed becomes Unknown rather than an error: a shader using an
//! exotic type should still be usable for everything else it declares.
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

        case GL_DOUBLE:
            return DataType::Double;
        case GL_DOUBLE_VEC2:
            return DataType::DVec2;
        case GL_DOUBLE_VEC3:
            return DataType::DVec3;
        case GL_DOUBLE_VEC4:
            return DataType::DVec4;

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

        case GL_SAMPLER_1D:
            return DataType::Sampler1D;
        case GL_SAMPLER_2D:
            return DataType::Sampler2D;
        case GL_SAMPLER_3D:
            return DataType::Sampler3D;
        case GL_SAMPLER_CUBE:
            return DataType::SamplerCube;
        case GL_SAMPLER_1D_ARRAY:
            return DataType::Sampler1DArray;
        case GL_SAMPLER_2D_ARRAY:
            return DataType::Sampler2DArray;
        case GL_SAMPLER_CUBE_MAP_ARRAY:
            return DataType::SamplerCubeArray;
        case GL_SAMPLER_2D_SHADOW:
            return DataType::Sampler2DShadow;
        case GL_SAMPLER_CUBE_SHADOW:
            return DataType::SamplerCubeShadow;
        case GL_SAMPLER_BUFFER:
            return DataType::SamplerBuffer;
        case GL_INT_SAMPLER_2D:
            return DataType::ISampler2D;
        case GL_INT_SAMPLER_3D:
            return DataType::ISampler3D;
        case GL_UNSIGNED_INT_SAMPLER_2D:
            return DataType::USampler2D;
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return DataType::USampler3D;

        case GL_IMAGE_1D:
            return DataType::Image1D;
        case GL_IMAGE_2D:
            return DataType::Image2D;
        case GL_IMAGE_3D:
            return DataType::Image3D;
        case GL_IMAGE_CUBE:
            return DataType::ImageCube;
        case GL_IMAGE_2D_ARRAY:
            return DataType::Image2DArray;
        case GL_INT_IMAGE_2D:
            return DataType::IImage2D;
        case GL_UNSIGNED_INT_IMAGE_2D:
            return DataType::UImage2D;

        default:
            return DataType::Unknown;
    }
}

//! \brief The compiler log of one stage, or an empty string when the driver
//! offers nothing, which some do on success.
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
    // The driver counts its terminating zero in the length it reported.
    while (!log.empty() && (log.back() == '\0'))
    {
        log.pop_back();
    }
    return log;
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
    while (!log.empty() && (log.back() == '\0'))
    {
        log.pop_back();
    }
    return log;
}

//! \brief The name of one resource of a program.
std::string resourceName(GLuint p_program, GLenum p_interface, GLuint p_index)
{
    GLint length = 0;
    const GLenum property = GL_NAME_LENGTH;
    glGetProgramResourceiv(
        p_program, p_interface, p_index, 1, &property, 1, nullptr, &length);
    if (length <= 1)
    {
        return {};
    }
    std::string name(static_cast<std::size_t>(length), '\0');
    glGetProgramResourceName(
        p_program, p_interface, p_index, length, nullptr, name.data());
    while (!name.empty() && (name.back() == '\0'))
    {
        name.pop_back();
    }
    return name;
}

//! \brief Ask for several properties of one resource in a single call.
void resourceProperties(GLuint p_program,
                        GLenum p_interface,
                        GLuint p_index,
                        std::span<const GLenum> p_properties,
                        std::span<GLint> p_values)
{
    glGetProgramResourceiv(p_program,
                           p_interface,
                           p_index,
                           static_cast<GLsizei>(p_properties.size()),
                           p_properties.data(),
                           static_cast<GLsizei>(p_values.size()),
                           nullptr,
                           p_values.data());
}

//! \brief How many things a program declares in one of its interfaces.
GLint resourceCount(GLuint p_program, GLenum p_interface)
{
    GLint count = 0;
    glGetProgramInterfaceiv(
        p_program, p_interface, GL_ACTIVE_RESOURCES, &count);
    return count;
}

//! \brief Turn the name the driver gives an array element back into the name of
//! the array.
//!
//! The driver reports an array under the name of its first element: "lights[0]"
//! for an array of floats, and "stars[0].position" for a member of an array of
//! structs. Callers look for "lights" and "stars.position", so every subscript
//! goes. Only element zero is ever reported, so nothing else can be lost.
void stripArrayIndices(std::string& p_name)
{
    const std::string subscript = "[0]";
    for (std::size_t at = p_name.find(subscript); at != std::string::npos;
         at = p_name.find(subscript, at))
    {
        p_name.erase(at, subscript.size());
    }
}

//! \brief Is this something the driver provides rather than the caller?
//!
//! A compute shader reading gl_GlobalInvocationID has it reported as one of its
//! inputs, at location -1. Leaving it in the list would mean a vertex layout is
//! asked to supply it, so the built-ins are left out: what remains is exactly
//! what the caller is responsible for.
bool isBuiltIn(std::string const& p_name)
{
    return p_name.compare(0u, 3u, "gl_") == 0;
}

//! \brief Drop the name of the block from the name of one of its members.
//!
//! A block declared with an instance name has its members reported as
//! "Matrices.model". Callers ask the block for "model", so the prefix goes, and
//! only that prefix: a member which is itself a struct keeps its own dots.
void stripBlockPrefix(std::string& p_name, std::string const& p_block)
{
    const std::string prefix = p_block + ".";
    if (p_name.compare(0u, prefix.size(), prefix) == 0)
    {
        p_name.erase(0u, prefix.size());
    }
}

//! \brief Read the members of one block, whether uniform or storage.
//!
//! \param[in] p_variable_interface GL_UNIFORM for a uniform block,
//! GL_BUFFER_VARIABLE for a storage block: the same questions, asked of two
//! different lists.
void readBlockMembers(GLuint p_program,
                      GLenum p_block_interface,
                      GLenum p_variable_interface,
                      GLuint p_block_index,
                      BlockInfo& p_block)
{
    constexpr std::array<GLenum, 3u> block_properties{
        GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES
    };
    std::array<GLint, 3u> block_values{ 0, 0, 0 };
    resourceProperties(p_program,
                       p_block_interface,
                       p_block_index,
                       block_properties,
                       block_values);

    p_block.binding = block_values[0];
    p_block.bytes = static_cast<std::size_t>(block_values[1]);

    const std::size_t member_count = static_cast<std::size_t>(block_values[2]);
    if (member_count == 0u)
    {
        return;
    }

    std::vector<GLint> member_indices(member_count, 0);
    const GLenum active_variables = GL_ACTIVE_VARIABLES;
    glGetProgramResourceiv(p_program,
                           p_block_interface,
                           p_block_index,
                           1,
                           &active_variables,
                           static_cast<GLsizei>(member_count),
                           nullptr,
                           member_indices.data());

    constexpr std::array<GLenum, 5u> member_properties{
        GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_ARRAY_STRIDE, GL_MATRIX_STRIDE
    };
    p_block.members.reserve(member_count);
    for (GLint index : member_indices)
    {
        std::array<GLint, 5u> values{ 0, 0, 0, 0, 0 };
        resourceProperties(p_program,
                           p_variable_interface,
                           static_cast<GLuint>(index),
                           member_properties,
                           values);

        BlockMember member;
        member.name = resourceName(
            p_program, p_variable_interface, static_cast<GLuint>(index));
        stripArrayIndices(member.name);
        stripBlockPrefix(member.name, p_block.name);
        member.type = fromGL(static_cast<GLenum>(values[0]));
        member.elements = values[1];
        member.offset = static_cast<std::uint32_t>(values[2]);
        member.array_stride = static_cast<std::uint32_t>(values[3]);
        member.matrix_stride = static_cast<std::uint32_t>(values[4]);
        p_block.members.emplace_back(std::move(member));
    }
}

} // namespace

Result<NativeId> compileShader(ShaderStage p_stage, std::string_view p_source)
{
    const GLuint shader = glCreateShader(toGL(p_stage));
    if (shader == 0u)
    {
        return failure("the driver refused to create a shader");
    }

    // The length is given explicitly, so the source need not be null terminated
    // and a std::string_view over part of a larger file works.
    const GLchar* source = p_source.data();
    const GLint length = static_cast<GLint>(p_source.size());
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
            log =
                "the shader did not compile and the driver said nothing about "
                "why";
        }
        return failure(std::move(log));
    }

    return static_cast<NativeId>(shader);
}

void destroyShader(NativeId p_shader)
{
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
    glLinkProgram(program);

    // Detaching lets the driver free what only the link needed, and lets a
    // Shader be released while the program keeps working.
    for (NativeId shader : p_shaders)
    {
        glDetachShader(program, static_cast<GLuint>(shader));
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE)
    {
        std::string log = programLog(program);
        glDeleteProgram(program);
        if (log.empty())
        {
            log =
                "the stages did not link and the driver said nothing about why";
        }
        return failure(std::move(log));
    }

    return static_cast<NativeId>(program);
}

void destroyProgram(NativeId p_program)
{
    glDeleteProgram(static_cast<GLuint>(p_program));
}

// Everything here goes through the one interface query introduced in
// OpenGL 4.3, glGetProgramResource*, rather than the older glGetActiveAttrib,
// glGetActiveUniform and glGetActiveUniformBlockiv. The same three calls then
// answer for attributes, uniforms, uniform blocks and storage blocks, and the
// older family has no way at all to describe a storage block.
void reflectProgram(NativeId p_program,
                    std::span<const ShaderStage> p_stages,
                    ProgramReflection& p_reflection)
{
    const GLuint program = static_cast<GLuint>(p_program);

    // The `in` variables of the vertex stage.
    constexpr std::array<GLenum, 3u> input_properties{ GL_TYPE,
                                                       GL_ARRAY_SIZE,
                                                       GL_LOCATION };
    const GLint input_count = resourceCount(program, GL_PROGRAM_INPUT);
    p_reflection.attributes.reserve(static_cast<std::size_t>(input_count));
    for (GLint i = 0; i < input_count; ++i)
    {
        std::array<GLint, 3u> values{ 0, 0, 0 };
        resourceProperties(program,
                           GL_PROGRAM_INPUT,
                           static_cast<GLuint>(i),
                           input_properties,
                           values);

        AttributeInfo attribute;
        attribute.name =
            resourceName(program, GL_PROGRAM_INPUT, static_cast<GLuint>(i));
        if (isBuiltIn(attribute.name))
        {
            continue;
        }
        stripArrayIndices(attribute.name);
        attribute.type = fromGL(static_cast<GLenum>(values[0]));
        attribute.elements = values[1];
        attribute.location = values[2];
        attribute.format = attributeFormatOf(attribute.type);
        p_reflection.attributes.emplace_back(std::move(attribute));
    }

    // The uniforms. GL_BLOCK_INDEX tells the ones living in a block from the
    // ones the driver keeps a copy of itself, and only the latter have a
    // location.
    constexpr std::array<GLenum, 4u> uniform_properties{
        GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION, GL_BLOCK_INDEX
    };
    const GLint uniform_count = resourceCount(program, GL_UNIFORM);
    for (GLint i = 0; i < uniform_count; ++i)
    {
        std::array<GLint, 4u> values{ 0, 0, 0, 0 };
        resourceProperties(program,
                           GL_UNIFORM,
                           static_cast<GLuint>(i),
                           uniform_properties,
                           values);

        if (values[3] != -1)
        {
            // It belongs to a block, and is reported with that block instead.
            continue;
        }

        UniformInfo uniform;
        uniform.name =
            resourceName(program, GL_UNIFORM, static_cast<GLuint>(i));
        if (isBuiltIn(uniform.name))
        {
            continue;
        }
        stripArrayIndices(uniform.name);
        uniform.type = fromGL(static_cast<GLenum>(values[0]));
        uniform.elements = values[1];
        uniform.location = values[2];
        p_reflection.uniforms.emplace_back(std::move(uniform));
    }

    // The uniform blocks, and then the storage blocks, which differ only in
    // which list their members are found in.
    const GLint uniform_block_count = resourceCount(program, GL_UNIFORM_BLOCK);
    p_reflection.uniform_blocks.reserve(
        static_cast<std::size_t>(uniform_block_count));
    for (GLint i = 0; i < uniform_block_count; ++i)
    {
        BlockInfo block;
        block.name =
            resourceName(program, GL_UNIFORM_BLOCK, static_cast<GLuint>(i));
        readBlockMembers(program,
                         GL_UNIFORM_BLOCK,
                         GL_UNIFORM,
                         static_cast<GLuint>(i),
                         block);
        p_reflection.uniform_blocks.emplace_back(std::move(block));
    }

    const GLint storage_block_count =
        resourceCount(program, GL_SHADER_STORAGE_BLOCK);
    p_reflection.storage_blocks.reserve(
        static_cast<std::size_t>(storage_block_count));
    for (GLint i = 0; i < storage_block_count; ++i)
    {
        BlockInfo block;
        block.name = resourceName(
            program, GL_SHADER_STORAGE_BLOCK, static_cast<GLuint>(i));
        readBlockMembers(program,
                         GL_SHADER_STORAGE_BLOCK,
                         GL_BUFFER_VARIABLE,
                         static_cast<GLuint>(i),
                         block);
        p_reflection.storage_blocks.emplace_back(std::move(block));
    }

    // Asking a program without a compute stage for its work group size is an
    // error the driver would report, hence the check on the stages rather than
    // on the answer.
    const bool has_compute =
        std::find(p_stages.begin(), p_stages.end(), ShaderStage::Compute) !=
        p_stages.end();
    if (has_compute)
    {
        std::array<GLint, 3u> work_group{ 0, 0, 0 };
        glGetProgramiv(program, GL_COMPUTE_WORK_GROUP_SIZE, work_group.data());
        p_reflection.work_group_size = { work_group[0],
                                         work_group[1],
                                         work_group[2] };
    }
}

// glProgramUniform rather than glUniform: it names the program it writes to, so
// nothing has to be made current first. The older glUniform writes to whatever
// program happens to be bound, which is a state the caller then has to remember
// to set, and a bug when they forget.
void setUniform(NativeId p_program,
                int p_location,
                DataType p_type,
                const void* p_data)
{
    const GLuint program = static_cast<GLuint>(p_program);
    const auto* floats = static_cast<const GLfloat*>(p_data);
    const auto* ints = static_cast<const GLint*>(p_data);
    const auto* uints = static_cast<const GLuint*>(p_data);

    switch (p_type)
    {
        case DataType::Float:
            glProgramUniform1fv(program, p_location, 1, floats);
            break;
        case DataType::Vec2:
            glProgramUniform2fv(program, p_location, 1, floats);
            break;
        case DataType::Vec3:
            glProgramUniform3fv(program, p_location, 1, floats);
            break;
        case DataType::Vec4:
            glProgramUniform4fv(program, p_location, 1, floats);
            break;

        case DataType::Int:
        case DataType::Bool:
            glProgramUniform1iv(program, p_location, 1, ints);
            break;
        case DataType::IVec2:
        case DataType::BVec2:
            glProgramUniform2iv(program, p_location, 1, ints);
            break;
        case DataType::IVec3:
        case DataType::BVec3:
            glProgramUniform3iv(program, p_location, 1, ints);
            break;
        case DataType::IVec4:
        case DataType::BVec4:
            glProgramUniform4iv(program, p_location, 1, ints);
            break;

        case DataType::UInt:
            glProgramUniform1uiv(program, p_location, 1, uints);
            break;
        case DataType::UVec2:
            glProgramUniform2uiv(program, p_location, 1, uints);
            break;
        case DataType::UVec3:
            glProgramUniform3uiv(program, p_location, 1, uints);
            break;
        case DataType::UVec4:
            glProgramUniform4uiv(program, p_location, 1, uints);
            break;

        // Row-major Scilab layout in C++; GL column-major via transpose flag.
        case DataType::Mat2:
            glProgramUniformMatrix2fv(program, p_location, 1, GL_TRUE, floats);
            break;
        case DataType::Mat3:
            glProgramUniformMatrix3fv(program, p_location, 1, GL_TRUE, floats);
            break;
        case DataType::Mat4:
            glProgramUniformMatrix4fv(program, p_location, 1, GL_TRUE, floats);
            break;

        // Everything below cannot arrive here, and is listed rather than left
        // to a default so that adding a type to DataType has to be decided here
        // too.
        //
        // A sampler and an image are set by giving them the number of a texture
        // unit, which arrives as DataType::Int above. The doubles and the non
        // square matrices have no set() taking them, so nothing can ask for
        // them yet; when one is added, this switch stops compiling until it is
        // handled.
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

void setUniformArray(NativeId p_program,
                     int p_location,
                     DataType p_type,
                     const void* p_data,
                     int p_count)
{
    if (p_count <= 0)
    {
        return;
    }
    const GLuint program = static_cast<GLuint>(p_program);
    const auto* floats = static_cast<const GLfloat*>(p_data);
    switch (p_type)
    {
        case DataType::Mat4:
            glProgramUniformMatrix4fv(
                program, p_location, p_count, GL_TRUE, floats);
            break;
        case DataType::Float:
        case DataType::Vec2:
        case DataType::Vec3:
        case DataType::Vec4:
        case DataType::Double:
        case DataType::DVec2:
        case DataType::DVec3:
        case DataType::DVec4:
        case DataType::Int:
        case DataType::IVec2:
        case DataType::IVec3:
        case DataType::IVec4:
        case DataType::UInt:
        case DataType::UVec2:
        case DataType::UVec3:
        case DataType::UVec4:
        case DataType::Bool:
        case DataType::BVec2:
        case DataType::BVec3:
        case DataType::BVec4:
        case DataType::Mat2:
        case DataType::Mat3:
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

void bindUniformBlock(NativeId p_program, int p_block_index, int p_binding)
{
    glUniformBlockBinding(static_cast<GLuint>(p_program),
                          static_cast<GLuint>(p_block_index),
                          static_cast<GLuint>(p_binding));
}

} // namespace compages::gpu::backend
