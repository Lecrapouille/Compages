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

#include "Compages/GPU/Shader.hpp"
#include "Compages/Core/File.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/Errors.hpp"
#include "GPU/Internal/Pools.hpp"

#include <algorithm>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Compiling stages, linking them, and asking the result what it holds.
//!
//! \note On matrices. \c compages::core matrices are row-major with the Scilab
//! convention (\c y = M * x). OpenGL expects column-major memory, so the GL45
//! backend uploads them with \c glProgramUniformMatrix*fv(..., GL_TRUE, ...).
// ****************************************************************************

namespace compages::gpu
{

namespace
{

//------------------------------------------------------------------------------
//! \brief Which stage a file holds, going by its extension.
//!
//! The extensions are the ones the Khronos reference compiler established, plus
//! the shorter forms in wide use.
//------------------------------------------------------------------------------
Result<ShaderStage> stageOfFile(std::string const& p_path)
{
    const std::string extension = compages::core::File::extension(p_path);

    if ((extension == "vert") || (extension == "vs"))
    {
        return ShaderStage::Vertex;
    }
    if ((extension == "frag") || (extension == "fs"))
    {
        return ShaderStage::Fragment;
    }
    if ((extension == "geom") || (extension == "gs"))
    {
        return ShaderStage::Geometry;
    }
    if ((extension == "comp") || (extension == "cs"))
    {
        return ShaderStage::Compute;
    }
    if (extension == "tesc")
    {
        return ShaderStage::TessellationControl;
    }
    if (extension == "tese")
    {
        return ShaderStage::TessellationEvaluation;
    }

    return failure("cannot tell which pipeline stage '" + p_path +
                   "' is meant to be: its extension is '" + extension +
                   "' and none of .vert, .frag, .geom, .comp, .tesc or .tese. "
                   "Say the stage explicitly instead");
}

//------------------------------------------------------------------------------
Result<std::string> readSource(std::string const& p_path)
{
    std::string source;
    if (!compages::core::File::readAllFile(p_path, source))
    {
        return failure("cannot read the shader '" + p_path + "'");
    }
    if (source.empty())
    {
        return failure("the shader '" + p_path + "' is empty");
    }
    return source;
}

//------------------------------------------------------------------------------
//! \brief Say what is wrong when a uniform is written with the wrong type.
//!
//! Worth the words: the two ways to get here, a name that does not exist and a
//! type that does not match, used to look identical from the outside, which is
//! to say they looked like nothing happening at all.
//------------------------------------------------------------------------------
std::string mismatchMessage(std::string_view p_name,
                            DataType p_expected,
                            DataType p_declared)
{
    return "the shader declares '" + std::string(p_name) + "' as " +
           toString(p_declared) + ", but it is being set as " +
           toString(p_expected);
}

} // namespace

//------------------------------------------------------------------------------
Result<Shader> Shader::fromSource(ShaderStage p_stage,
                                  std::string_view p_source,
                                  std::string p_name)
{
    if (!initialized())
    {
        return failure(
            "compages::gpu::init() has not been called, so there is no device "
            "to compile a shader on");
    }
    if (p_source.empty())
    {
        return failure("there is no source to compile");
    }

    auto compiled = backend::compileShader(p_stage, p_source);
    if (!compiled)
    {
        // The compiler log says which line, but not which file, so both are put
        // in front of the reader.
        const std::string where =
            p_name.empty() ? std::string(toString(p_stage)) + " shader"
                           : "'" + p_name + "'";
        return failure(where + " did not compile:\n" + compiled.error());
    }

    auto added = detail::pools().shaders.add(
        detail::ShaderRecord{ compiled.value(), p_stage, std::move(p_name) });
    if (!added)
    {
        backend::destroyShader(compiled.value());
        return failure(added.error());
    }
    return Shader(added.take());
}

//------------------------------------------------------------------------------
Result<Shader> Shader::fromFile(std::string const& p_path)
{
    auto stage = stageOfFile(p_path);
    if (!stage)
    {
        return failure(stage.error());
    }
    return fromFile(stage.value(), p_path);
}

//------------------------------------------------------------------------------
Result<Shader> Shader::fromFile(ShaderStage p_stage, std::string const& p_path)
{
    auto source = readSource(p_path);
    if (!source)
    {
        return failure(source.error());
    }
    return fromSource(p_stage, source.value(), p_path);
}

//------------------------------------------------------------------------------
Shader::Shader(Shader&& p_other) noexcept : m_handle(p_other.m_handle)
{
    p_other.m_handle = ShaderHandle{};
}

//------------------------------------------------------------------------------
Shader& Shader::operator=(Shader&& p_other) noexcept
{
    if (this != &p_other)
    {
        release();
        m_handle = p_other.m_handle;
        p_other.m_handle = ShaderHandle{};
    }
    return *this;
}

//------------------------------------------------------------------------------
Shader::~Shader()
{
    release();
}

//------------------------------------------------------------------------------
void Shader::release()
{
    detail::ShaderRecord* record = detail::pools().shaders.get(m_handle);
    if (record != nullptr)
    {
        if (initialized())
        {
            backend::destroyShader(record->native);
        }
        (void)detail::pools().shaders.remove(m_handle);
    }
    m_handle = ShaderHandle{};
}

//------------------------------------------------------------------------------
ShaderStage Shader::stage() const
{
    detail::ShaderRecord const* record = detail::pools().shaders.get(m_handle);
    assert((record != nullptr) && "stage() of a shader that no longer exists");
    return (record == nullptr) ? ShaderStage::Vertex : record->stage;
}

//------------------------------------------------------------------------------
std::string const& Shader::name() const
{
    static const std::string nothing;
    detail::ShaderRecord const* record = detail::pools().shaders.get(m_handle);
    return (record == nullptr) ? nothing : record->name;
}

//------------------------------------------------------------------------------
bool Shader::valid() const
{
    return detail::pools().shaders.valid(m_handle);
}

//------------------------------------------------------------------------------
Result<Program> Program::link(std::initializer_list<Shader const*> p_stages)
{
    if (!initialized())
    {
        return failure(
            "compages::gpu::init() has not been called, so there is no device "
            "to link a program on");
    }
    if (p_stages.size() == 0u)
    {
        return failure("a program needs at least one stage to link");
    }

    std::vector<backend::NativeId> natives;
    std::vector<ShaderStage> stages;
    natives.reserve(p_stages.size());
    stages.reserve(p_stages.size());

    for (Shader const* one : p_stages)
    {
        if ((one == nullptr) || !one->valid())
        {
            return failure(
                "one of the stages given to link() holds no compiled "
                "shader. A Shader that failed to compile, or one that "
                "has been released or moved from");
        }
        detail::ShaderRecord const* record =
            detail::pools().shaders.get(one->handle());
        natives.emplace_back(record->native);
        stages.emplace_back(record->stage);
    }

    // Two stages of the same kind cannot both be linked, and the driver's word
    // for it is rarely the first thing a reader understands.
    std::vector<ShaderStage> sorted = stages;
    std::sort(sorted.begin(), sorted.end());
    auto duplicate = std::adjacent_find(sorted.begin(), sorted.end());
    if (duplicate != sorted.end())
    {
        return failure(std::string("two ") + toString(*duplicate) +
                       " stages were given to link(), and a program can only "
                       "have one of each");
    }

    auto linked = backend::linkProgram(natives);
    if (!linked)
    {
        return failure("the stages did not link:\n" + linked.error());
    }

    detail::ProgramRecord record;
    record.native = linked.value();
    backend::reflectProgram(record.native, stages, record.reflection);

    auto added = detail::pools().programs.add(std::move(record));
    if (!added)
    {
        backend::destroyProgram(linked.value());
        return failure(added.error());
    }
    return Program(added.take());
}

//------------------------------------------------------------------------------
Result<Program> Program::fromSources(std::string_view p_vertex,
                                     std::string_view p_fragment)
{
    Shader vertex;
    Shader fragment;
    COMPAGES_TRY_ASSIGN(vertex,
                        Shader::fromSource(ShaderStage::Vertex, p_vertex));
    COMPAGES_TRY_ASSIGN(fragment,
                        Shader::fromSource(ShaderStage::Fragment, p_fragment));
    return link({ &vertex, &fragment });
}

//------------------------------------------------------------------------------
Result<Program> Program::fromFiles(std::string const& p_vertex,
                                   std::string const& p_fragment)
{
    Shader vertex;
    Shader fragment;
    COMPAGES_TRY_ASSIGN(vertex,
                        Shader::fromFile(ShaderStage::Vertex, p_vertex));
    COMPAGES_TRY_ASSIGN(fragment,
                        Shader::fromFile(ShaderStage::Fragment, p_fragment));
    return link({ &vertex, &fragment });
}

//------------------------------------------------------------------------------
Result<Program> Program::fromComputeSource(std::string_view p_source)
{
    Shader compute;
    COMPAGES_TRY_ASSIGN(compute,
                        Shader::fromSource(ShaderStage::Compute, p_source));
    return link({ &compute });
}

//------------------------------------------------------------------------------
Result<Program> Program::fromComputeFile(std::string const& p_path)
{
    Shader compute;
    COMPAGES_TRY_ASSIGN(compute,
                        Shader::fromFile(ShaderStage::Compute, p_path));
    return link({ &compute });
}

//------------------------------------------------------------------------------
Status Program::load(std::string_view p_vertex, std::string_view p_fragment)
{
    COMPAGES_TRY_ASSIGN(*this, fromSources(p_vertex, p_fragment));
    return success();
}

//------------------------------------------------------------------------------
Status Program::loadFiles(std::string const& p_vertex,
                          std::string const& p_fragment)
{
    COMPAGES_TRY_ASSIGN(*this, fromFiles(p_vertex, p_fragment));
    return success();
}

//------------------------------------------------------------------------------
Status Program::loadCompute(std::string_view p_source)
{
    COMPAGES_TRY_ASSIGN(*this, fromComputeSource(p_source));
    return success();
}

//------------------------------------------------------------------------------
Status Program::loadComputeFile(std::string const& p_path)
{
    COMPAGES_TRY_ASSIGN(*this, fromComputeFile(p_path));
    return success();
}

//------------------------------------------------------------------------------
Program::Program(Program&& p_other) noexcept : m_handle(p_other.m_handle)
{
    p_other.m_handle = ProgramHandle{};
}

//------------------------------------------------------------------------------
Program& Program::operator=(Program&& p_other) noexcept
{
    if (this != &p_other)
    {
        release();
        m_handle = p_other.m_handle;
        p_other.m_handle = ProgramHandle{};
    }
    return *this;
}

//------------------------------------------------------------------------------
Program::~Program()
{
    release();
}

//------------------------------------------------------------------------------
void Program::release()
{
    detail::ProgramRecord* record = detail::pools().programs.get(m_handle);
    if (record != nullptr)
    {
        if (initialized())
        {
            backend::destroyProgram(record->native);
        }
        (void)detail::pools().programs.remove(m_handle);
    }
    m_handle = ProgramHandle{};
}

//------------------------------------------------------------------------------
bool Program::valid() const
{
    return detail::pools().programs.valid(m_handle);
}

//------------------------------------------------------------------------------
ProgramReflection const& Program::reflection() const
{
    static const ProgramReflection nothing;
    detail::ProgramRecord const* record =
        detail::pools().programs.get(m_handle);
    return (record == nullptr) ? nothing : record->reflection;
}

//------------------------------------------------------------------------------
bool Program::has(std::string_view p_name) const
{
    return reflection().uniform(p_name) != nullptr;
}

//------------------------------------------------------------------------------
Result<int> Program::locationOf(std::string_view p_name,
                                DataType p_expected) const
{
    detail::ProgramRecord const* record =
        detail::pools().programs.get(m_handle);
    if (record == nullptr)
    {
        return failure("this program no longer exists. Either it was released "
                       "while something still referred to it, or the Program "
                       "object was moved from");
    }

    UniformInfo const* uniform = record->reflection.uniform(p_name);
    if (uniform == nullptr)
    {
        // A uniform the shader declares but never reads is optimized away, and
        // that is by far the most common reason for landing here.
        BlockMember const* in_block = nullptr;
        std::string block_name;
        for (BlockInfo const& block : record->reflection.uniform_blocks)
        {
            in_block = block.find(p_name);
            if (in_block != nullptr)
            {
                block_name = block.name;
                break;
            }
        }
        if (in_block != nullptr)
        {
            return failure(
                "'" + std::string(p_name) + "' belongs to the uniform block '" +
                block_name +
                "', so it is written into that block's buffer rather "
                "than one value at a time");
        }

        return failure(
            "this program declares no uniform called '" + std::string(p_name) +
            "'. Either the name differs from the shader, or the "
            "shader never reads it and the compiler removed it. What "
            "it does declare:\n" +
            record->reflection.toString());
    }

    // A sampler is set by giving it the number of a texture unit, which is an
    // integer, so asking for an int where the shader wants a sampler is right.
    const bool sampler_as_integer =
        (p_expected == DataType::Int) &&
        (isSampler(uniform->type) || isImage(uniform->type));

    if ((uniform->type != p_expected) && !sampler_as_integer)
    {
        return failure(mismatchMessage(p_name, p_expected, uniform->type));
    }

    return uniform->location;
}

//------------------------------------------------------------------------------
//! \brief The body every set() shares: find the uniform, check the type, write.
//------------------------------------------------------------------------------
#define GPU_SET_UNIFORM(name, type, data)                                   \
    do                                                                      \
    {                                                                       \
        auto location = locationOf(name, type);                             \
        if (!location)                                                      \
        {                                                                   \
            reportError(location.error());                                  \
            return;                                                         \
        }                                                                   \
        backend::setUniform(detail::pools().programs.get(m_handle)->native, \
                            location.value(),                               \
                            type,                                           \
                            data);                                          \
    } while (false)

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name, float p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Float, &p_value);
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Vector2f const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Vec2, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Vector3f const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Vec3, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Vector4f const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Vec4, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name, int p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Int, &p_value);
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name, unsigned int p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::UInt, &p_value);
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name, bool p_value)
{
    // GLSL has a bool type but the driver is given an int, since a C++ bool is
    // one byte and the shader expects four.
    const int as_int = p_value ? 1 : 0;
    GPU_SET_UNIFORM(p_name, DataType::Bool, &as_int);
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Vector2i const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::IVec2, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Vector3i const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::IVec3, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Vector4i const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::IVec4, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Matrix22f const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Mat2, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Matrix33f const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Mat3, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  compages::core::Matrix44f const& p_value)
{
    GPU_SET_UNIFORM(p_name, DataType::Mat4, p_value.data());
}

