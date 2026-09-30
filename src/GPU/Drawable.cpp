// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/Drawable.hpp"
#include "Compages/GPU/Draw.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace compages::gpu
{

namespace
{

std::size_t valuesPerVertex(AttributeFormat const& p_format)
{
    return std::size_t{ p_format.components } * std::size_t{ p_format.slots };
}

bool sameFormat(AttributeFormat const& p_left, AttributeFormat const& p_right)
{
    return (p_left.scalar == p_right.scalar) &&
           (p_left.components == p_right.components) &&
           (p_left.slots == p_right.slots) &&
           (p_left.normalized == p_right.normalized) &&
           (p_left.as_integer == p_right.as_integer);
}

bool sameLayout(VertexLayout const& p_left, VertexLayout const& p_right)
{
    if ((p_left.stride() != p_right.stride()) ||
        (p_left.fields().size() != p_right.fields().size()))
    {
        return false;
    }
    for (std::size_t i = 0u; i < p_left.fields().size(); ++i)
    {
        FieldDesc const& a = p_left.fields()[i];
        FieldDesc const& b = p_right.fields()[i];
        if ((a.name != b.name) || (a.offset != b.offset) ||
            (a.per_instance != b.per_instance) ||
            !sameFormat(a.format, b.format))
        {
            return false;
        }
    }
    return true;
}

template <typename T>
void store(std::byte* p_destination, double p_value, bool p_normalized)
{
    T value;
    if constexpr (std::is_floating_point_v<T>)
    {
        value = static_cast<T>(p_value);
    }
    else
    {
        // A normalized integer field is written as the fraction it stands for:
        // 1.0 is the largest value the integer holds.
        const double scaled =
            p_normalized
                ? std::round(p_value * double(std::numeric_limits<T>::max()))
                : p_value;
        const double clamped =
            std::clamp(scaled,
                       double(std::numeric_limits<T>::lowest()),
                       double(std::numeric_limits<T>::max()));
        value = static_cast<T>(clamped);
    }
    std::memcpy(p_destination, &value, sizeof(T));
}

std::string unknownName(std::string const& p_name, Program const& p_program)
{
    return "'" + p_name +
           "' is neither an attribute, a uniform nor a sampler of this "
           "shader (a name the shader declares but never uses is removed by "
           "the compiler). The shader has:\n" +
           p_program.reflection().toString();
}

} // namespace

Status Drawable::load(std::string_view p_vertex, std::string_view p_fragment)
{
    Program program;
    COMPAGES_TRY(program.load(p_vertex, p_fragment));
    adoptProgram(std::move(program));
    return success();
}

Status Drawable::loadFiles(std::string const& p_vertex,
                           std::string const& p_fragment)
{
    Program program;
    COMPAGES_TRY(program.loadFiles(p_vertex, p_fragment));
    adoptProgram(std::move(program));
    return success();
}

void Drawable::adoptProgram(Program p_program)
{
    // The pipeline refers to the program: let it go before the program does.
    m_pipeline.release();
    m_program = std::move(p_program);
    m_rebuild = true;
    m_layout = VertexLayout{};
    m_type = nullptr;
    m_filled.clear();
    m_bytes.clear();
    m_count = 0u;
    m_dirty.clear();
    m_device.release();
    m_borrowing = false;
    m_indices.clear();
    m_indices_changed = false;
    m_index_device.release();
    m_samplers.clear();
}

void Drawable::clear()
{
    m_borrowing = false;
    m_bytes.clear();
    m_count = 0u;
    m_filled.clear();
    m_dirty.clear();
}

void Drawable::indices(std::span<const std::uint32_t> p_indices)
{
    m_indices.assign(p_indices.begin(), p_indices.end());
    m_indices_changed = true;
}

void Drawable::indices(std::span<const std::uint16_t> p_indices)
{
    m_indices.assign(p_indices.begin(), p_indices.end());
    m_indices_changed = true;
}

void Drawable::adopt(VertexLayout p_layout,
                     const void* p_type,
                     std::size_t p_stride)
{
    assert((p_layout.empty() || (p_layout.stride() == p_stride)) &&
           "the layout given does not describe the struct given");
    (void)p_stride;
    if ((m_type != p_type) || !sameLayout(m_layout, p_layout))
    {
        m_layout = std::move(p_layout);
        m_rebuild = true;
    }
    m_type = p_type;
    m_filled.clear();
}

void Drawable::replaceBytes(std::span<const std::byte> p_bytes,
                            std::size_t p_count)
{
    m_borrowing = false;
    m_bytes.assign(p_bytes.begin(), p_bytes.end());
    m_count = p_count;
    m_dirty.addAll(m_bytes.size());
}

void Drawable::appendBytes(std::span<const std::byte> p_bytes)
{
    if (m_borrowing)
    {
        m_borrowing = false;
        m_count = 0u;
    }
    const std::size_t first = m_bytes.size();
    m_bytes.insert(m_bytes.end(), p_bytes.begin(), p_bytes.end());
    m_count += 1u;
    m_dirty.add(first, p_bytes.size());
}

void Drawable::borrow(BufferHandle p_buffer, std::size_t p_count)
{
    m_bytes.clear();
    m_dirty.clear();
    m_device.release();
    m_borrowed = p_buffer;
    m_borrowing = true;
    m_count = p_count;
}

BufferHandle Drawable::vertexBuffer() const
{
    return m_borrowing ? m_borrowed : m_device.handle();
}

FieldDesc const* Drawable::attributeField(std::string const& p_name,
                                          std::size_t p_count)
{
    if (!m_program.valid())
    {
        reportError("drawable[\"" + p_name +
                    "\"] before load(): the shader "
                    "is what says what '" +
                    p_name + "' is");
        return nullptr;
    }

    ProgramReflection const& reflection = m_program.reflection();
    if (reflection.attribute(p_name) == nullptr)
    {
        if (reflection.uniform(p_name) != nullptr)
        {
            reportError("'" + p_name +
                        "' is a uniform, which holds one value "
                        "for the whole draw, not one per vertex");
        }
        else
        {
            reportError(unknownName(p_name, m_program));
        }
        return nullptr;
    }

    // Named mode: the first attribute given lays out a vertex the way the
    // shader reads it, every attribute in the order of its location.
    if ((m_type == nullptr) && m_layout.empty())
    {
        std::vector<AttributeInfo const*> attributes;
        for (AttributeInfo const& attribute : reflection.attributes)
        {
            if (attribute.location >= 0)
            {
                attributes.emplace_back(&attribute);
            }
        }
        std::sort(attributes.begin(),
                  attributes.end(),
                  [](AttributeInfo const* p_a, AttributeInfo const* p_b)
                  { return p_a->location < p_b->location; });

        std::vector<FieldDesc> fields;
        std::uint32_t offset = 0u;
        for (AttributeInfo const* attribute : attributes)
        {
            fields.emplace_back(
                FieldDesc{ attribute->name, attribute->format, offset, false });
            offset += static_cast<std::uint32_t>(attribute->format.size());
        }
        m_layout = VertexLayout(std::move(fields), offset);
        m_rebuild = true;
    }

    FieldDesc const* field = m_layout.find(p_name);
    if (field == nullptr)
    {
        reportError("the shader reads the attribute '" + p_name +
                    "' but the vertex struct has no field of that name. "
                    "The layout is:\n" +
                    m_layout.toString());
        return nullptr;
    }

    if (p_count != m_count)
    {
        if (m_type != nullptr)
        {
            reportError("'" + p_name + "' was given " +
                        std::to_string(p_count) + " values, but there are " +
                        std::to_string(m_count) +
                        " vertices: to change their number, give the vertices "
                        "again with vertices<Vertex>()");
            return nullptr;
        }
        // The other attributes keep what they had and are checked at the
        // draw: they must end up with as many values as this one.
        m_count = p_count;
        m_bytes.resize(m_count * m_layout.stride());
        m_dirty.addAll(m_bytes.size());
    }
    return field;
}

void Drawable::writeValue(FieldDesc const& p_field,
                          std::size_t p_vertex,
                          std::span<const double> p_components)
{
    std::byte* destination =
        m_bytes.data() + p_vertex * m_layout.stride() + p_field.offset;
    const std::size_t scalar_size = sizeOf(p_field.format.scalar);
    const bool normalized = p_field.format.normalized;

    for (double component : p_components)
    {
        switch (p_field.format.scalar)
        {
            case ScalarType::Float:
                store<float>(destination, component, normalized);
                break;
            case ScalarType::Double:
                store<double>(destination, component, normalized);
                break;
            case ScalarType::Int8:
                store<std::int8_t>(destination, component, normalized);
                break;
            case ScalarType::UInt8:
                store<std::uint8_t>(destination, component, normalized);
                break;
            case ScalarType::Int16:
                store<std::int16_t>(destination, component, normalized);
                break;
            case ScalarType::UInt16:
                store<std::uint16_t>(destination, component, normalized);
                break;
            case ScalarType::Int32:
                store<std::int32_t>(destination, component, normalized);
                break;
            case ScalarType::UInt32:
                store<std::uint32_t>(destination, component, normalized);
                break;
            case ScalarType::Half:
                reportError("the field '" + p_field.name +
                            "' holds half floats, which cannot be written by "
                            "name: fill it from a struct");
                return;
        }
        destination += scalar_size;
    }
    m_dirty.add(p_vertex * m_layout.stride() + p_field.offset,
                valuesPerVertex(p_field.format) * scalar_size);
}

void Drawable::markFilled(std::string const& p_name, std::size_t p_count)
{
    if (m_type != nullptr)
    {
        return;
    }
    auto it = std::find_if(m_filled.begin(),
                           m_filled.end(),
                           [&](Filled const& p_filled)
                           { return p_filled.name == p_name; });
    if (it == m_filled.end())
    {
        m_filled.emplace_back(Filled{ p_name, p_count });
    }
    else
    {
        it->count = p_count;
    }
}

template <typename Scalar>
void Drawable::assignNested(
    std::string const& p_name,
    std::initializer_list<std::initializer_list<Scalar>> p_values)
{
    FieldDesc const* field = attributeField(p_name, p_values.size());
    if (field == nullptr)
    {
        return;
    }

    const std::size_t expected = valuesPerVertex(field->format);
    std::vector<double> components;
    std::size_t vertex = 0u;
    for (std::initializer_list<Scalar> const& value : p_values)
    {
        if (value.size() != expected)
        {
            reportError("vertex " + std::to_string(vertex) + " of '" + p_name +
                        "' has " + std::to_string(value.size()) +
                        " components, but the shader reads a " +
                        field->format.glslType() + " of " +
                        std::to_string(expected));
            return;
        }
        components.assign(value.begin(), value.end());
        writeValue(*field, vertex, components);
        ++vertex;
    }
    markFilled(p_name, p_values.size());
}

template void Drawable::assignNested<float>(
    std::string const&,
    std::initializer_list<std::initializer_list<float>>);
template void
Drawable::assignNested<int>(std::string const&,
                            std::initializer_list<std::initializer_list<int>>);

void Drawable::assignFlat(std::string const& p_name,
                          std::initializer_list<float> p_values)
{
    if (m_program.valid() &&
        (m_program.reflection().uniform(p_name) != nullptr))
    {
        const float* v = p_values.begin();
        switch (p_values.size())
        {
            case 1u:
                m_program.set(p_name, v[0]);
                return;
            case 2u:
                m_program.set(p_name, compages::core::Vector2f(v[0], v[1]));
                return;
            case 3u:
                m_program.set(p_name,
                              compages::core::Vector3f(v[0], v[1], v[2]));
                return;
            case 4u:
                m_program.set(p_name,
                              compages::core::Vector4f(v[0], v[1], v[2], v[3]));
                return;
            default:
                reportError("the uniform '" + p_name + "' was given " +
                            std::to_string(p_values.size()) +
                            " numbers: a vector has two to four");
                return;
        }
    }

    FieldDesc const* field = attributeField(p_name, p_values.size());
    if (field == nullptr)
    {
        return;
    }
    if (valuesPerVertex(field->format) != 1u)
    {
        reportError(
            "'" + p_name + "' is a " + field->format.glslType() +
            ", so each vertex needs its own braces: { {x, y}, {x, y} }");
        return;
    }
    std::size_t vertex = 0u;
    for (float value : p_values)
    {
        const double component = value;
        writeValue(*field, vertex, std::span<const double>(&component, 1u));
        ++vertex;
    }
    markFilled(p_name, p_values.size());
}

void Drawable::assignTyped(std::string const& p_name,
                           DataType p_type,
                           std::span<const std::byte> p_values)
{
    if (m_program.valid())
    {
        AttributeInfo const* attribute =
            m_program.reflection().attribute(p_name);
        if ((attribute != nullptr) && (attribute->type != p_type))
        {
            reportError(std::string("'") + p_name + "' is a " +
                        toString(attribute->type) +
                        " in the shader, but was "
                        "given values of type " +
                        toString(p_type));
            return;
        }
    }

    const std::size_t size = attributeFormatOf(p_type).size();
    const std::size_t count = (size == 0u) ? 0u : (p_values.size() / size);
    FieldDesc const* field = attributeField(p_name, count);
    if (field == nullptr)
    {
        return;
    }
    if (!sameFormat(field->format, attributeFormatOf(p_type)))
    {
        reportError("the field '" + p_name + "' is stored as a " +
                    field->format.glslType() +
                    ", which values of another type cannot be copied into");
        return;
    }

    const std::size_t stride = m_layout.stride();
    for (std::size_t i = 0u; i < count; ++i)
    {
        std::memcpy(m_bytes.data() + i * stride + field->offset,
                    p_values.data() + i * size,
                    size);
    }
    m_dirty.addAll(m_bytes.size());
    markFilled(p_name, count);
}

void Drawable::assignOne(std::string const& p_name,
                         DataType p_type,
                         std::size_t p_vertex,
                         std::span<const std::byte> p_value)
{
    if (p_vertex >= m_count)
    {
        reportError("'" + p_name + "'.set(" + std::to_string(p_vertex) +
                    ", ...) but there are " + std::to_string(m_count) +
                    " vertices");
        return;
    }
    FieldDesc const* field = attributeField(p_name, m_count);
    if (field == nullptr)
    {
        return;
    }
    if (!sameFormat(field->format, attributeFormatOf(p_type)) ||
        (p_value.size() != field->format.size()))
    {
        reportError("'" + p_name + "' is a " + field->format.glslType() +
                    ", which a value of type " + toString(p_type) +
                    " cannot be copied into");
        return;
    }
    const std::size_t at = p_vertex * m_layout.stride() + field->offset;
    std::memcpy(m_bytes.data() + at, p_value.data(), p_value.size());
    m_dirty.add(at, p_value.size());
}

bool Drawable::expectUniform(std::string const& p_name)
{
    if (!m_program.valid())
    {
        reportError("drawable[\"" + p_name +
                    "\"] before load(): the shader "
                    "is what says what '" +
                    p_name + "' is");
        return false;
    }
    ProgramReflection const& reflection = m_program.reflection();
    if (reflection.uniform(p_name) != nullptr)
    {
        return true;
    }
    if (reflection.attribute(p_name) != nullptr)
    {
        reportError("'" + p_name +
                    "' is an attribute, which takes one value "
                    "per vertex: { {...}, {...}, ... }");
    }
    else
    {
        reportError(unknownName(p_name, m_program));
    }
    return false;
}

void Drawable::assignTexture(std::string const& p_name,
                             Texture const& p_texture)
{
    if (!m_program.valid())
    {
        reportError("drawable[\"" + p_name + "\"] = texture before load()");
        return;
    }
    UniformInfo const* uniform = m_program.reflection().uniform(p_name);
    if ((uniform == nullptr) || !isSampler(uniform->type))
    {
        reportError((uniform == nullptr)
                        ? unknownName(p_name, m_program)
                        : ("'" + p_name + "' is a " + toString(uniform->type) +
                           ", not a sampler: a texture cannot be given to it"));
        return;
    }

    auto it = std::find_if(m_samplers.begin(),
                           m_samplers.end(),
                           [&](Sampler const& p_sampler)
                           { return p_sampler.name == p_name; });
    if (it == m_samplers.end())
    {
        const auto unit = static_cast<std::uint32_t>(m_samplers.size());
        m_samplers.emplace_back(Sampler{ p_name, &p_texture, unit });
        m_program.set(p_name, static_cast<int>(unit));
    }
    else
    {
        it->texture = &p_texture;
    }
}

Status Drawable::sendVertices()
{
    if (m_borrowing || m_bytes.empty())
    {
        return success();
    }
    if (m_usage == BufferUsage::Immutable)
    {
        if (!m_device.valid())
        {
            COMPAGES_TRY_ASSIGN(
                m_device,
                Buffer<std::byte>::from(std::span<const std::byte>(m_bytes),
                                        BufferKind::Vertex,
                                        BufferUsage::Immutable));
            m_dirty.clear();
        }
        else if (!m_dirty.empty() || (m_device.count() != m_bytes.size()))
        {
            m_dirty.clear();
            return failure(
                "the vertices of a drawable whose usage is Immutable changed "
                "after its first draw. Say "
                "usage(compages::gpu::BufferUsage::Dynamic) for "
                "vertices meant to change");
        }
        return success();
    }
    if (!m_device.valid() || (m_device.count() < m_bytes.size()))
    {
        // Growing by doubling keeps emplace_back cheap: the buffer is recreated
        // a logarithmic number of times, not once per vertex.
        const std::size_t capacity =
            std::max(m_bytes.size(), 2u * m_device.count());
        COMPAGES_TRY_ASSIGN(m_device,
                            Buffer<std::byte>::create(capacity,
                                                      BufferKind::Vertex,
                                                      BufferUsage::Dynamic));
        m_dirty.addAll(m_bytes.size());
    }
    m_dirty.clampTo(m_bytes.size());
    if (!m_dirty.empty())
    {
        COMPAGES_TRY(detail::writeBuffer(m_device.handle(),
                                         m_dirty.begin(),
                                         m_dirty.count(),
                                         m_bytes.data() + m_dirty.begin()));
        m_dirty.clear();
    }
    return success();
}

Status Drawable::sendIndices()
{
    if (!m_indices_changed)
    {
        return success();
    }
    m_indices_changed = false;
    if (m_indices.empty())
    {
        m_index_device.release();
        return success();
    }
    COMPAGES_TRY_ASSIGN(
        m_index_device,
        Buffer<std::uint32_t>::from(std::span<const std::uint32_t>(m_indices),
                                    BufferKind::Index,
                                    BufferUsage::Dynamic));
    return success();
}

Status Drawable::ready()
{
    if (!m_program.valid())
    {
        return failure("a drawable was drawn before load() succeeded");
    }

    if (m_type == nullptr)
    {
        for (AttributeInfo const& attribute : m_program.reflection().attributes)
        {
            if (attribute.location < 0)
            {
                continue;
            }
            auto it = std::find_if(m_filled.begin(),
                                   m_filled.end(),
                                   [&](Filled const& p_filled)
                                   { return p_filled.name == attribute.name; });
            if (it == m_filled.end())
            {
                return failure("the shader reads the attribute '" +
                               attribute.name +
                               "', which was never given "
                               "values: drawable[\"" +
                               attribute.name + "\"] = { ... }");
            }
            if (it->count != m_count)
            {
                return failure("'" + attribute.name + "' has " +
                               std::to_string(it->count) +
                               " values but the other attributes have " +
                               std::to_string(m_count) +
                               ": every attribute needs one value per vertex");
            }
        }
    }

    if (m_rebuild || !m_pipeline.valid() || !(m_pipeline.state() == m_state))
    {
        m_pipeline.release();
        COMPAGES_TRY_ASSIGN(m_pipeline,
                            Pipeline::create(m_program, m_layout, m_state));
        m_rebuild = false;
    }

    COMPAGES_TRY(sendVertices());
    COMPAGES_TRY(sendIndices());
    return success();
}

void Drawable::bindTextures() const
{
    for (Sampler const& sampler : m_samplers)
    {
        sampler.texture->bind(sampler.unit);
    }
}

Status Drawable::prepare()
{
    COMPAGES_TRY(ready());
    if (hasFrameError())
    {
        return failure(takeFrameError());
    }
    return success();
}

void Drawable::draw()
{
    const std::size_t count = m_indices.empty() ? m_count : m_indices.size();
    if (count == 0u)
    {
        return reportError("draw() with nothing to draw: give vertices "
                           "first, or say how many the shader makes with "
                           "draw(count)");
    }
    drawRange(0u, count);
}

void Drawable::draw(Primitive p_primitive)
{
    m_state.primitive = p_primitive;
    draw();
}

void Drawable::draw(std::size_t p_count)
{
    drawRange(0u, p_count);
}

void Drawable::drawRange(std::size_t p_first, std::size_t p_count)
{
    if (!check(ready()))
    {
        return;
    }
    bindTextures();

    if (!m_indices.empty())
    {
        drawIndexed(m_pipeline,
                    vertexBuffer(),
                    m_index_device.handle(),
                    IndexType::UInt32,
                    p_count,
                    p_first);
    }
    else if (!m_borrowing && m_bytes.empty())
    {
        if (p_first != 0u)
        {
            return reportError("drawRange() from vertex " +
                               std::to_string(p_first) +
                               " of a shader making its own vertices: "
                               "offset gl_VertexID in the shader instead");
        }
        drawWithoutVertices(m_pipeline, p_count);
    }
    else
    {
        compages::gpu::draw(m_pipeline, vertexBuffer(), p_count, p_first);
    }
}

void Drawable::drawInstanced(std::size_t p_instances, std::size_t p_vertices)
{
    if (!check(ready()))
    {
        return;
    }
    bindTextures();
    const std::size_t vertices = (p_vertices == 0u) ? m_count : p_vertices;
    compages::gpu::drawInstanced(
        m_pipeline, vertexBuffer(), vertices, p_instances);
}

void Drawable::drawIndirect(Buffer<DrawIndirectCommand> const& p_commands)
{
    if (!check(ready()))
    {
        return;
    }
    bindTextures();
    compages::gpu::drawIndirect(
        m_pipeline, vertexBuffer(), p_commands.handle());
}

std::string Drawable::describe() const
{
    std::string text = m_layout.toString();
    if (m_borrowing)
    {
        text += "read from a GPU buffer the drawable does not own\n";
    }
    else if (m_usage == BufferUsage::Immutable)
    {
        text += "sent once and never changed (usage Immutable)\n";
    }
    if (m_pipeline.valid())
    {
        text += "\n" + m_pipeline.describeAttributes();
    }
    return text;
}

} // namespace compages::gpu
