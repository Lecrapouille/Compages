// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Internal/Pools.hpp"

#include <algorithm>

namespace compages::gpu
{

namespace
{

//! \brief The names of every field of a layout, for an error that has to say what
//! was on offer.
std::string namesOf(VertexLayout const& p_layout)
{
    if (p_layout.fields().empty())
    {
        return "nothing at all";
    }

    std::string text;
    for (FieldDesc const& field : p_layout.fields())
    {
        if (!text.empty())
        {
            text += ", ";
        }
        text += field.format.glslType() + " " + field.name;
    }
    return text;
}

//! \brief Can a field stored like this feed an attribute declared like that?
//!
//! Both sides are an AttributeFormat by the time they get here, one from the C++
//! struct and one from what the driver says the shader declared, which is the
//! whole reason the two are described in the same words.
//!
//! \return empty when they agree, otherwise what to tell the caller.
std::string whyIncompatible(AttributeFormat const& p_field,
                            AttributeFormat const& p_wanted)
{
    // Whether the shader sees whole numbers or fractions is the one mismatch that
    // silently produces nonsense rather than nothing: the same bytes read as an
    // integer and as a float have no relation to each other.
    if (p_field.as_integer != p_wanted.as_integer)
    {
        if (p_wanted.as_integer)
        {
            return "the shader reads whole numbers but the field is stored as "
                   "floating point, or was marked normalized(), which turns "
                   "whole numbers into fractions. Drop normalized(), or declare "
                   "the attribute as a float type in the shader";
        }
        return "the shader reads floating point but the field is stored as whole "
               "numbers, which would arrive as unrelated values. Either mark the "
               "field normalized() to turn it into a fraction between 0 and 1, "
               "or declare the attribute as an integer type in the shader";
    }

    if (p_field.components != p_wanted.components)
    {
        return "the shader wants " + std::to_string(p_wanted.components) +
               " components per slot and the field holds " +
               std::to_string(p_field.components) +
               ". The missing ones would be filled in with zeroes, and the extra "
               "ones dropped, so this is refused rather than half done";
    }

    if (p_field.slots != p_wanted.slots)
    {
        return "the shader reads this over " + std::to_string(p_wanted.slots) +
               " attribute slots and the field spans " +
               std::to_string(p_field.slots) +
               ", so one of the two is not the matrix the other thinks it is";
    }

    // Everything else about the kind of number is left alone on purpose. The
    // hardware converts a half or a short into a float on the way in, and that
    // conversion is the reason half precision and normalized fields exist. Double
    // precision is the exception: nothing is converted up to it.
    if ((p_wanted.scalar == ScalarType::Double) &&
        (p_field.scalar != ScalarType::Double))
    {
        return "the shader declares this as a double precision attribute, which "
               "the hardware reads only from double precision data, but the field "
               "is stored as " +
               std::string(toString(p_field.scalar));
    }

    return {};
}

//! \brief Match every attribute the shader declares to a field of the layout.
//!
//! This is the check the whole file exists for, so it is worth being clear about
//! its direction. The shader is the one asking: every attribute it declares must
//! be found. Fields the shader ignores are fine and simply not sent, which is
//! precisely what lets one vertex buffer feed a pass reading three fields and
//! another reading one.
Result<std::vector<backend::VertexAttribute>> matchAttributes(
    ProgramReflection const& p_reflection, VertexLayout const& p_layout)
{
    std::vector<backend::VertexAttribute> matched;
    matched.reserve(p_reflection.attributes.size());

    std::size_t slots_used = 0u;

    for (AttributeInfo const& wanted : p_reflection.attributes)
    {
        if (wanted.location < 0)
        {
            // The driver reports an attribute it decided not to give a slot to.
            // Nothing has to be sent for it.
            continue;
        }

        FieldDesc const* field = p_layout.find(wanted.name);
        if (field == nullptr)
        {
            return failure(
                "the shader reads a vertex attribute called '" + wanted.name +
                "' of type " + toString(wanted.type) +
                ", which this vertex does not have. It offers: " +
                namesOf(p_layout) +
                ".\nThe name of a field is the name of the attribute it feeds. "
                "Either rename one of them, or say which field feeds it, as in "
                "compages::gpu::VertexLayout::of<Vertex>().rename(\"position\", \"" +
                wanted.name + "\")");
        }

        const std::string why =
            whyIncompatible(field->format, attributeFormatOf(wanted.type));
        if (!why.empty())
        {
            return failure("the field '" + field->name + "', stored as " +
                           field->format.glslType() +
                           ", cannot feed the shader attribute '" + wanted.name +
                           "' declared as " + toString(wanted.type) + ": " + why);
        }

        if (wanted.elements > 1)
        {
            return failure(
                "the shader declares '" + wanted.name + "' as an array of " +
                std::to_string(wanted.elements) +
                " attributes. One field of a vertex struct feeds one attribute, "
                "so declare them separately in the shader");
        }

        matched.emplace_back(backend::VertexAttribute{ wanted.location,
                                                    field->format,
                                                    field->offset,
                                                    field->per_instance });
        slots_used += field->format.slots;
    }

    const auto allowed = static_cast<std::size_t>(device().max_vertex_attributes);
    if (slots_used > allowed)
    {
        return failure("this shader reads " + std::to_string(slots_used) +
                       " attribute slots but the driver offers " +
                       std::to_string(allowed) +
                       ". Note that a mat4 attribute takes four of them");
    }

    // A slot the driver assigned beyond what it says it has would be its own bug,
    // but the symptom lands on the caller, so it is worth naming.
    for (backend::VertexAttribute const& one : matched)
    {
        const auto last = static_cast<std::size_t>(one.location) +
                          one.format.slots;
        if (last > allowed)
        {
            return failure(
                "an attribute was given slot " + std::to_string(one.location) +
                " and spans " + std::to_string(one.format.slots) +
                ", which runs past the " + std::to_string(allowed) +
                " slots the driver offers. Lower the explicit layout(location "
                "= ...) in the shader");
        }
    }

    return matched;
}

//! \brief The message given when a handle names nothing alive.
std::string stalePipelineMessage()
{
    return "this pipeline no longer exists. Either it was released while "
           "something still referred to it, or the Pipeline object was moved from "
           "and the old one is being used";
}

} // namespace

Result<Pipeline> Pipeline::create(Program const& p_program,
                                  VertexLayout const& p_layout,
                                  RenderState const& p_state)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is no device "
                       "to make a pipeline on");
    }

    detail::ProgramRecord const* program =
        detail::pools().programs.get(p_program.handle());
    if (program == nullptr)
    {
        return failure("this program has not been linked, or was released. A "
                       "pipeline needs a linked program to check the vertex "
                       "layout against");
    }

    // Whatever can be known about the layout on its own: no field running past
    // the end of the vertex, no two fields overlapping, no repeated name.
    COMPAGES_TRY(p_layout.validate());

    ProgramReflection const& reflection = program->reflection;

    // A compute program has no vertices at all, so pairing one with a layout is a
    // mistake about which kind of program it is.
    if (reflection.work_group_size[0] != 0)
    {
        return failure(
            "this is a compute program, which is dispatched over a grid of work "
            "groups rather than fed with vertices, so it cannot be put in a "
            "pipeline. Use compages::gpu::ComputeProgram for it");
    }

    if (reflection.attributes.empty() && !p_layout.empty())
    {
        return failure(
            "this shader reads no vertex attributes at all, but a layout "
            "describing " +
            std::to_string(p_layout.fields().size()) +
            " fields was given. A pass that generates its own vertices, such as "
            "a full screen quad, takes a default constructed compages::gpu::VertexLayout");
    }

    auto attributes_result = matchAttributes(reflection, p_layout);
    if (!attributes_result)
    {
        return compages::failure(attributes_result.error());
    }
    auto attributes = attributes_result.take();

    // Wireframe of a primitive that has no faces to outline draws the same thing
    // either way, which is confusing enough to be worth saying.
    if ((p_state.polygon != PolygonMode::Fill) &&
        (verticesPerPrimitive(p_state.primitive) == 1u))
    {
        return failure(std::string("drawing points as ") +
                       toString(p_state.polygon) +
                       " means nothing: a point has no edges and no faces");
    }

    auto reader_result = backend::acquireVertexReader(attributes, p_layout.stride());
    if (!reader_result)
    {
        return compages::failure(reader_result.error());
    }
    auto reader = reader_result.take();

    detail::PipelineRecord record;
    record.reader = reader;
    record.program = p_program.handle();
    record.native_program = program->native;
    record.layout = p_layout;
    record.state = p_state;
    record.attributes = std::move(attributes);

    auto added = detail::pools().pipelines.add(std::move(record));
    if (!added)
    {
        backend::releaseVertexReader(reader);
        return failure(added.error());
    }
    return Pipeline(added.take());
}

