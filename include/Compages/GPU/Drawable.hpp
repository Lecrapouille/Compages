// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Drawable: a shape, its look and how to draw it, in one object. What
//! most code draws with.
//!
//! The shader is where everything starts. It says which values each corner of
//! a shape has (position, colour...), which settings it reads and which
//! pictures it shows, so the rest is filled by those names. What changed on the
//! CPU is sent to the graphics card when drawing, without being asked. The idea
//! comes from Glumpy.
//!
//! \code
//! compages::gpu::Drawable triangle;
//! COMPAGES_TRY(triangle.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));
//! triangle["position"] = { {-0.8f, -0.6f}, {0.8f, -0.6f}, {0.0f, 0.8f} };
//! triangle["color"]    = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
//! triangle.draw();
//! \endcode
//!
//! Underneath it uses a Program, a Pipeline and Buffers; reach for those only
//! for what Drawable does not cover.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/DirtyRange.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/RenderState.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/GPU/VertexLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

namespace compages::gpu
{

namespace detail
{

//! \brief One address per type, to recognise the vertex struct a Drawable was
//! given when it is asked for it back.
template <typename T>
inline constexpr char TYPE_TAG = 0;

//! \brief The C++ types a uniform can be set from.
template <typename T>
concept UniformValue = requires(Program& p_program, T const& p_value) {
    p_program.set(std::string_view{}, p_value);
};

//! \brief A container of vertex values: a std::vector, a std::array, a span.
template <typename R>
concept AttributeValues =
    std::ranges::contiguous_range<R> && std::ranges::sized_range<R> &&
    !UniformValue<R> &&
    std::is_trivially_copyable_v<std::ranges::range_value_t<R>>;

} // namespace detail

// ****************************************************************************
//! \brief Something to draw, all in one object: its shape, its look and how
//! to draw it.
//!
//! A Drawable is the shortest path from an idea to a picture. Give it a shader
//! (the look), fill in its corners (the shape) by the names the shader uses,
//! and call draw(). It sends to the card only what changed, and builds the
//! Pipeline it needs by itself.
//!
//! **Filling the vertices by name.** The shader says what a vertex is, so the
//! values are given attribute by attribute. They are stored interleaved,
//! one vertex after the other, which is what the GPU reads fastest:
//! \code
//! compages::gpu::Drawable m_triangle;
//!
//! COMPAGES_TRY(m_triangle.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));
//! m_triangle["position"] = { {-0.8f, -0.6f}, {0.8f, -0.6f}, {0.0f, 0.8f} };
//! m_triangle["color"]    = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
//! ...
//! m_triangle.draw();
//! \endcode
//!
//! **Filling the vertices from a struct.** When the vertices are computed, a
//! struct is the natural shape. Its fields are matched to the shader
//! attributes by name, read off the struct by the compiler. The drawable
//! keeps them on the CPU and sends only what changed:
//! \code
//! struct Vertex
//! {
//!     compages::Vector2f position;
//!     compages::Vector3f color;
//! };
//!
//! m_triangle.setVertices<Vertex>({ { {-0.8f, -0.6f}, {1, 0, 0} },
//!                                  { { 0.8f, -0.6f}, {0, 1, 0} },
//!                                  { { 0.0f,  0.8f}, {0, 0, 1} } });
//! m_triangle.editVertex<Vertex>(2u).position.y = 0.5f; // sent at next draw
//! \endcode
//!
//! **Reading vertices that live on the card.** When a compute pass makes the
//! vertices, they are already where the card reads them. The drawable reads
//! that Buffer where it is, without a copy:
//! \code
//! m_points.borrowVertices(m_particles);
//! \endcode
//!
//! One rule: the drawable owns its vertices; borrow a Buffer when the
//! vertices are made on the card, shared by several drawables, or large and
//! never changed (a Static Buffer keeps no CPU copy).
//! - from a struct: setVertices(data), editVertex(), emplace_back();
//! - one attribute at a time: drawable["name"] = { ... };
//! - borrowed: borrowVertices(buffer), the Buffer you own.
//! The same table, with demos, is in doc/04-GPU-Layer.md §3.
//!
//! **Uniforms and textures, by name too.** A uniform is written immediately
//! and keeps its value until written again. A texture is given a texture unit
//! of its own and bound when drawing:
//! \code
//! m_quad["scale"] = 2.0f;
//! m_quad["tint"]  = compages::Vector3f(1.0f, 0.8f, 0.6f);
//! m_quad["image"] = m_texture;   // the texture must outlive the drawable
//! \endcode
//!
//! **How to draw.** state() is the render state (depth test, blending,
//! culling, primitive); the pipeline is rebuilt when it changes, and only then:
//! \code
//! m_cube.state().depth_test = true;
//! m_cube.indices({ 0, 1, 2, 2, 3, 0 });
//! m_cube.draw();
//! \endcode
//!
//! Nothing here returns an error except loading: a misspelled name, values of
//! the wrong type or a draw with nothing to draw are recorded as the frame
//! error (see Errors.hpp), which the window shows. Call prepare() at the end
//! of a set up to find them there rather than at the first frame.
// ****************************************************************************
class Drawable: private compages::NotCopiable
{
public:

