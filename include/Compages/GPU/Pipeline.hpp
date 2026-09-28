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

#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Core/Layout.hpp"
#include "Compages/GPU/Core/RenderState.hpp"
#include "Compages/GPU/Shader.hpp"

#include <string>
#include <type_traits>

// ****************************************************************************
//! \file
//! \brief One complete way of drawing: a program, a vertex layout, and a state.
//!
//! This is where the reversal the whole library is built around actually
//! happens, so it is worth stating plainly.
//!
//! The previous design derived the vertex layout from the shader. A program was
//! introspected, a buffer was created for each attribute it declared, and a VAO
//! belonged to one program forever: `GLProgram::bind(vao)` refused a VAO that had
//! been used with another program. So a mesh could not be drawn twice with two
//! different shaders, which is what a second material, a shadow pass or a
//! wireframe overlay all need.
//!
//! Here the layout is declared in C++, and the shader is only asked to confirm
//! that it can be fed by it. The consequence is the point of the exercise: one
//! Buffer<Vertex> holding position, normal and texture coordinates feeds a
//! forward pass that reads all three and a shadow pass that reads only the
//! position, and neither pass knows about the other. That is example
//! 06_MultiPassMesh.
//!
//! And the confirmation is a real one. A layout offering "aPosition" to a shader
//! declaring "position" used to draw nothing, silently; it is now an error naming
//! both lists. Same for a vec2 field feeding a vec3 attribute, and for whole
//! numbers feeding a float attribute.
// ****************************************************************************

namespace compages::gpu
{

//! \brief Names one pipeline.
using PipelineHandle = Handle<struct PipelineTag>;

// ****************************************************************************
//! \brief A program, a vertex layout and a render state, checked against each
//! other once and then never again.
//!
//! Immutable on purpose. Everything that could be wrong about a way of drawing is
//! found out when the pipeline is created, which is at load time, where a message
//! can be read and acted on. Nothing is checked per draw, and nothing can be
//! changed behind a caller's back between two draws.
//!
//! \code
//! struct Vertex
//! {
//!     Vector3f position;
//!     Vector3f normal;
//!     Vector2f uv;
//! };
//!
//! // The layout is read off the struct: position, normal and uv feed the
//! // shader attributes of the same names.
//! COMPAGES_TRY_ASSIGN(m_forward, compages::gpu::Pipeline::create<Vertex>(
//!     m_program, { .depth_test = true, .cull = compages::gpu::CullMode::Back }));
//! \endcode
//!
//! Most code never builds one: compages::gpu::Drawable makes the pipeline it needs from
//! its shader and its vertices. A Pipeline written by hand is for sharing one
//! program between several vertex layouts or render states.
//!
//! \note A pipeline does not own its program: it names it. The program has to
//! outlive it. Releasing the program first is caught rather than crashed on, as
//! a failure the next time the pipeline is used.
// ****************************************************************************
class Pipeline
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty pipeline, drawing nothing.
    // ------------------------------------------------------------------------
    Pipeline() = default;