Pipeline::Pipeline(Pipeline&& p_other) noexcept : m_handle(p_other.m_handle)
{
    p_other.m_handle = PipelineHandle{};
}

Pipeline& Pipeline::operator=(Pipeline&& p_other) noexcept
{
    if (this != &p_other)
    {
        release();
        m_handle = p_other.m_handle;
        p_other.m_handle = PipelineHandle{};
    }
    return *this;
}

Pipeline::~Pipeline()
{
    release();
}

void Pipeline::release()
{
    detail::PipelineRecord* record = detail::pools().pipelines.get(m_handle);
    if (record != nullptr)
    {
        if (initialized())
        {
            backend::releaseVertexReader(record->reader);
        }
        (void)detail::pools().pipelines.remove(m_handle);
    }
    m_handle = PipelineHandle{};
}

bool Pipeline::valid() const
{
    return detail::pools().pipelines.valid(m_handle);
}

Status Pipeline::bind() const
{
    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(m_handle);
    if (record == nullptr)
    {
        return failure(stalePipelineMessage());
    }

    // The one thing a pipeline cannot check when it is created, because it can
    // only become wrong afterwards: the program it names may have been released
    // in the meantime.
    if (!detail::pools().programs.valid(record->program))
    {
        return failure("the program this pipeline draws with has been released. "
                       "A pipeline does not own its program, so the program has "
                       "to outlive it");
    }

    backend::bindPipeline(record->native_program, record->reader, record->state);
    return success();
}