    // ************************************************************************
    //! \brief What `drawable["name"]` returns: the attribute, uniform or
    //! sampler of that name, waiting to be assigned.
    //!
    //! Which of the three it is comes from the shader, so the same syntax
    //! fills vertices, sets uniforms and attaches textures.
    // ************************************************************************
    class Slot
    {
    public:

        // --------------------------------------------------------------------
        //! \brief Vector values, one per vertex, for an attribute; or the
        //! components of one vector for a uniform.
        //!
        //! \code
        //! drawable["position"] = { {-1, -1}, {1, -1}, {0, 1} };   // vec2
        //! attribute
        //! \endcode
        //!
        //! Whole numbers are welcome for a float attribute: the list is
        //! converted to what the shader declares. (No overload for lists of
        //! int: clang finds it as good as this one, so ambiguous.)
        //!
        //! \param[in] p_values one inner list per vertex, or a single inner
        //! list for a uniform.
        //! \return this slot, so assignments can be chained.
        // --------------------------------------------------------------------
        Slot&
        operator=(std::initializer_list<std::initializer_list<float>> p_values)
        {
            m_drawable.assignNested(m_name, p_values);
            return *this;
        }

        // --------------------------------------------------------------------
        //! \brief One number per vertex for a float attribute, or the
        //! components of a vector uniform.
        //!
        //! \code
        //! drawable["weight"] = { 0.0f, 0.5f, 1.0f };   // float attribute
        //! drawable["tint"]   = { 1.0f, 0.5f, 0.0f };   // vec3 uniform
        //! \endcode
        //!
        //! \param[in] p_values one number per vertex, or the components of
        //! the uniform.
        //! \return this slot.
        // --------------------------------------------------------------------
        Slot& operator=(std::initializer_list<float> p_values)
        {
            m_drawable.assignFlat(m_name, p_values);
            return *this;
        }

        // --------------------------------------------------------------------
        //! \brief Values from a container, one per vertex, of exactly the type
        //! the shader declares.
        //!
        //! \code
        //! std::vector<compages::Vector3f> positions =
        //! computePositions(); drawable["position"] = positions;
        //! \endcode
        //!
        //! \param[in] p_values a std::vector, std::array or span, copied; its
        //! element type must match the attribute (Vector3f for a vec3).
        //! \return this slot.
        // --------------------------------------------------------------------
        template <detail::AttributeValues R>
        Slot& operator=(R const& p_values)
        {
            using Element = std::remove_cv_t<std::ranges::range_value_t<R>>;
            m_drawable.assignTyped(
                m_name,
                TypeOf<Element>::value,
                std::as_bytes(std::span<const Element>(
                    std::ranges::data(p_values), std::ranges::size(p_values))));
            return *this;
        }

        // --------------------------------------------------------------------
        //! \brief The value of a uniform.
        //!
        //! \code
        //! drawable["time"]  = p_frame.total;
        //! drawable["model"] =
        //! compages::Matrix44f(compages::matrix::Identity);
        //! \endcode
        //!
        //! Written into the program at once, not at the next draw.
        //!
        //! \param[in] p_value any type Program::set() takes.
        //! \return this slot.
        // --------------------------------------------------------------------
        template <detail::UniformValue T>
        Slot& operator=(T const& p_value)
        {
            if (m_drawable.expectUniform(m_name))
            {
                m_drawable.m_program.set(m_name, p_value);
            }
            return *this;
        }

