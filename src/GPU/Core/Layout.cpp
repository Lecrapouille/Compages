// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/Core/Layout.hpp"

#include <algorithm>

namespace compages::gpu
{

std::string AttributeFormat::glslType() const
{
    const std::string count = std::to_string(components);

    // Several slots means a matrix, which the hardware reads one column per slot.
    // GLSL names it by columns first, so a field spanning four slots of four
    // components is a mat4, and one spanning three slots of four is a mat3x4.
    if (slots > 1u)
    {
        const std::string prefix =
            (scalar == ScalarType::Double) ? "dmat" : "mat";
        if (slots == components)
        {
            return prefix + std::to_string(slots);
        }
        return prefix + std::to_string(slots) + "x" + count;
    }

    if (components == 1u)
    {
        if (normalized || !as_integer)
        {
            return (scalar == ScalarType::Double) ? "double" : "float";
        }
        return (scalar == ScalarType::UInt8) || (scalar == ScalarType::UInt16) ||
                       (scalar == ScalarType::UInt32)
                   ? "uint"
                   : "int";
    }

    if (normalized || !as_integer)
    {
        return ((scalar == ScalarType::Double) ? "dvec" : "vec") + count;
    }
    const bool unsigned_kind = (scalar == ScalarType::UInt8) ||
                               (scalar == ScalarType::UInt16) ||
                               (scalar == ScalarType::UInt32);
    return (unsigned_kind ? "uvec" : "ivec") + count;
}

VertexLayout::VertexLayout(std::vector<FieldDesc> p_fields,
                           std::uint32_t p_stride)
    : m_fields(std::move(p_fields)), m_stride(p_stride)
{
    check();
}

FieldDesc const* VertexLayout::find(std::string_view p_name) const
{
    auto it = std::find_if(
        m_fields.begin(), m_fields.end(), [&](FieldDesc const& p_field) {
            return p_field.name == p_name;
        });
    return (it == m_fields.end()) ? nullptr : &(*it);
}

std::size_t VertexLayout::slots() const
{
    std::size_t total = 0u;
    for (auto const& field : m_fields)
    {
        total += field.format.slots;
    }
    return total;
}

bool VertexLayout::hasPerInstanceFields() const
{
    return std::any_of(
        m_fields.begin(), m_fields.end(), [](FieldDesc const& p_field) {
            return p_field.per_instance;
        });
}

Status VertexLayout::validate() const
{
    if (!m_edit_error.empty())
    {
        return failure(m_edit_error);
    }
    if (m_error.empty())
    {
        return success();
    }
    return failure(m_error);
}

FieldDesc* VertexLayout::edit(std::string_view p_name, const char* p_what)
{
    auto it = std::find_if(
        m_fields.begin(), m_fields.end(), [&](FieldDesc const& p_field) {
            return p_field.name == p_name;
        });
    if (it != m_fields.end())
    {
        return &(*it);
    }
    if (m_edit_error.empty())
    {
        std::string names;
        for (FieldDesc const& field : m_fields)
        {
            names += (names.empty() ? "" : ", ") + field.name;
        }
        m_edit_error = std::string(p_what) + "('" + std::string(p_name) +
                       "') names no field of this vertex, whose fields are: " +
                       (names.empty() ? "none" : names);
    }
    return nullptr;
}

VertexLayout& VertexLayout::rename(std::string_view p_field,
                                   std::string p_attribute) &
{
    if (FieldDesc* field = edit(p_field, "rename"))
    {
        field->name = std::move(p_attribute);
        m_error.clear();
        check();
    }
    return *this;
}

VertexLayout& VertexLayout::perInstance(std::string_view p_field) &
{
    if (FieldDesc* field = edit(p_field, "perInstance"))
    {
        field->per_instance = true;
    }
    return *this;
}

VertexLayout& VertexLayout::perInstance() &
{
    for (FieldDesc& field : m_fields)
    {
        field.per_instance = true;
    }
    return *this;
}

VertexLayout& VertexLayout::normalized(std::string_view p_field) &
{
    FieldDesc* field = edit(p_field, "normalized");
    if (field == nullptr)
    {
        return *this;
    }
    if (!isInteger(field->format.scalar))
    {
        if (m_edit_error.empty())
        {
            m_edit_error = "normalized('" + field->name +
                           "') applies to whole numbers, which it turns into "
                           "fractions. This field already holds " +
                           compages::gpu::toString(field->format.scalar) + "s";
        }
        return *this;
    }
    field->format.normalized = true;
    field->format.as_integer = false;
    return *this;
}

// Everything checked here is knowable without a shader. The point is to fail
// while the reader is still looking at the layout, rather than to draw a wrong
// image and leave them wondering.
void VertexLayout::check()
{
    if (m_fields.empty())
    {
        return;
    }

    if (m_stride == 0u)
    {
        m_error = "a vertex layout with fields cannot have a stride of zero";
        return;
    }

    for (std::size_t i = 0u; i < m_fields.size(); ++i)
    {
        FieldDesc const& field = m_fields[i];

        if (field.name.empty())
        {
            m_error = "field " + std::to_string(i) + " has no name, so no "
                      "shader attribute can be matched to it";
            return;
        }

        if (!field.format.valid())
        {
            m_error = "field '" + field.name + "' has a format the hardware "
                      "cannot read: " +
                      std::to_string(field.format.components) +
                      " components of " + compages::gpu::toString(field.format.scalar) +
                      " over " + std::to_string(field.format.slots) +
                      " slots. Between 1 and 4 components are allowed, and a "
                      "field cannot be both normalized and read as a whole "
                      "number";
            return;
        }

        const std::size_t end = field.offset + field.format.size();
        if (end > m_stride)
        {
            m_error = "field '" + field.name + "' ends at byte " +
                      std::to_string(end) + " but a vertex is only " +
                      std::to_string(m_stride) +
                      " bytes long. Is the field described as belonging to the "
                      "wrong struct?";
            return;
        }

        for (std::size_t j = i + 1u; j < m_fields.size(); ++j)
        {
            FieldDesc const& other = m_fields[j];

            if (other.name == field.name)
            {
                m_error = "two fields are both named '" + field.name +
                          "'. A shader attribute would not know which one to "
                          "read";
                return;
            }

            const std::size_t other_end = other.offset + other.format.size();
            const bool overlap =
                (field.offset < other_end) && (other.offset < end);
            if (overlap)
            {
                m_error = "fields '" + field.name + "' at byte " +
                          std::to_string(field.offset) + " and '" + other.name +
                          "' at byte " + std::to_string(other.offset) +
                          " overlap. Describing the same member twice is the "
                          "usual cause";
                return;
            }
        }
    }
}

std::string VertexLayout::toString() const
{
    if (m_fields.empty())
    {
        return "no vertex data";
    }

    std::string text = "vertex of " + std::to_string(m_stride) + " bytes:";
    for (auto const& field : m_fields)
    {
        text += "\n  " + field.format.glslType() + " " + field.name +
                " at byte " + std::to_string(field.offset) + " (" +
                std::to_string(field.format.components) + " x " +
                compages::gpu::toString(field.format.scalar);
        if (field.format.normalized)
        {
            text += ", normalized";
        }
        if (field.per_instance)
        {
            text += ", per instance";
        }
        text += ")";
    }
    return text;
}

} // namespace compages::gpu
