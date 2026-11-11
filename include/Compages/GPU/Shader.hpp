// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Shader programs: the small programs the graphics card runs for every
//! corner and every pixel it draws.
//!
//! A shader is written in GLSL, a language close to C. Before the card can run
//! it, it is compiled (each stage on its own: a Shader) and linked (the stages
//! joined: a Program). Errors at either step come back with the driver's log,
//! line numbers included.
//!
//! \code
//! compages::gpu::Program program;
//! COMPAGES_TRY(program.loadFiles("shaders/mesh.vert", "shaders/mesh.frag"));
//! program.set("uColor", compages::Vector4f(1.0f, 0.5f, 0.0f, 1.0f));
//! \endcode
//!
//! What it holds:
//! - Shader: one compiled stage.
//! - Program: the linked stages, their uniforms, and what they declare.
//! - liveShaders(), livePrograms(): to spot leaks.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/ShaderSource.hpp"
#include "Compages/GPU/Enums.hpp"
#include "Compages/GPU/Handle.hpp"
#include "Compages/GPU/ProgramReflection.hpp"

#include <filesystem>
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
//! \brief One piece of a shader program, translated for the graphics card
//! and ready to be joined with the others.
//!
//! A shader program is written in several pieces: one says where each corner
//! of a shape goes (the vertex stage), one says what colour each pixel is (the
//! fragment stage). Each piece is compiled on its own, like the chapters of a
//! book proofread one at a time, then the Program binds them together.
//!
//! A stage is worth having as a value of its own for two reasons. A vertex
//! shader shared by several programs is compiled once. And a compilation error
//! is reported on its own, with the log of the one stage that failed, rather
//! than being mixed into a link error that names none of them.
//!
//! \code
//! auto vertex = compages::gpu::Shader::fromFile("shaders/mesh.vert");
//! if (!vertex) { std::cerr << vertex.error(); return; }
//! \endcode
//!
//! Most callers never need this type: Program::loadFiles() does both steps.
// ****************************************************************************
class Shader: private compages::NotCopiable
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty shader: valid() is false.
    // ------------------------------------------------------------------------
    Shader() = default;

    // ------------------------------------------------------------------------
    //! \brief Compile source code for one stage.
    //!
    //! \param[in] p_stage which stage this is.
    //! \param[in] p_source the GLSL source, which must begin with its
    //! \#version.
    //! \param[in] p_name what to call it in messages, typically the file it
    //! came from.
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
    //!
    //! \param[in] p_path the file to read. Its path is the name used in error
    //! messages.
    //! \return the compiled stage, or why the file could not be read or
    //! compiled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader>
    fromFile(std::filesystem::path const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Read a file and compile it as the given stage, whatever its
    //! extension: for a .glsl file, or one holding several stages.
    //!
    //! \param[in] p_stage which stage this is.
    //! \param[in] p_path the file to read.
    //! \return the compiled stage, or why the file could not be read or
    //! compiled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader>
    fromFile(ShaderStage p_stage, std::filesystem::path const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Put the includes of a source in and compile it. The compiler
    //! log names the piece and the line of each error.
    //!
    //! \param[in] p_stage which stage this is.
    //! \param[in] p_source the main text or file, with its includes.
    //! \return the compiled stage, or why it could not be resolved or
    //! compiled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Shader> fromSource(ShaderStage p_stage,
                                                   ShaderSource const& p_source);

    //! \brief Take over the stage of p_other, which is left not valid().
    //! \param[in,out] p_other the shader to empty.
    Shader(Shader&& p_other) noexcept;
    //! \brief Release this stage, then take over p_other's.
    //! \param[in,out] p_other the shader to empty.
    //! \return this object, now owning the other stage.
    Shader& operator=(Shader&& p_other) noexcept;
    //! \brief Release the compiled stage. Programs already linked keep working.
    ~Shader();

    // ------------------------------------------------------------------------
    //! \brief Drop the compiled stage now.
    //!
    //! Safe to do as soon as every program using it has been linked: a program
    //! keeps what it needs.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Which stage this is, as given or as read from the extension.
    //! \return the stage.
    // ------------------------------------------------------------------------
    [[nodiscard]] ShaderStage stage() const;

    // ------------------------------------------------------------------------
    //! \brief What it is called in messages: the file it came from, or the
    //! name given to fromSource(); empty when none was given.
    //! \return the name; empty when none was given.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& name() const;

    // ------------------------------------------------------------------------
    //! \brief False when default constructed, moved from or released.
    //! \return true when the stage can be linked.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief The handle naming this stage, for code that keeps it without
    //! owning it.
    //! \return the handle; empty when not valid().
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
//! \brief A complete shader program: the small programs the graphics card
//! runs for every corner and every pixel of what it draws.
//!
//! The vertex stage places each corner on screen, the fragment stage colours
//! each pixel inside. A Program holds both, linked, and knows what they need:
//! which vertex fields, which values (uniforms) such as a colour or a matrix.
//!
//! \code
//! compages::gpu::Program program;
//! COMPAGES_TRY(program.loadFiles("shaders/mesh.vert", "shaders/mesh.frag"));
//! std::cout << program.reflection().toString() << std::endl;
//! program.set("uColor", compages::Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
//! \endcode
//!
//! loadSources() and loadFiles() fill this object in place, as Drawable and
//! ComputeProgram do: one COMPAGES_TRY, no temporary. link() joins stages
//! compiled beforehand. A compute program is a ComputeProgram.
//!
//! Linking is where the driver decides everything the library then relies on:
//! which slot each attribute got, where each member of a uniform block really
//! sits, which binding a storage block expects. All of it is read once, here,
//! and kept: see reflection().
// ****************************************************************************
class Program: private compages::NotCopiable
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty program: valid() is false until a load succeeds.
    // ------------------------------------------------------------------------
    Program() = default;

    // ------------------------------------------------------------------------
    //! \brief Link stages that have already been compiled.
    //!
    //! \param[in] p_stages the stages, which stay usable afterwards and may be
    //! linked into other programs.
    //! \return the linked program, or the linker log as the error.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program>
    link(std::initializer_list<Shader const*> p_stages);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from source, into
    //! this object.
    //!
    //! \code
    //! COMPAGES_TRY(m_program.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));
    //! \endcode
    //!
    //! \param[in] p_vertex the GLSL of the vertex stage.
    //! \param[in] p_fragment the GLSL of the fragment stage.
    //! \return the compiler or linker log when it failed, in which case this
    //! object is left as it was.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadSources(std::string_view p_vertex,
                                     std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Same as loadSources(), reading the two stages from files. A
    //! failure keeps the previous program, so a hot reload with a typo does not
    //! break the frame.
    //!
    //! \param[in] p_vertex path of the vertex stage.
    //! \param[in] p_fragment path of the fragment stage.
    //! \return the log, or why a file could not be read.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadFiles(std::filesystem::path const& p_vertex,
                                   std::filesystem::path const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Same as loadSources(), from two ShaderSource: texts or files
    //! with includes and hooks. A failure keeps the previous program.
    //!
    //! \code
    //! auto pbr = compages::renderer::shaders::pbr();
    //! pbr.vertex().hook("deform", WAVES);
    //! COMPAGES_TRY(m_program.load(pbr.vertex(), pbr.fragment()));
    //! \endcode
    //!
    //! \param[in] p_vertex the source of the vertex stage.
    //! \param[in] p_fragment the source of the fragment stage.
    //! \return the log, naming the piece and line of each error.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status load(ShaderSource const& p_vertex,
                              ShaderSource const& p_fragment);

    //! \brief Take over the program of p_other, which is left not valid().
    //! \param[in,out] p_other the program to empty.
    Program(Program&& p_other) noexcept;
    //! \brief Release this program, then take over p_other's.
    //! \param[in,out] p_other the program to empty.
    //! \return this object, now owning the other program.
    Program& operator=(Program&& p_other) noexcept;
    //! \brief Release the program. Pipelines still naming it fail on their
    //! next use rather than crash.
    ~Program();

    // ------------------------------------------------------------------------
    //! \brief Drop the program now rather than at the end of the scope.
    //! Pipelines still naming it fail on their next use.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief False when default constructed, moved from or released.
    //! \return true when the program can be used.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief The handle naming this program, which a Pipeline keeps.
    //! \return the handle; empty when not valid().
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramHandle handle() const
    {
        return m_handle;
    }

    // ------------------------------------------------------------------------
    //! \brief Everything the program declares, as the driver reports it.
    //!
    //! Read once at link time, so asking costs nothing.
    //! \return the reflection, read once at link time.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramReflection const& reflection() const;

    // ------------------------------------------------------------------------
    //! \brief Does the program declare, and read, a uniform of this name?
    //!
    //! A uniform the shader declares but never reads is removed by the
    //! compiler, so this also answers whether setting it would do anything.
    //!
    //! \param[in] p_name the uniform in GLSL, spelled as reflection() lists it.
    //! \return true when setting this uniform has an effect.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool has(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief Set a uniform, checking that the shader declares it with a
    //! matching type.
    //!
    //! \code
    //! program.set("uModel", model);
    //! program.set("uColor", compages::Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
    //! program.set("uTexture", 0);   // texture unit
    //! \endcode
    //!
    //! Nothing is returned: a uniform is set every frame, and a failure there
    //! is a mistake to fix rather than a case to handle. No uniform of that
    //! name, or one declared with another type, is recorded as the frame error
    //! (see compages::gpu::takeFrameError()). Both used to pass unnoticed and
    //! show up as a black screen.
    //!
    //! \note For anything that changes every frame and is shared by several
    //! programs, a uniform block is the better tool: one buffer written once
    //! rather than one call per program per value.
    //!
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value, kept by the program until set again.
    // ------------------------------------------------------------------------
    void set(std::string_view p_name, float p_value);
    //! \brief Set a vec2 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Vector2f const& p_value);
    //! \brief Set a vec3 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Vector3f const& p_value);
    //! \brief Set a vec4 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Vector4f const& p_value);
    //! \brief Set an int uniform, or the texture unit of a sampler.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, int p_value);
    //! \brief Set a uint uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, unsigned int p_value);
    //! \brief Set a bool uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, bool p_value);
    //! \brief Set an ivec2 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Vector2i const& p_value);
    //! \brief Set an ivec3 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Vector3i const& p_value);
    //! \brief Set an ivec4 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Vector4i const& p_value);
    //! \brief Set a mat2 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Matrix22f const& p_value);
    //! \brief Set a mat3 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Matrix33f const& p_value);
    //! \brief Set a mat4 uniform.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_value its new value.
    void set(std::string_view p_name, compages::Matrix44f const& p_value);
    //! \brief Set a mat4 array, such as a joint palette.
    //! \param[in] p_name the uniform in GLSL.
    //! \param[in] p_values the matrices, in array order.
    void set(std::string_view p_name,
             std::span<const compages::Matrix44f> p_values);

    // ------------------------------------------------------------------------
    //! \brief Say which binding point a uniform block is to be read from.
    //!
    //! Only needed when the shader did not say so itself with
    //! `layout(binding = 1)`, which is the clearer place to say it.
    //!
    //! \param[in] p_name the block name in GLSL (Camera in "uniform Camera {").
    //! \param[in] p_binding the point a UniformBlock is attached to.
    //! \return an error when the program declares no such block.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bindUniformBlock(std::string_view p_name,
                                          int p_binding);