        //! \brief A double is taken as the float the shader holds, so that
        //! `drawable["scale"] = 2.0;` works as written.
        //! \param[in] p_value the value, narrowed to float.
        //! \return this slot.
        Slot& operator=(double p_value)
        {
            return operator=(static_cast<float>(p_value));
        }

        // --------------------------------------------------------------------
        //! \brief The texture a sampler reads.
        //!
        //! The drawable keeps a pointer to it, not a copy: the texture has to
        //! outlive the drawable, or be assigned again before the next draw.
        //! Each sampler gets its own texture unit, bound at every draw.
        //!
        //! \param[in] p_texture the texture the sampler reads.
        //! \return this slot.
        // --------------------------------------------------------------------
        Slot& operator=(Texture const& p_texture)
        {
            m_drawable.assignTexture(m_name, p_texture);
            return *this;
        }

        // --------------------------------------------------------------------
        //! \brief Change the value of one vertex of an attribute.
        //!
        //! \code
        //! drawable["position"].set(2u, compages::Vector2f(0.0f, 0.5f));
        //! \endcode
        //!
        //! Only that vertex is sent at the next draw.
        //!
        //! \param[in] p_vertex which vertex, which must already exist.
        //! \param[in] p_value the value, of the type the attribute declares.
        // --------------------------------------------------------------------
        template <typename T>
        void set(std::size_t p_vertex, T const& p_value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            m_drawable.assignOne(
                m_name,
                TypeOf<T>::value,
                p_vertex,
                std::as_bytes(std::span<const T>(&p_value, 1u)));
        }

    private:

        friend class Drawable;

        Slot(Drawable& p_drawable, std::string_view p_name)
            : m_drawable(p_drawable), m_name(p_name)
        {
        }

        Drawable& m_drawable;
        std::string m_name;
    };

    //! \brief Nothing to draw yet: loadSources() or loadFiles() first.
    Drawable() = default;
    //! \brief Take over everything of another drawable, left empty.
    Drawable(Drawable&&) noexcept = default;
    //! \brief Release everything of this drawable, then take over another's.
    //! \return this drawable, now owning what the other owned.
    Drawable& operator=(Drawable&&) noexcept = default;

    // ------------------------------------------------------------------------
    //! \brief Compile and link the shader this drawable draws with.
    //!
    //! Forgets any vertices, indices and textures given before, since the new
    //! shader may want other ones. The render state is kept.
    //!
    //! \param[in] p_vertex the GLSL of the vertex stage, from its \#version
    //! line.
    //! \param[in] p_fragment the GLSL of the fragment stage.
    //! \return the compiler or linker log when it failed.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadSources(std::string_view p_vertex,
                                     std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Same as loadSources(), reading the two stages from files.
    //!
    //! \param[in] p_vertex path of the vertex stage, usually a .vert file.
    //! \param[in] p_fragment path of the fragment stage, usually a .frag file.
    //! \return the compiler or linker log, or why a file could not be read.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadFiles(std::filesystem::path const& p_vertex,
                                   std::filesystem::path const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief The attribute, uniform or sampler called p_name.
    //!
    //! See Slot for what can be assigned to it. A name the shader does not
    //! declare is reported when something is assigned, not here.
    //!
    //! \param[in] p_name the name in GLSL.
    //! \return the slot, valid only for the statement that uses it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Slot operator[](std::string_view p_name)
    {
        return Slot(*this, p_name);
    }

    // ------------------------------------------------------------------------
    //! \brief Copy the vertices into the drawable, replacing any it had.
    //!
    //! The drawable keeps them on the CPU and sends what changes at the next
    //! draw (see editVertex()). For large data the CPU never touches again,
    //! keep it in a Static Buffer of your own and borrowVertices() it.
    //!
    //! \code
    //! m_cube.setVertices(corners);   // a std::vector, std::array or span
    //! m_triangle.setVertices<Vertex>({ { ... }, { ... }, { ... } });
    //! \endcode
    //!
    //! \param[in] p_vertices the vertices, copied: any contiguous container
    //! of a trivially copyable struct.
    //! \param[in] p_layout how the fields feed the shader. Read off the struct
    //! by default; give one to rename a field or read it per instance:
    //! \code
    //! m_sprites.setVertices(
    //!     sprites, compages::gpu::VertexLayout::of<Sprite>().perInstance());
    //! \endcode
    // ------------------------------------------------------------------------
    template <std::ranges::contiguous_range Range,
              typename Vertex = std::ranges::range_value_t<Range>>
    void setVertices(Range const& p_vertices,
                     VertexLayout p_layout = VertexLayout::of<Vertex>())
    {
        static_assert(std::is_trivially_copyable_v<Vertex>,
                      "a vertex is sent to the GPU as the bytes it already is, "
                      "so it must be trivially copyable");
        const std::span<const Vertex> vertices(std::ranges::data(p_vertices),
                                               std::ranges::size(p_vertices));
        adopt(std::move(p_layout), &detail::TYPE_TAG<Vertex>, sizeof(Vertex));
        replaceBytes(std::as_bytes(vertices), vertices.size());
    }

