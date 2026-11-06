// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief One complete way of drawing: a shader, the shape of a vertex, and the
//! drawing settings.
//!
//! To draw, the graphics card needs a shader (the program saying where each
//! corner goes and what colour each pixel is), the layout of the vertices it
//! reads, and the settings (depth, transparency...). A Pipeline is the three,
//! checked against each other when it is created.
//!
//! The layout is declared in C++, from the vertex struct, and the shader is
//! only asked to confirm that it can be fed by it. One VertexBuffer<Vertex>
//! holding position, normal and texture coordinates can thus feed a pass that
//! reads all three and a shadow pass that reads only the position
//! (04c_MultiPassMesh).
//!
//! The check is a real one: a layout offering "aPosition" to a shader declaring
//! "position" is an error naming both lists, and so are a vec2 field feeding a
//! vec3 attribute and whole numbers feeding a float.
//!
//! What it holds:
//! - Pipeline: the checked recipe.
//! - forgetRenderState(): resync after foreign code touched the context.
//! - showWireframe(): a debug view of every triangle edge.
//! - livePipelines(), vertexReadersHeld(): to spot leaks and check sharing.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/Handle.hpp"
#include "Compages/GPU/RenderState.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/VertexLayout.hpp"

#include <string>
#include <type_traits>

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

namespace compages::gpu
{

//! \brief Names one pipeline.
using PipelineHandle = Handle<struct PipelineTag>;

// ****************************************************************************
//! \brief One recipe for drawing: which shader, what a vertex looks like,
//! and with which settings, checked once against each other.
//!
//! Like a recipe card, it is written once and then followed every time: the
//! same pipeline draws a hundred meshes, as long as their vertices have the
//! shape it was written for. A mistake in the recipe, such as a shader asking
//! for a colour the vertices do not have, is found when the card is written,
//! not in the middle of a frame.
//!
//! Immutable on purpose. Everything that could be wrong about a way of drawing
//! is found out when the pipeline is created, which is at load time, where a
//! message can be read and acted on. Nothing is checked per draw, and nothing
//! can be changed behind a caller's back between two draws.
//!
//! \code
//! struct Vertex
//! {
//!     compages::Vector3f position;
//!     compages::Vector3f normal;
//!     compages::Vector2f uv;
//! };
//!
//! // The layout is read off the struct: position, normal and uv feed the
//! // shader attributes of the same names.
//! COMPAGES_TRY_ASSIGN(
//!     m_forward,
//!     compages::gpu::Pipeline::create<Vertex>(
//!         m_program,
//!         { .depth_test = true, .cull = compages::gpu::CullMode::Back }));
//! \endcode
//!
//! Most code never builds one: compages::gpu::Drawable makes the pipeline it
//! needs from its shader and its vertices. A Pipeline written by hand is for
//! sharing one program between several vertex layouts or render states.
//!
//! \note A pipeline does not own its program: it names it. The program has to
//! outlive it. Releasing the program first is caught rather than crashed on, as
//! a failure the next time the pipeline is used.
// ****************************************************************************
class Pipeline: private compages::NotCopiable
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty pipeline: valid() is false and drawing with it records
    //! a frame error.
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
    [[nodiscard]] static Result<Pipeline>
    create(Program const& p_program,
           VertexLayout const& p_layout,
           RenderState const& p_state = {});