private:

    //! \brief ComputeProgram builds its program with fromComputeSource() and
    //! fromComputeFile().
    friend class ComputeProgram;

    explicit Program(ProgramHandle p_handle) : m_handle(p_handle) {}

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from source, for
    //! shaders kept as strings in the C++ file.
    //!
    //! \param[in] p_vertex the GLSL of the vertex stage, from its \#version
    //! line.
    //! \param[in] p_fragment the GLSL of the fragment stage.
    //! \return the program, or the log of the stage that failed.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program>
    fromSources(std::string_view p_vertex, std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a vertex and a fragment stage from files.
    //!
    //! What almost every caller wants. On failure the error names the file that
    //! would not compile, followed by the log of the driver.
    //!
    //! \param[in] p_vertex path of the vertex stage, usually a .vert file.
    //! \param[in] p_fragment path of the fragment stage, usually a .frag file.
    //! \return the program, or the name of the file that failed and the driver
    //! log.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program>
    fromFiles(std::filesystem::path const& p_vertex,
              std::filesystem::path const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from source.
    //!
    //! A compute program has one stage and no drawing: it runs on a grid of
    //! work groups whose size the shader itself declares, which reflection()
    //! reports as work_group_size. ComputeProgram wraps this with dispatch().
    //!
    //! \param[in] p_source the GLSL of the compute stage.
    //! \return the program, or the compiler log.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program>
    fromComputeSource(std::string_view p_source);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from a file.
    //!
    //! \param[in] p_path the file to read, any extension.
    //! \return the program, or why the file could not be read or compiled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program>
    fromComputeFile(std::filesystem::path const& p_path);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from a ShaderSource.
    //!
    //! \param[in] p_source the main text or file, with its includes.
    //! \return the program, or why it could not be resolved or compiled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Program>
    fromComputeSource(ShaderSource const& p_source);


    //! \brief Find a uniform and check its declared type before writing to it.
    [[nodiscard]] Result<int> locationOf(std::string_view p_name,
                                         DataType p_expected) const;

    ProgramHandle m_handle;
};

// ----------------------------------------------------------------------------
//! \brief How many compiled stages are alive. Used to spot leaks; a Shader
//! kept after linking counts here.
//! \return the number of live compiled stages.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t liveShaders();

// ----------------------------------------------------------------------------
//! \brief How many linked programs are alive. Used to spot leaks.
//! \return the number of live programs.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t livePrograms();

} // namespace compages::gpu