    //! \brief Same, from braces: `setVertices<Vertex>({ {...}, {...} })`.
    //! \param[in] p_vertices the vertices, copied.
    //! \param[in] p_layout how the fields feed the shader.
    template <typename Vertex>
    void setVertices(std::initializer_list<Vertex> p_vertices,
                     VertexLayout p_layout = VertexLayout::of<Vertex>())
    {
        setVertices(
            std::span<const Vertex>(p_vertices.begin(), p_vertices.size()),
            std::move(p_layout));
    }

    // ------------------------------------------------------------------------
    //! \brief Draw vertices that only live on the GPU, such as a buffer a
    //! compute shader writes. Nothing is copied: the drawable reads the
    //! buffer where it is.
    //!
    //! \code
    //! COMPAGES_TRY(m_step.dispatchItems(COUNT));
    //! compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);
    //! m_points.borrowVertices(m_particles);
    //! m_points.draw();
    //! \endcode
    //!
    //! Borrowed, not owned: the buffer has to outlive the drawable, or be
    //! given again before the next draw. Giving it every frame costs nothing,
    //! which is how the current half of a PingPong is drawn. The vertices
    //! cannot be edited through editVertices(): they are not on the CPU.
    //!
    //! \param[in] p_buffer the buffer to read, not copied.
    //! \param[in] p_layout how the fields feed the shader.
    // ------------------------------------------------------------------------
    template <typename Vertex, BufferKind Kind>
        requires VertexSource<Kind>
    void borrowVertices(Buffer<Vertex, Kind> const& p_buffer,
                        VertexLayout p_layout = VertexLayout::of<Vertex>())
    {
        adopt(std::move(p_layout), &detail::TYPE_TAG<Vertex>, sizeof(Vertex));
        borrow(p_buffer.handle(), p_buffer.count());
    }

    // ------------------------------------------------------------------------
    //! \brief Every vertex, to be changed in place. All of them are sent at
    //! the next draw.
    //!
    //! \code
    //! for (Vertex& v : m_wave.editVertices<Vertex>())
    //! {
    //!     v.position.y = std::sin(v.position.x + time);
    //! }
    //! \endcode
    //!
    //! \tparam Vertex the struct given to setVertices(). Asking for another
    //! one is a programming error and asserts. So is editing borrowed
    //! vertices: they are not on the CPU.
    //! \return every vertex, to change in place.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] std::span<Vertex> editVertices()
    {
        assertHolds<Vertex>();
        assertEditable();
        m_dirty.addAll(m_bytes.size());
        // The storage is a std::vector<std::byte>, whose memory comes from
        // operator new and is therefore aligned for any vertex field. The cast
        // goes through void* to say so to -Wcast-align.
        return { static_cast<Vertex*>(static_cast<void*>(m_bytes.data())),
                 m_count };
    }

    // ------------------------------------------------------------------------
    //! \brief A run of neighbouring vertices, to be changed in place. Only
    //! that run is sent at the next draw.
    //!
    //! \code
    //! for (Vertex& v : m_wave.editVertices<Vertex>(10u, 5u))   // 10 to 14
    //! {
    //!     v.position.y += 0.1f;
    //! }
    //! \endcode
    //!
    //! \tparam Vertex the struct given to setVertices().
    //! \param[in] p_first the first vertex of the run.
    //! \param[in] p_count how many; past the last vertex a debug build stops.
    //! \return the run, to change in place.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] std::span<Vertex> editVertices(std::size_t p_first,
                                                 std::size_t p_count)
    {
        assertHolds<Vertex>();
        assertEditable();
        assert((p_first + p_count <= m_count) &&
               "editVertices() past the last vertex");
        m_dirty.add(p_first * sizeof(Vertex), p_count * sizeof(Vertex));
        return { static_cast<Vertex*>(static_cast<void*>(m_bytes.data())) +
                     p_first,
                 p_count };
    }