    // ------------------------------------------------------------------------
    //! \brief The same thing, saying which C++ struct the layout describes.
    //!
    //! Worth preferring: it checks that the layout really is the layout of that
    //! struct, which catches the mistake of passing the layout of one vertex
    //! type while drawing from a buffer of another. That mistake reads the
    //! right number of bytes from the wrong places, so it draws something,
    //! which is why it is worth refusing here.
    //!
    //! \tparam Vertex the struct one element of the vertex buffer holds.
    //! \param[in] p_program the linked program, which must outlive the
    //! pipeline.
    //! \param[in] p_layout what one vertex looks like; its stride must be
    //! sizeof(Vertex).
    //! \param[in] p_state how to draw.
    //! \return the pipeline, or why not: a stride that is not sizeof(Vertex),
    //! or what create() refuses.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] static Result<Pipeline>
    create(Program const& p_program,
           VertexLayout const& p_layout,
           RenderState const& p_state = {})
    {
        static_assert(
            std::is_standard_layout_v<Vertex>,
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
                " bytes, but the type given is " +
                std::to_string(sizeof(Vertex)) +
                " bytes. Either the layout belongs to another vertex struct, "
                "or "
                "a field was added to the struct and not described");
        }
        return create(p_program, p_layout, p_state);
    }

    // ------------------------------------------------------------------------
    //! \brief Same, with the layout read off the struct by
    //! VertexLayout::of<Vertex>(): each field feeds the shader attribute of
    //! the same name. The shortest form, and the one to try first.
    //!
    //! \param[in] p_program the linked program, which must outlive the
    //! pipeline.
    //! \param[in] p_state how to draw.
    //! \return the pipeline, or what the shader wants that the struct does not
    //! offer.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] static Result<Pipeline>
    create(Program const& p_program, RenderState const& p_state = {})
    {
        return create<Vertex>(p_program, VertexLayout::of<Vertex>(), p_state);
    }

    //! \brief Take over the pipeline of p_other, which is left not valid().
    //! \param[in,out] p_other the pipeline to empty.
    Pipeline(Pipeline&& p_other) noexcept;
    //! \brief Release this pipeline, then take over p_other's.
    //! \param[in,out] p_other the pipeline to empty.
    //! \return this object, now owning the other pipeline.
    Pipeline& operator=(Pipeline&& p_other) noexcept;
    //! \brief Release the pipeline; the program stays.
    ~Pipeline();

    // ------------------------------------------------------------------------
    //! \brief Drop the pipeline now rather than at the end of the scope.
    //!
    //! Does not touch the program, which the pipeline never owned.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief False when default constructed, moved from or released. Stays
    //! true when the program is released; bind() is what notices that.
    //! \return true when the pipeline exists.
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
    //! \brief Which program this draws with: a handle, since the pipeline
    //! names the program without owning it.
    //! \return the program handle; empty when not valid().
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramHandle program() const;

    // ------------------------------------------------------------------------
    //! \brief What one vertex looks like, as it was checked. Fields the shader
    //! does not read are still listed here.
    //! \return the layout checked at creation.
    // ------------------------------------------------------------------------
    [[nodiscard]] VertexLayout const& layout() const;

    // ------------------------------------------------------------------------
    //! \brief The settings given at creation, unchanged: a pipeline never
    //! changes after it is made.
    //! \return the render state given at creation.
    // ------------------------------------------------------------------------
    [[nodiscard]] RenderState const& state() const;

    // ------------------------------------------------------------------------
    //! \brief Distance in bytes from one vertex to the next, which is the size
    //! of the struct the vertex buffer holds.
    //! \return the stride in bytes.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uint32_t stride() const;

    // ------------------------------------------------------------------------
    //! \brief Does anything here change once per instance rather than once per
    //! vertex? True when this pipeline is meant for instanced drawing.
    //! \return true when a field is read per instance.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool instanced() const;

    // ------------------------------------------------------------------------
    //! \brief Which fields of the layout the shader actually reads, and at
    //! which attribute slot the driver put each one.
    //!
    //! The result of the check, kept because it is also the answer to "why is
    //! my attribute not arriving": a field absent from this list is one the
    //! shader never declared, and is not being sent anywhere.
    //! \return a readable list, one line per attribute.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string describeAttributes() const;

    // ------------------------------------------------------------------------
    //! \brief The handle naming this pipeline, for code that keeps it without
    //! owning it.
    //! \return the handle; empty when not valid().
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
//! \brief How many pipelines are alive, including those Drawable made for
//! itself. Used to spot leaks.
//! \return the number of live pipelines.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t livePipelines();

// ----------------------------------------------------------------------------
//! \brief How many distinct ways of reading a vertex the device is holding.
//!
//! Pipelines that read a vertex the same way share one, so this is at most the
//! number of live pipelines and usually far less: the passes of
//! 04c_MultiPassMesh read the same vertex, so they cost one between them.
//! Watching this number stay put while pipelines come and go is what says the
//! sharing works.
//! \return the number of vertex readers held.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t vertexReadersHeld();

// ----------------------------------------------------------------------------
//! \brief Forget what the device was last told, because someone else told it
//! something.
//!
//! Binding a pipeline sends only what changed since the previous one, which is
//! what makes many small draws affordable. That is a bet on nobody else
//! touching the context, and it is off the moment a library sharing the context
//! issues calls of its own: a user interface toolkit, a video player, a
//! profiler overlay, a piece of code from before this library existed.
//!
//! Called after such code has run, this makes the next pipeline send everything
//! again. Cheap, and once per frame it costs nothing measurable; the
//! alternative is a frame drawn with somebody else's blending still on.
// ----------------------------------------------------------------------------
void forgetRenderState();

// ----------------------------------------------------------------------------
//! \brief Draw every filled triangle as its edges, whatever the pipelines ask
//! for: a debugging view showing how a surface is cut into triangles.
//!
//! \code
//! compages::gpu::showWireframe(true);   // until turned off again
//! \endcode
//!
//! \param[in] p_enabled true to outline every filled triangle, false to go
//! back to what each pipeline asks for. Lines and points are not affected.
// ----------------------------------------------------------------------------
void showWireframe(bool p_enabled);

//! \brief Is every triangle drawn as its edges? Read each time a pipeline is
//! bound.
//! \return true while showWireframe(true) is in effect.
[[nodiscard]] bool wireframeShown();

} // namespace compages::gpu