//------------------------------------------------------------------------------
void Program::set(std::string_view p_name,
                  std::span<const compages::core::Matrix44f> p_values)
{
    auto location = locationOf(p_name, DataType::Mat4);
    if (!location)
    {
        reportError(location.error());
        return;
    }
    if (p_values.empty())
    {
        return;
    }
    backend::setUniformArray(detail::pools().programs.get(m_handle)->native,
                             location.value(),
                             DataType::Mat4,
                             p_values.data()->data(),
                             static_cast<int>(p_values.size()));
}

#undef GPU_SET_UNIFORM

//------------------------------------------------------------------------------
Status Program::bindUniformBlock(std::string_view p_name, int p_binding)
{
    detail::ProgramRecord* record = detail::pools().programs.get(m_handle);
    if (record == nullptr)
    {
        return failure("this program no longer exists");
    }

    std::vector<BlockInfo>& blocks = record->reflection.uniform_blocks;
    for (std::size_t i = 0u; i < blocks.size(); ++i)
    {
        if (blocks[i].name == p_name)
        {
            backend::bindUniformBlock(
                record->native, static_cast<int>(i), p_binding);
            // Keep what we report in step with what we just told the driver.
            blocks[i].binding = p_binding;
            return success();
        }
    }

    return failure("this program declares no uniform block called '" +
                   std::string(p_name) + "'. What it does declare:\n" +
                   record->reflection.toString());
}

//------------------------------------------------------------------------------
std::size_t liveShaders()
{
    return detail::pools().shaders.size();
}

//------------------------------------------------------------------------------
std::size_t livePrograms()
{
    return detail::pools().programs.size();
}

} // namespace compages::gpu
