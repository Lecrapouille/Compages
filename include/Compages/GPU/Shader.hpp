// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Core/Enums.hpp"
#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Core/Reflection.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <initializer_list>
#include <span>
#include <string>
#include <string_view>

namespace compages::gpu
{

//! \brief Names one compiled stage.
using ShaderHandle = Handle<struct ShaderTag>;

//! \brief Names one linked program.
using ProgramHandle = Handle<struct ProgramTag>;

// ****************************************************************************
//! \brief One stage of a pipeline, compiled and ready to be linked.
//!
//! A stage is worth having as a value of its own for two reasons. A vertex shader
//! shared by several programs is compiled once. And a compilation error is
//! reported on its own, with the log of the one stage that failed, rather than
//! being mixed into a link error that names none of them.
//!
//! \code
//! auto vertex = compages::gpu::Shader::fromFile("shaders/mesh.vert");
//! if (!vertex) { std::cerr << vertex.error(); return; }
//! \endcode
//!
//! Most callers never need this type: Program::fromFiles() does both steps.
// ****************************************************************************
class Shader
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty shader, holding nothing.
    // ------------------------------------------------------------------------
    Shader() = default;

    // ------------------------------------------------------------------------
    //! \brief Compile source code for one stage.
    //!
    //! \param[in] p_stage which stage this is.
    //! \param[in] p_source the GLSL source, which must begin with its #version.
    //! \param[in] p_name what to call it in messages, typically the file it came
    //! from.
    //! \return the compiled stage, or the compiler log as the error. The log is
    //! passed through as the driver wrote it, since its line numbers are what
    //! makes it useful.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromSource(ShaderStage p_stage,
                                                   std::string_view p_source,
                                                   std::string p_name = {});

    // ------------------------------------------------------------------------
    //! \brief Read a file and compile it, taking the stage from the extension.
    //!
    //! Recognises .vert and .vs, .frag and .fs, .geom and .gs, .comp and .cs,
    //! .tesc and .tese. Use the overload taking a stage for anything else.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromFile(std::string const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Read a file and compile it as the given stage.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromFile(ShaderStage p_stage,
                                                 std::string const& p_path);

    Shader(Shader&& p_other) noexcept;
    Shader& operator=(Shader&& p_other) noexcept;
    Shader(Shader const&) = delete;
    Shader& operator=(Shader const&) = delete;
    ~Shader();

    // ------------------------------------------------------------------------
    //! \brief Drop the compiled stage now.
    //!
    //! Safe to do as soon as every program using it has been linked: a program
    //! keeps what it needs.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Which stage this is.
    // ------------------------------------------------------------------------
    [[nodiscard]] ShaderStage stage() const;

    // ------------------------------------------------------------------------
    //! \brief What it is called in messages.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& name() const;

    // ------------------------------------------------------------------------
    //! \brief Is there a compiled stage here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief Name of the compiled stage.
    // ------------------------------------------------------------------------
    [[nodiscard]] ShaderHandle handle() const
    {
        return m_handle;
    }

private:

    explicit Shader(ShaderHandle p_handle) : m_handle(p_handle) {}

    ShaderHandle m_handle;
};