ProgramHandle Pipeline::program() const
{
    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(m_handle);
    return (record == nullptr) ? ProgramHandle{} : record->program;
}

VertexLayout const& Pipeline::layout() const
{
    static const VertexLayout nothing;
    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(m_handle);
    return (record == nullptr) ? nothing : record->layout;
}

RenderState const& Pipeline::state() const
{
    static const RenderState nothing;
    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(m_handle);
    return (record == nullptr) ? nothing : record->state;
}

std::uint32_t Pipeline::stride() const
{
    return layout().stride();
}

bool Pipeline::instanced() const
{
    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(m_handle);
    if (record == nullptr)
    {
        return false;
    }
    return std::any_of(record->attributes.begin(),
                       record->attributes.end(),
                       [](backend::VertexAttribute const& p_attribute) {
                           return p_attribute.per_instance;
                       });
}

std::string Pipeline::describeAttributes() const
{
    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(m_handle);
    if (record == nullptr)
    {
        return stalePipelineMessage();
    }
    if (record->attributes.empty())
    {
        return "this pipeline sends no vertex attributes: the shader generates "
               "its own vertices";
    }

    std::string text;
    for (backend::VertexAttribute const& one : record->attributes)
    {
        // The field of the layout sitting at this offset is the one that was
        // matched, and its name is what the reader is looking for.
        const char* name = "?";
        for (FieldDesc const& field : record->layout.fields())
        {
            if (field.offset == one.offset)
            {
                name = field.name.c_str();
                break;
            }
        }

        text += "slot " + std::to_string(one.location) + ": " +
                one.format.glslType() + " " + name + " at byte " +
                std::to_string(one.offset);
        if (one.per_instance)
        {
            text += ", once per instance";
        }
        text += "\n";
    }
    // The fields nobody reads are the other half of the answer.
    for (FieldDesc const& field : record->layout.fields())
    {
        const bool matched =
            std::any_of(record->attributes.begin(),
                        record->attributes.end(),
                        [&field](backend::VertexAttribute const& p_attribute) {
                            return p_attribute.offset == field.offset;
                        });
        if (!matched)
        {
            text += "not read by this shader: " + field.format.glslType() + " " +
                    field.name + "\n";
        }
    }
    return text;
}

std::size_t livePipelines()
{
    return detail::pools().pipelines.size();
}

std::size_t vertexReadersHeld()
{
    return initialized() ? backend::vertexReadersHeld() : 0u;
}

void forgetRenderState()
{
    if (initialized())
    {
        backend::forgetRenderState();
    }
}

void showWireframe(bool p_enabled)
{
    backend::showWireframe(p_enabled);
}

bool wireframeShown()
{
    return backend::wireframeShown();
}

} // namespace compages::gpu