    // ------------------------------------------------------------------------
    //! \brief One vertex, to be changed in place. Only it is sent at the next
    //! draw.
    //!
    //! \code
    //! m_triangle.editVertex<Vertex>(2u).position = mouse;
    //! \endcode
    //!
    //! \tparam Vertex the struct given to setVertices().
    //! \param[in] p_index which vertex; past the last one a debug build stops.
    //! \return the vertex, to change in place.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] Vertex& editVertex(std::size_t p_index)
    {
        assertHolds<Vertex>();
        assertEditable();
        assert((p_index < m_count) && "editVertex() past the last vertex");
        m_dirty.add(p_index * sizeof(Vertex), sizeof(Vertex));
        return static_cast<Vertex*>(
            static_cast<void*>(m_bytes.data()))[p_index];
    }

    // ------------------------------------------------------------------------
    //! \brief Add one vertex at the end.
    //!
    //! \code
    //! m_trail.emplace_back(Vertex{ point, color });
    //! \endcode
    //!
    //! The first vertex added to an empty drawable fixes the struct and its
    //! layout. Sent at the next draw, with the vertices added since.
    //!
    //! \param[in] p_vertex the vertex, copied.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    void emplace_back(Vertex const& p_vertex)
    {
        if (m_type == nullptr)
        {
            adopt(VertexLayout::of<Vertex>(),
                  &detail::TYPE_TAG<Vertex>,
                  sizeof(Vertex));
        }
        assertHolds<Vertex>();
        appendBytes(std::as_bytes(std::span<const Vertex>(&p_vertex, 1u)));
    }

    // ------------------------------------------------------------------------
    //! \brief How many vertices there are, on the CPU or in the borrowed
    //! buffer.
    //! \return the number of vertices.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t vertexCount() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Remove every vertex, keeping the layout, so that the same
    //! drawable can be refilled each frame.
    // ------------------------------------------------------------------------
    void clear();

    // ------------------------------------------------------------------------
    //! \brief Draw through these indices rather than the vertices in order.
    //!
    //! \code
    //! m_quad.indices({ 0, 1, 2, 2, 3, 0 });
    //! \endcode
    //!
    //! A square needs four corners and six indices rather than six corners:
    //! shared corners are stored once.
    //!
    //! \param[in] p_indices which vertices, in which order. Sent at the next
    //! draw.
    // ------------------------------------------------------------------------
    void indices(std::initializer_list<std::uint32_t> p_indices)
    {
        replaceIndices(std::span<const std::uint32_t>(p_indices.begin(),
                                                      p_indices.size()));
    }

    //! \brief Same, from any contiguous container of std::uint16_t (as mesh
    //! generators often produce, widened on the way) or std::uint32_t.
    //! \param[in] p_indices which vertices, in which order, copied.
    template <std::ranges::contiguous_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>,
                              std::uint16_t> ||
                 std::same_as<std::ranges::range_value_t<Range>, std::uint32_t>
    void indices(Range const& p_indices)
    {
        using Index = std::ranges::range_value_t<Range>;
        replaceIndices(std::span<const Index>(std::ranges::data(p_indices),
                                              std::ranges::size(p_indices)));
    }

    // ------------------------------------------------------------------------
    //! \brief How to draw: primitive, depth test, blending, culling...
    //!
    //! \code
    //! m_cube.state().depth_test = true;
    //! m_cube.state().cull = compages::gpu::CullMode::Back;
    //! m_glass.state().blend = compages::gpu::Blend::alpha();
    //! m_cloud.state().primitive = compages::gpu::Primitive::Points;
    //! \endcode
    //!
    //! Change it freely; the pipeline is rebuilt at the next draw when it
    //! differs from the one in use.
    //! \return the render state, to change.
    // ------------------------------------------------------------------------
    [[nodiscard]] RenderState& state()
    {
        return m_state;
    }

    //! \brief How to draw, read only.
    //! \return the render state, read only.
    [[nodiscard]] RenderState const& state() const
    {
        return m_state;
    }

    // ------------------------------------------------------------------------
    //! \brief Draw everything: every index if there are indices, every vertex
    //! otherwise.
    // ------------------------------------------------------------------------
    void draw();