// ****************************************************************************
//! \brief Several stages linked together, and everything they turn out to
//! declare.
//!
//! \code
//! compages::gpu::Program program;
//! COMPAGES_TRY(program.loadFiles("shaders/mesh.vert", "shaders/mesh.frag"));
//! std::cout << program.reflection().toString() << std::endl;
//! program.set("uColor", Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
//! \endcode
//!
//! The static fromSources(), fromFiles() and link() return the program as a
//! Result, which suits code filling a container. The load() family fills this
//! object in place, which suits a member: one COMPAGES_TRY, no temporary.
//!
//! Linking is where the driver decides everything the library then relies on:
//! which slot each attribute got, where each member of a uniform block really
//! sits, which binding a storage block expects. All of it is read once, here, and
//! kept: see reflection().
// ****************************************************************************
class Program
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty program.
    // ------------------------------------------------------------------------
    Program() = default;

    // ------------------------------------------------------------------------
    //! \brief Link stages that have already been compiled.
    //!
    //! \param[in] p_stages the stages, which stay usable afterwards and may be
    //! linked into other programs.
    //! \return the linked program, or the linker log as the error.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> link(
        std::initializer_list<Shader const*> p_stages);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from source.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromSources(
        std::string_view p_vertex, std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from files.
    //!
    //! What almost every caller wants. On failure the error names the file that
    //! would not compile, followed by the log of the driver.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromFiles(
        std::string const& p_vertex, std::string const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from source.
    //!
    //! A compute program has one stage and no drawing: it runs on a grid of work
    //! groups whose size the shader itself declares, which reflection() reports
    //! as work_group_size.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromComputeSource(
        std::string_view p_source);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from a file.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program> fromComputeFile(
        std::string const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from source, into
    //! this object.
    //!
    //! \code
    //! COMPAGES_TRY(m_program.load(VERTEX_SHADER, FRAGMENT_SHADER));
    //! \endcode
    //!
    //! \return the compiler or linker log when it failed, in which case this
    //! object is left as it was.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status load(std::string_view p_vertex,
                              std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Same as load(), reading the two stages from files.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadFiles(std::string const& p_vertex,
                                   std::string const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute stage from source, into this object.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadCompute(std::string_view p_source);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute stage from a file, into this object.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadComputeFile(std::string const& p_path);

    Program(Program&& p_other) noexcept;
    Program& operator=(Program&& p_other) noexcept;
    Program(Program const&) = delete;
    Program& operator=(Program const&) = delete;
    ~Program();

    // ------------------------------------------------------------------------
    //! \brief Drop the program now rather than at the end of the scope.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Is there a linked program here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief Name of the linked program.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramHandle handle() const
    {
        return m_handle;
    }

    // ------------------------------------------------------------------------
    //! \brief Everything the program declares, as the driver reports it.
    //!
    //! Read once at link time, so asking costs nothing.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramReflection const& reflection() const;

    // ------------------------------------------------------------------------
    //! \brief Does the program declare, and read, a uniform of this name?
    //!
    //! A uniform the shader declares but never reads is removed by the
    //! compiler, so this also answers whether setting it would do anything.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool has(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief Set a uniform, checking that the shader declares it with a
    //! matching type.
    //!
    //! \code
    //! program.set("uModel", model);
    //! program.set("uColor", Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
    //! program.set("uTexture", 0);   // texture unit
    //! \endcode
    //!
    //! Nothing is returned: a uniform is set every frame, and a failure there
    //! is a mistake to fix rather than a case to handle. No uniform of that
    //! name, or one declared with another type, is recorded as the frame error
    //! (see compages::gpu::takeFrameError()). Both used to pass unnoticed and show up as
    //! a black screen.
    //!
    //! \note For anything that changes every frame and is shared by several
    //! programs, a uniform block is the better tool: one buffer written once
    //! rather than one call per program per value.
    // ------------------------------------------------------------------------
    void set(std::string_view p_name, float p_value);
    //! \brief Set a vec2 uniform.
    void set(std::string_view p_name, Vector2f const& p_value);
    //! \brief Set a vec3 uniform.
    void set(std::string_view p_name, Vector3f const& p_value);
    //! \brief Set a vec4 uniform.
    void set(std::string_view p_name, Vector4f const& p_value);
    //! \brief Set an int uniform, or the texture unit of a sampler.
    void set(std::string_view p_name, int p_value);
    //! \brief Set a uint uniform.
    void set(std::string_view p_name, unsigned int p_value);
    //! \brief Set a bool uniform.
    void set(std::string_view p_name, bool p_value);
    //! \brief Set an ivec2 uniform.
    void set(std::string_view p_name, Vector2i const& p_value);
    //! \brief Set an ivec3 uniform.
    void set(std::string_view p_name, Vector3i const& p_value);
    //! \brief Set an ivec4 uniform.
    void set(std::string_view p_name, Vector4i const& p_value);
    //! \brief Set a mat2 uniform.
    void set(std::string_view p_name, Matrix22f const& p_value);
    //! \brief Set a mat3 uniform.
    void set(std::string_view p_name, Matrix33f const& p_value);
    //! \brief Set a mat4 uniform.
    void set(std::string_view p_name, Matrix44f const& p_value);
    //! \brief Set a mat4 array, such as a joint palette.
    void set(std::string_view p_name, std::span<const Matrix44f> p_values);

    // ------------------------------------------------------------------------
    //! \brief Say which binding point a uniform block is to be read from.
    //!
    //! Only needed when the shader did not say so itself with
    //! `layout(binding = 1)`, which is the clearer place to say it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bindUniformBlock(std::string_view p_name,
                                          int p_binding);

private:

    explicit Program(ProgramHandle p_handle) : m_handle(p_handle) {}

    //! \brief Find a uniform and check its declared type before writing to it.
    [[nodiscard]] Result<int> locationOf(std::string_view p_name,
                                         DataType p_expected) const;

    ProgramHandle m_handle;
};

// ----------------------------------------------------------------------------
//! \brief How many compiled stages are alive. Used to spot leaks.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t liveShaders();

// ----------------------------------------------------------------------------
//! \brief How many linked programs are alive. Used to spot leaks.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t livePrograms();

} // namespace compages::gpu