    // ------------------------------------------------------------------------
    //! \brief Check a program against a vertex layout and keep the pair.
    //!
    //! \param[in] p_program the linked program. It must stay alive as long as
    //! this pipeline is used.
    //! \param[in] p_layout what one vertex looks like. Every attribute the
    //! shader declares has to be found here, by name and by type; fields the
    //! shader does not read are allowed and ignored, which is what lets one
    //! buffer feed several passes.
    //! \param[in] p_state how to draw. Defaults to the simplest thing that
    //! works: filled triangles, no depth test, no blending.
    //! \return the pipeline, or what the shader wants that the layout does not
    //! offer, with both lists printed.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Pipeline> create(Program const& p_program,
                                                 VertexLayout const& p_layout,
                                                 RenderState const& p_state = {});

    // ------------------------------------------------------------------------
    //! \brief The same thing, saying which C++ struct the layout describes.
    //!
    //! Worth preferring: it checks that the layout really is the layout of that
    //! struct, which catches the mistake of passing the layout of one vertex type
    //! while drawing from a buffer of another. That mistake reads the right
    //! number of bytes from the wrong places, so it draws something, which is why
    //! it is worth refusing here.
    //!
    //! \tparam Vertex the struct one element of the vertex buffer holds.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] static Result<Pipeline> create(Program const& p_program,
                                                 VertexLayout const& p_layout,
                                                 RenderState const& p_state = {})
    {
        static_assert(std::is_standard_layout_v<Vertex>,
                      "a vertex struct must be standard layout, otherwise where "
                      "its members sit is not something the GPU can be told");
        static_assert(std::is_trivially_copyable_v<Vertex>,
                      "a vertex struct must be trivially copyable, since it is "
                      "sent to the device as the bytes it already is");

        if (p_layout.stride() != sizeof(Vertex))
        {
            return failure(
                "this layout describes a vertex of " +
                std::to_string(p_layout.stride()) +
                " bytes, but the type given is " + std::to_string(sizeof(Vertex)) +
                " bytes. Either the layout belongs to another vertex struct, or "
                "a field was added to the struct and not described");
        }
        return create(p_program, p_layout, p_state);
    }

    // ------------------------------------------------------------------------
    //! \brief Same, with the layout read off the struct by
    //! VertexLayout::of<Vertex>().
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] static Result<Pipeline> create(Program const& p_program,
                                                 RenderState const& p_state = {})
    {
        return create<Vertex>(p_program, VertexLayout::of<Vertex>(), p_state);
    }

    Pipeline(Pipeline&& p_other) noexcept;
    Pipeline& operator=(Pipeline&& p_other) noexcept;
    Pipeline(Pipeline const&) = delete;
    Pipeline& operator=(Pipeline const&) = delete;
    ~Pipeline();

    // ------------------------------------------------------------------------
    //! \brief Drop the pipeline now rather than at the end of the scope.
    //!
    //! Does not touch the program, which the pipeline never owned.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Is there a pipeline here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief Make this the way the next draw happens.
    //!
    //! Chooses the program, tells the device how to read a vertex, and applies
    //! the whole render state. Called by draw(), so a caller normally has no
    //! reason to; it is public for the passes the library does not offer yet.
    //!
    //! \return why it could not be done, which for a live pipeline means the
    //! program was released while this pipeline still named it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind() const;

    // ------------------------------------------------------------------------
    //! \brief Which program this draws with.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramHandle program() const;

    // ------------------------------------------------------------------------
    //! \brief What one vertex looks like, as it was checked.
    // ------------------------------------------------------------------------
    [[nodiscard]] VertexLayout const& layout() const;

    // ------------------------------------------------------------------------
    //! \brief How this draws.
    // ------------------------------------------------------------------------
    [[nodiscard]] RenderState const& state() const;

    // ------------------------------------------------------------------------
    //! \brief Distance in bytes from one vertex to the next, which is the size of
    //! the struct the vertex buffer holds.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uint32_t stride() const;

    // ------------------------------------------------------------------------
    //! \brief Does anything here change once per instance rather than once per
    //! vertex? True when this pipeline is meant for instanced drawing.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool instanced() const;

    // ------------------------------------------------------------------------
    //! \brief Which fields of the layout the shader actually reads, and at which
    //! attribute slot the driver put each one.
    //!
    //! The result of the check, kept because it is also the answer to "why is my
    //! attribute not arriving": a field absent from this list is one the shader
    //! never declared, and is not being sent anywhere.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string describeAttributes() const;

    // ------------------------------------------------------------------------
    //! \brief Name of the pipeline.
    // ------------------------------------------------------------------------
    [[nodiscard]] PipelineHandle handle() const
    {
        return m_handle;
    }

private:

    explicit Pipeline(PipelineHandle p_handle) : m_handle(p_handle) {}

    PipelineHandle m_handle;
};

// ----------------------------------------------------------------------------
//! \brief How many pipelines are alive. Used to spot leaks.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t livePipelines();

// ----------------------------------------------------------------------------
//! \brief How many distinct ways of reading a vertex the device is holding.
//!
//! Pipelines that read a vertex the same way share one, so this is at most the
//! number of live pipelines and usually far less: the three passes of
//! 06_MultiPassMesh read the same vertex, so they cost one between them. Watching
//! this number stay put while pipelines come and go is what says the sharing
//! works.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t vertexReadersHeld();

// ----------------------------------------------------------------------------
//! \brief Forget what the device was last told, because someone else told it
//! something.
//!
//! Binding a pipeline sends only what changed since the previous one, which is
//! what makes many small draws affordable. That is a bet on nobody else touching
//! the context, and it is off the moment a library sharing the context issues
//! calls of its own: a user interface toolkit, a video player, a profiler overlay,
//! a piece of code from before this library existed.
//!
//! Called after such code has run, this makes the next pipeline send everything
//! again. Cheap, and once per frame it costs nothing measurable; the alternative
//! is a frame drawn with somebody else's blending still on.
// ----------------------------------------------------------------------------
void forgetRenderState();

// ----------------------------------------------------------------------------
//! \brief Draw every filled triangle as its edges, whatever the pipelines ask
//! for: a debugging view showing how a surface is cut into triangles.
//!
//! \code
//! compages::gpu::showWireframe(true);   // until turned off again
//! \endcode
// ----------------------------------------------------------------------------
void showWireframe(bool p_enabled);

//! \brief Is every triangle drawn as its edges?
[[nodiscard]] bool wireframeShown();

} // namespace compages::gpu