    // ------------------------------------------------------------------------
    //! \brief Draw p_count vertices (or indices) starting at p_first: one part
    //! of a buffer holding several shapes.
    //!
    //! Also how a shader making its own vertices from gl_VertexID is drawn:
    //! `m_screen.draw(3u)` for a triangle covering the screen.
    //!
    //! \param[in] p_count how many vertices, or indices when there are some.
    //! \param[in] p_first the first vertex, or index.
    // ------------------------------------------------------------------------
    void draw(std::size_t p_count, std::size_t p_first = 0u);

    // ------------------------------------------------------------------------
    //! \brief Draw the vertices several times, once per instance.
    //!
    //! \param[in] p_instances how many copies.
    //! \param[in] p_vertices how many vertices make one copy. Zero means all
    //! of them. With a layout read per instance, each record is one instance
    //! and this is the number of corners the shader makes from gl_VertexID:
    //! \code
    //! m_sprites.drawInstanced(m_sprites.vertexCount(), 4u);
    //! \endcode
    // ------------------------------------------------------------------------
    void drawInstanced(std::size_t p_instances, std::size_t p_vertices = 0u);

    // ------------------------------------------------------------------------
    //! \brief Draw as many vertices as a command sitting on the GPU says.
    //!
    //! A compute pass that decides what survives writes the command; the CPU
    //! never learns the count:
    //! \code
    //! compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib |
    //!                        compages::gpu::Barrier::Command);
    //! m_kept_points.drawIndirect(m_command);
    //! \endcode
    //!
    //! \param[in] p_commands a buffer holding one DrawIndirectCommand.
    // ------------------------------------------------------------------------
    void drawIndirect(StorageBuffer<DrawIndirectCommand> const& p_commands);

    // ------------------------------------------------------------------------
    //! \brief Send everything waiting and build the pipeline now.
    //!
    //! Draws nothing. At the end of a set up, it turns the mistakes the first
    //! frame would have found into an error returned there:
    //! \code
    //! COMPAGES_TRY(m_triangle.prepare());
    //! \endcode
    //! \return the first mistake found: a misspelled name, values of the wrong
    //! type, a shader and vertices that disagree.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status prepare();

    // ------------------------------------------------------------------------
    //! \brief The program, for what this class does not cover, such as a
    //! uniform array or binding a uniform block.
    //! \return the program, to change.
    // ------------------------------------------------------------------------
    [[nodiscard]] Program& program()
    {
        return m_program;
    }

    //! \brief The program, read only: to ask what the shader declares.
    //! \return the program, read only.
    [[nodiscard]] Program const& program() const
    {
        return m_program;
    }

    // ------------------------------------------------------------------------
    //! \brief Which field feeds which attribute, and how, once prepared. What
    //! to print when a shader and its vertices disagree.
    //! \return a readable dump, several lines.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string describe() const;

private:

    //! \brief A texture a sampler reads, and the unit it was given.
    struct Sampler
    {
        std::string name;
        Texture const* texture = nullptr;
        std::uint32_t unit = 0u;
    };

    //! \brief How many values a named attribute has received.
    struct Filled
    {
        std::string name;
        std::size_t count = 0u;
    };

    //! \brief Debug check: Vertex is the struct the vertices were given as.
    template <typename Vertex>
    void assertHolds() const
    {
        assert((m_type == &detail::TYPE_TAG<Vertex>) &&
               "this drawable was given vertices of another struct");
    }

    //! \brief Debug check: the vertices are on the CPU, so editing them
    //! means something.
    void assertEditable() const
    {
        assert(!m_borrowing && "these vertices are borrowed: they live on "
                               "the GPU only");
    }

    //! \brief Take a freshly loaded shader and forget everything that came
    //! with the previous one (vertices, indices, textures).
    void adoptProgram(Program p_program);

