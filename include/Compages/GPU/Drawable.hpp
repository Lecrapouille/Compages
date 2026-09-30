// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Core/DirtyRange.hpp"
#include "Compages/GPU/Core/Layout.hpp"
#include "Compages/GPU/Core/RenderState.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ****************************************************************************
//! \file
//! \brief A shader with its data: the one object most code draws with.
//!
//! The idea comes from Glumpy: the shader is where everything starts. It says
//! which attributes a vertex has, which uniforms it reads and which textures
//! it samples, so the rest is filled by name, and what changed on the CPU is
//! sent to the GPU when drawing, without being asked.
// ****************************************************************************

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
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
//! \brief A shader program, the vertices it draws, the uniforms and textures
//! it reads, and how to draw them.
//!
//! **Filling the vertices by name.** The shader says what a vertex is, so the
//! values are given attribute by attribute. They are stored interleaved,
//! one vertex after the other, which is what the GPU reads fastest:
//! \code
//! compages::gpu::Drawable m_triangle;
//!
//! COMPAGES_TRY(m_triangle.load(VERTEX_SHADER, FRAGMENT_SHADER));
//! m_triangle["position"] = { {-0.8f, -0.6f}, {0.8f, -0.6f}, {0.0f, 0.8f} };
//! m_triangle["color"]    = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
//! ...
//! m_triangle.draw();
//! \endcode
//!
//! **Filling the vertices from a struct.** When the vertices are computed, a
//! struct is the natural shape. Its fields are matched to the shader
//! attributes by name, read off the struct by the compiler:
//! \code
//! struct Vertex { compages::core::Vector2f position; compages::core::Vector3f
//! color; };
//!
//! m_triangle.vertices<Vertex>({ { {-0.8f, -0.6f}, {1, 0, 0} },
//!                               { { 0.8f, -0.6f}, {0, 1, 0} },
//!                               { { 0.0f,  0.8f}, {0, 0, 1} } });
//! m_triangle.vertex<Vertex>(2u).position.y = 0.5f;   // sent at next draw
//! \endcode
//!
//! **Uniforms and textures, by name too.** A uniform is written immediately
//! and keeps its value until written again. A texture is given a texture unit
//! of its own and bound when drawing:
//! \code
//! m_quad["scale"] = 2.0f;
//! m_quad["tint"]  = compages::core::Vector3f(1.0f, 0.8f, 0.6f);
//! m_quad["image"] = m_texture;   // the texture must outlive the drawable
//! \endcode
//!
//! **How to draw.** state() is the render state (depth test, blending,
//! culling, primitive); the pipeline is rebuilt when it changes, and only then:
//! \code
//! m_cube.state().depth_test = true;
//! m_cube.indices({ 0, 1, 2, 2, 3, 0 });
//! m_cube.draw();                         // or
//! draw(compages::gpu::Primitive::Lines)
//! \endcode
//!
//! Nothing here returns an error except load(): a misspelled name, values of
//! the wrong type or a draw with nothing to draw are recorded as the frame
//! error (see Errors.hpp), which the window shows. Call prepare() at the end
//! of a set up to find them there rather than at the first frame.
// ****************************************************************************
class Drawable
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
        //! converted to what the shader declares.
        // --------------------------------------------------------------------
        Slot&
        operator=(std::initializer_list<std::initializer_list<float>> p_values)
        {
            m_drawable.assignNested(m_name, p_values);
            return *this;
        }

        //! \brief Same, when every value is written as a whole number.
        Slot&
        operator=(std::initializer_list<std::initializer_list<int>> p_values)
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
        //! std::vector<compages::core::Vector3f> positions =
        //! computePositions(); drawable["position"] = positions;
        //! \endcode
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
        //! compages::core::Matrix44f(compages::core::matrix::Identity);
        //! \endcode
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
        Slot& operator=(double p_value)
        {
            return operator=(static_cast<float>(p_value));
        }

        // --------------------------------------------------------------------
        //! \brief The texture a sampler reads.
        //!
        //! The drawable keeps a pointer to it, not a copy: the texture has to
        //! outlive the drawable, or be assigned again before the next draw.
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
        //! drawable["position"].set(2u, compages::core::Vector2f(0.0f, 0.5f));
        //! \endcode
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

    Drawable() = default;
    Drawable(Drawable&&) noexcept = default;
    Drawable& operator=(Drawable&&) noexcept = default;
    Drawable(Drawable const&) = delete;
    Drawable& operator=(Drawable const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Compile and link the shader this drawable draws with.
    //!
    //! Forgets any vertices, indices and textures given before, since the new
    //! shader may want other ones. The render state is kept.
    //!
    //! \return the compiler or linker log when it failed.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status load(std::string_view p_vertex,
                              std::string_view p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Same as load(), reading the two stages from files.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadFiles(std::string const& p_vertex,
                                   std::string const& p_fragment);

    // ------------------------------------------------------------------------
    //! \brief Load, then give the vertices, in one call.
    //!
    //! \code
    //! COMPAGES_TRY(m_points.load(VERTEX, FRAGMENT, makeCloud()));
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] Status load(std::string_view p_vertex,
                              std::string_view p_fragment,
                              std::span<const Vertex> p_vertices)
    {
        COMPAGES_TRY(load(p_vertex, p_fragment));
        vertices<Vertex>(p_vertices);
        return success();
    }

    //! \brief Same, taking the vertices from a std::vector.
    template <typename Vertex>
    [[nodiscard]] Status load(std::string_view p_vertex,
                              std::string_view p_fragment,
                              std::vector<Vertex> const& p_vertices)
    {
        return load(p_vertex, p_fragment, std::span<const Vertex>(p_vertices));
    }

    // ------------------------------------------------------------------------
    //! \brief The attribute, uniform or sampler called p_name.
    //!
    //! See Slot for what can be assigned to it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Slot operator[](std::string_view p_name)
    {
        return Slot(*this, p_name);
    }

    // ------------------------------------------------------------------------
    //! \brief Replace every vertex with the elements of a struct.
    //!
    //! \param[in] p_vertices the vertices, copied.
    //! \param[in] p_layout how the fields feed the shader. Read off the struct
    //! by default; give one to rename a field or read it per instance:
    //! \code
    //! m_sprites.vertices<Sprite>(sprites,
    //!                            compages::gpu::VertexLayout::of<Sprite>().perInstance());
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename Vertex>
    void vertices(std::span<const Vertex> p_vertices,
                  VertexLayout p_layout = VertexLayout::of<Vertex>())
    {
        static_assert(std::is_trivially_copyable_v<Vertex>,
                      "a vertex is sent to the GPU as the bytes it already is, "
                      "so it must be trivially copyable");
        adopt(std::move(p_layout), &detail::TYPE_TAG<Vertex>, sizeof(Vertex));
        replaceBytes(std::as_bytes(p_vertices), p_vertices.size());
    }

    //! \brief Same, from braces: `vertices<Vertex>({ {...}, {...} })`.
    template <typename Vertex>
    void vertices(std::initializer_list<Vertex> p_vertices,
                  VertexLayout p_layout = VertexLayout::of<Vertex>())
    {
        vertices<Vertex>(
            std::span<const Vertex>(p_vertices.begin(), p_vertices.size()),
            std::move(p_layout));
    }

    //! \brief Same, from a std::vector.
    template <typename Vertex>
    void vertices(std::vector<Vertex> const& p_vertices,
                  VertexLayout p_layout = VertexLayout::of<Vertex>())
    {
        vertices<Vertex>(std::span<const Vertex>(p_vertices),
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
    //! m_points.vertices(m_particles);
    //! m_points.draw();
    //! \endcode
    //!
    //! The buffer has to outlive the drawable, or be given again before the
    //! next draw. Giving it every frame costs nothing, which is how the
    //! current half of a PingPong is drawn. The vertices cannot be read back
    //! through vertices<Vertex>(): they are not on the CPU.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    void vertices(Buffer<Vertex> const& p_buffer,
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
    //! for (Vertex& v : m_wave.vertices<Vertex>())
    //! {
    //!     v.position.y = std::sin(v.position.x + time);
    //! }
    //! \endcode
    //!
    //! \tparam Vertex the struct given to vertices(). Asking for another one is
    //! a programming error and asserts.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] std::span<Vertex> vertices()
    {
        assertHolds<Vertex>();
        assert(!m_borrowing && "these vertices live on the GPU only");
        m_dirty.addAll(m_bytes.size());
        // The storage is a std::vector<std::byte>, whose memory comes from
        // operator new and is therefore aligned for any vertex field.
        return { reinterpret_cast<Vertex*>(m_bytes.data()), m_count };
    }

    // ------------------------------------------------------------------------
    //! \brief One vertex, to be changed in place. Only it is sent at the next
    //! draw.
    // ------------------------------------------------------------------------
    template <typename Vertex>
    [[nodiscard]] Vertex& vertex(std::size_t p_index)
    {
        assertHolds<Vertex>();
        assert(!m_borrowing && "these vertices live on the GPU only");
        assert((p_index < m_count) && "vertex() past the last vertex");
        m_dirty.add(p_index * sizeof(Vertex), sizeof(Vertex));
        return reinterpret_cast<Vertex*>(m_bytes.data())[p_index];
    }

    // ------------------------------------------------------------------------
    //! \brief Add one vertex at the end.
    //!
    //! \code
    //! m_trail.emplace_back(Vertex{ point, color });
    //! m_trail.emplace_back<Vertex>(point, color);   // same, built in place
    //! \endcode
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

    //! \brief Same, the vertex being built from \c p_args.
    template <typename Vertex, typename... Args>
        requires(sizeof...(Args) != 1u)
    void emplace_back(Args&&... p_args)
    {
        emplace_back(Vertex{ std::forward<Args>(p_args)... });
    }

    // ------------------------------------------------------------------------
    //! \brief How many vertices there are.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t count() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Remove every vertex, keeping the layout.
    // ------------------------------------------------------------------------
    void clear();

    // ------------------------------------------------------------------------
    //! \brief Draw through these indices rather than the vertices in order.
    //!
    //! \code
    //! m_quad.indices({ 0, 1, 2, 2, 3, 0 });
    //! \endcode
    // ------------------------------------------------------------------------
    void indices(std::initializer_list<std::uint32_t> p_indices)
    {
        indices(std::span<const std::uint32_t>(p_indices.begin(),
                                               p_indices.size()));
    }

    //! \brief Same, from 32 bit indices.
    void indices(std::span<const std::uint32_t> p_indices);

    //! \brief Same, from 16 bit indices, as mesh generators often produce.
    void indices(std::span<const std::uint16_t> p_indices);

    //! \brief Same, from a std::vector.
    template <typename Index>
    void indices(std::vector<Index> const& p_indices)
    {
        indices(std::span<const Index>(p_indices));
    }

    // ------------------------------------------------------------------------
    //! \brief How to draw: primitive, depth test, blending, culling...
    //!
    //! Change it freely; the pipeline is rebuilt at the next draw when it
    //! differs from the one in use.
    // ------------------------------------------------------------------------
    [[nodiscard]] RenderState& state()
    {
        return m_state;
    }

    //! \brief How to draw, read only.
    [[nodiscard]] RenderState const& state() const
    {
        return m_state;
    }

    //! \brief Test and write depth, so that nearer faces hide farther ones.
    Drawable& depthTest(bool p_enabled = true)
    {
        m_state.depth_test = p_enabled;
        return *this;
    }

    //! \brief How colours written are mixed with those already there.
    Drawable& blend(Blend const& p_blend)
    {
        m_state.blend = p_blend;
        return *this;
    }

    //! \brief Which faces not to draw.
    Drawable& cull(CullMode p_mode)
    {
        m_state.cull = p_mode;
        return *this;
    }

    //! \brief What the vertices make: triangles, lines, points...
    Drawable& primitive(Primitive p_primitive)
    {
        m_state.primitive = p_primitive;
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief How often the vertices change once drawn.
    //!
    //! Dynamic by default: vertices can be changed and added at any time.
    //! Immutable tells the driver they never will, which lets it keep them in
    //! the fastest memory; changing one after the first draw is then an error.
    //! \code
    //! m_triangle.vertices<Vertex>({ ... });
    //! m_triangle.usage(compages::gpu::BufferUsage::Immutable);
    //! \endcode
    // ------------------------------------------------------------------------
    Drawable& usage(BufferUsage p_usage)
    {
        if (p_usage != m_usage)
        {
            m_usage = p_usage;
            m_device.release();
        }
        return *this;
    }

    //! \brief How often the vertices change, as usage() said.
    [[nodiscard]] BufferUsage usage() const
    {
        return m_usage;
    }

    // ------------------------------------------------------------------------
    //! \brief Draw everything: every index if there are indices, every vertex
    //! otherwise.
    // ------------------------------------------------------------------------
    void draw();

    // ------------------------------------------------------------------------
    //! \brief Draw everything as this primitive, which becomes the one kept in
    //! state().
    // ------------------------------------------------------------------------
    void draw(Primitive p_primitive);

    // ------------------------------------------------------------------------
    //! \brief Draw the first p_count vertices (or indices).
    //!
    //! Also how a shader making its own vertices from gl_VertexID is drawn:
    //! `m_screen.draw(3u)` for a triangle covering the screen.
    // ------------------------------------------------------------------------
    void draw(std::size_t p_count);

    // ------------------------------------------------------------------------
    //! \brief Draw p_count vertices (or indices) starting at p_first.
    // ------------------------------------------------------------------------
    void drawRange(std::size_t p_first, std::size_t p_count);

    // ------------------------------------------------------------------------
    //! \brief Draw the vertices several times, once per instance.
    //!
    //! \param[in] p_instances how many copies.
    //! \param[in] p_vertices how many vertices make one copy. Zero means all
    //! of them. With a layout read per instance, each record is one instance
    //! and this is the number of corners the shader makes from gl_VertexID:
    //! \code
    //! m_sprites.drawInstanced(m_sprites.count(), 4u);
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
    //! compages::gpu::Barrier::Command); m_kept_points.drawIndirect(m_command);
    //! \endcode
    // ------------------------------------------------------------------------
    void drawIndirect(Buffer<DrawIndirectCommand> const& p_commands);

    // ------------------------------------------------------------------------
    //! \brief Send everything waiting and build the pipeline now.
    //!
    //! Draws nothing. At the end of a set up, it turns the mistakes the first
    //! frame would have found into an error returned there:
    //! \code
    //! COMPAGES_TRY(m_triangle.prepare());
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] Status prepare();

    // ------------------------------------------------------------------------
    //! \brief The program, for what this class does not cover.
    // ------------------------------------------------------------------------
    [[nodiscard]] Program& program()
    {
        return m_program;
    }

    //! \brief The program, read only.
    [[nodiscard]] Program const& program() const
    {
        return m_program;
    }

    //! \brief The vertex layout in use: read off the shader in named mode, off
    //! the struct otherwise. Empty until vertices are given.
    [[nodiscard]] VertexLayout const& layout() const
    {
        return m_layout;
    }

    // ------------------------------------------------------------------------
    //! \brief Which field feeds which attribute, and how, once prepared. What
    //! to print when a shader and its vertices disagree.
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

    template <typename Vertex>
    void assertHolds() const
    {
        assert((m_type == &detail::TYPE_TAG<Vertex>) &&
               "this drawable was given vertices of another struct");
    }

    void adoptProgram(Program p_program);

    // Filling from a struct.
    void adopt(VertexLayout p_layout, const void* p_type, std::size_t p_stride);
    void replaceBytes(std::span<const std::byte> p_bytes, std::size_t p_count);
    void appendBytes(std::span<const std::byte> p_bytes);
    void borrow(BufferHandle p_buffer, std::size_t p_count);
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
    [[nodiscard]] bool expectUniform(std::string const& p_name);

    //! \brief The field an attribute is written to, creating the named layout
    //! on first use, making room for p_count vertices. Records why not.
    [[nodiscard]] FieldDesc const* attributeField(std::string const& p_name,
                                                  std::size_t p_count);
    //! \brief Write one converted value, taken from doubles, into a field.
    void writeValue(FieldDesc const& p_field,
                    std::size_t p_vertex,
                    std::span<const double> p_components);
    void markFilled(std::string const& p_name, std::size_t p_count);

    // Drawing.
    [[nodiscard]] Status ready();
    [[nodiscard]] Status sendVertices();
    [[nodiscard]] Status sendIndices();
    void bindTextures() const;

    Program m_program;
    Pipeline m_pipeline;
    RenderState m_state;
    //! \brief The layout the pipeline was last built with changed.
    bool m_rebuild = true;

    //! \brief The vertices, interleaved, as the layout describes them.
    VertexLayout m_layout;
    std::vector<std::byte> m_bytes;
    std::size_t m_count = 0u;
    DirtyRange m_dirty;
    //! \brief The struct given to vertices(), or nullptr in named mode.
    const void* m_type = nullptr;
    //! \brief Named mode: how many values each attribute received.
    std::vector<Filled> m_filled;
    BufferUsage m_usage = BufferUsage::Dynamic;
    Buffer<std::byte> m_device;
    //! \brief Vertices given as a GPU buffer someone else owns.
    BufferHandle m_borrowed{};
    bool m_borrowing = false;

    std::vector<std::uint32_t> m_indices;
    bool m_indices_changed = false;
    Buffer<std::uint32_t> m_index_device;

    std::vector<Sampler> m_samplers;
};

} // namespace compages::gpu