    // Filling from a struct.
    //! \brief Remember the struct and its layout; rebuild the pipeline only
    //! when they differ from the previous ones.
    void adopt(VertexLayout p_layout, const void* p_type, std::size_t p_stride);
    //! \brief Copy all the vertices, as bytes, and mark them all to send.
    void replaceBytes(std::span<const std::byte> p_bytes, std::size_t p_count);
    //! \brief Add one vertex, as bytes, and mark it to send.
    void appendBytes(std::span<const std::byte> p_bytes);
    //! \brief Read the vertices from a buffer someone else owns.
    void borrow(BufferHandle p_buffer, std::size_t p_count);
    //! \brief Copy the indices, widened to 32 bits, and mark them to send.
    void replaceIndices(std::span<const std::uint32_t> p_indices);
    //! \brief Same, from 16 bit indices.
    void replaceIndices(std::span<const std::uint16_t> p_indices);
    //! \brief The buffer the vertices are read from: ours, or the one borrowed.
    [[nodiscard]] BufferHandle vertexBuffer() const;

    // Filling by name.
    template <typename Scalar>
    void
    assignNested(std::string const& p_name,
                 std::initializer_list<std::initializer_list<Scalar>> p_values);
    void assignFlat(std::string const& p_name,
                    std::initializer_list<float> p_values);
    void assignTyped(std::string const& p_name,
                     DataType p_type,
                     std::span<const std::byte> p_values);
    void assignOne(std::string const& p_name,
                   DataType p_type,
                   std::size_t p_vertex,
                   std::span<const std::byte> p_value);
    void assignTexture(std::string const& p_name, Texture const& p_texture);
    //! \brief Is p_name a uniform of the shader? Records why not.
    [[nodiscard]] bool expectUniform(std::string const& p_name);

    //! \brief The field an attribute is written to, creating the named layout
    //! on first use, making room for p_count vertices. Records why not.
    [[nodiscard]] FieldDesc const* attributeField(std::string const& p_name,
                                                  std::size_t p_count);
    //! \brief Write one converted value, taken from doubles, into a field.
    void writeValue(FieldDesc const& p_field,
                    std::size_t p_vertex,
                    std::span<const double> p_components);
    //! \brief Named mode: remember that p_name received p_count values.
    void markFilled(std::string const& p_name, std::size_t p_count);

    // Drawing.
    //! \brief Everything a draw needs: attributes complete, pipeline built,
    //! vertices and indices sent.
    [[nodiscard]] Status ready();
    //! \brief Send what changed in m_bytes, creating or growing m_device.
    [[nodiscard]] Status sendVertices();
    //! \brief Send m_indices when they changed.
    [[nodiscard]] Status sendIndices();
    //! \brief Bind each texture to the unit its sampler was given.
    void bindTextures() const;

    // The shader and how to draw with it.
    //! \brief The shader, loaded by loadSources() or loadFiles().
    Program m_program;
    //! \brief How m_program reads the vertices, with m_state. Built at the
    //! first draw, rebuilt when m_rebuild is set or m_state changes.
    Pipeline m_pipeline;
    //! \brief Depth test, blending, culling, primitive: see state().
    RenderState m_state;
    //! \brief The layout changed since m_pipeline was built.
    bool m_rebuild = true;

    // The vertices, on the CPU.
    //! \brief Where each attribute sits inside one vertex.
    VertexLayout m_layout;
    //! \brief The vertices, interleaved, as m_layout describes them. Empty
    //! when borrowed.
    std::vector<std::byte> m_bytes;
    //! \brief How many vertices there are, even when m_bytes is empty.
    std::size_t m_count = 0u;
    //! \brief In bytes: the part of m_bytes the card has not received yet.
    DirtyRange m_dirty;
    //! \brief The struct given to setVertices(), or nullptr in named mode.
    const void* m_type = nullptr;
    //! \brief Named mode: how many values each attribute received, to check
    //! at the draw that they all have one per vertex.
    std::vector<Filled> m_filled;

    // The vertices, on the card.
    //! \brief Our copy of m_bytes on the card. Unused when borrowing.
    VertexBuffer<std::byte> m_device;
    //! \brief The buffer read instead of m_device, when m_borrowing.
    BufferHandle m_borrowed{};
    //! \brief The vertices come from borrowVertices(): m_bytes is empty and
    //! m_device unused.
    bool m_borrowing = false;

    // The indices.
    //! \brief The order in which to join the vertices; empty to draw them in
    //! order.
    std::vector<std::uint32_t> m_indices;
    //! \brief m_indices changed since m_index_device received them.
    bool m_indices_changed = false;
    //! \brief m_indices on the card.
    IndexBuffer<std::uint32_t> m_index_device;

    // The textures.
    //! \brief One per sampler of the shader: its texture and its unit.
    std::vector<Sampler> m_samplers;
};

} // namespace compages::gpu
