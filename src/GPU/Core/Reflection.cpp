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

#include "Compages/GPU/Core/Reflection.hpp"

#include <algorithm>
#include <iterator>

namespace compages::gpu
{

namespace
{

//! \brief What family a type belongs to, which is what decides how it is bound.
enum class Family
{
    //! \brief A number, a vector or a matrix: it has components.
    Numeric,
    //! \brief A texture read through a sampler.
    Sampler,
    //! \brief A texture a shader may also write to.
    Image,
    //! \brief Something the library has no name for.
    Unknown,
};

// ****************************************************************************
//! \brief Everything worth knowing about one type, in one row.
// ****************************************************************************
struct TypeInfo
{
    //! \brief Which type this row describes, so the table can be checked against
    //! the enum at compile time.
    DataType type;
    //! \brief How it is spelled in shader source.
    const char* name;
    //! \brief How many columns. One for anything but a matrix.
    std::uint8_t columns;
    //! \brief How many components one column holds. Zero for a sampler, which
    //! has no components of its own.
    std::uint8_t rows;
    //! \brief What kind of number the components are.
    ScalarType scalar;
    //! \brief What family it belongs to.
    Family family;
};

// ----------------------------------------------------------------------------
//! \brief One row per type, in the order the enum declares them.
//!
//! A table rather than four switches on the same enum. Adding a type means adding
//! one line here, and the checks below refuse to compile if that line is missing
//! or out of order, which a set of switches could not promise.
// ----------------------------------------------------------------------------
constexpr TypeInfo TYPES[] = {
    { DataType::Float, "float", 1u, 1u, ScalarType::Float, Family::Numeric },
    { DataType::Vec2, "vec2", 1u, 2u, ScalarType::Float, Family::Numeric },
    { DataType::Vec3, "vec3", 1u, 3u, ScalarType::Float, Family::Numeric },
    { DataType::Vec4, "vec4", 1u, 4u, ScalarType::Float, Family::Numeric },

    { DataType::Double, "double", 1u, 1u, ScalarType::Double, Family::Numeric },
    { DataType::DVec2, "dvec2", 1u, 2u, ScalarType::Double, Family::Numeric },
    { DataType::DVec3, "dvec3", 1u, 3u, ScalarType::Double, Family::Numeric },
    { DataType::DVec4, "dvec4", 1u, 4u, ScalarType::Double, Family::Numeric },

    { DataType::Int, "int", 1u, 1u, ScalarType::Int32, Family::Numeric },
    { DataType::IVec2, "ivec2", 1u, 2u, ScalarType::Int32, Family::Numeric },
    { DataType::IVec3, "ivec3", 1u, 3u, ScalarType::Int32, Family::Numeric },
    { DataType::IVec4, "ivec4", 1u, 4u, ScalarType::Int32, Family::Numeric },

    { DataType::UInt, "uint", 1u, 1u, ScalarType::UInt32, Family::Numeric },
    { DataType::UVec2, "uvec2", 1u, 2u, ScalarType::UInt32, Family::Numeric },
    { DataType::UVec3, "uvec3", 1u, 3u, ScalarType::UInt32, Family::Numeric },
    { DataType::UVec4, "uvec4", 1u, 4u, ScalarType::UInt32, Family::Numeric },

    // A GLSL bool is four bytes on the device, so it travels as an int.
    { DataType::Bool, "bool", 1u, 1u, ScalarType::Int32, Family::Numeric },
    { DataType::BVec2, "bvec2", 1u, 2u, ScalarType::Int32, Family::Numeric },
    { DataType::BVec3, "bvec3", 1u, 3u, ScalarType::Int32, Family::Numeric },
    { DataType::BVec4, "bvec4", 1u, 4u, ScalarType::Int32, Family::Numeric },

    { DataType::Mat2, "mat2", 2u, 2u, ScalarType::Float, Family::Numeric },
    { DataType::Mat3, "mat3", 3u, 3u, ScalarType::Float, Family::Numeric },
    { DataType::Mat4, "mat4", 4u, 4u, ScalarType::Float, Family::Numeric },
    { DataType::Mat2x3, "mat2x3", 2u, 3u, ScalarType::Float, Family::Numeric },
    { DataType::Mat2x4, "mat2x4", 2u, 4u, ScalarType::Float, Family::Numeric },
    { DataType::Mat3x2, "mat3x2", 3u, 2u, ScalarType::Float, Family::Numeric },
    { DataType::Mat3x4, "mat3x4", 3u, 4u, ScalarType::Float, Family::Numeric },
    { DataType::Mat4x2, "mat4x2", 4u, 2u, ScalarType::Float, Family::Numeric },
    { DataType::Mat4x3, "mat4x3", 4u, 3u, ScalarType::Float, Family::Numeric },

    { DataType::Sampler1D, "sampler1D", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::Sampler2D, "sampler2D", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::Sampler3D, "sampler3D", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::SamplerCube, "samplerCube", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::Sampler1DArray, "sampler1DArray", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::Sampler2DArray, "sampler2DArray", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::SamplerCubeArray, "samplerCubeArray", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::Sampler2DShadow, "sampler2DShadow", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::SamplerCubeShadow, "samplerCubeShadow", 1u, 0u,
      ScalarType::Float, Family::Sampler },
    { DataType::SamplerBuffer, "samplerBuffer", 1u, 0u, ScalarType::Float,
      Family::Sampler },
    { DataType::ISampler2D, "isampler2D", 1u, 0u, ScalarType::Int32,
      Family::Sampler },
    { DataType::ISampler3D, "isampler3D", 1u, 0u, ScalarType::Int32,
      Family::Sampler },
    { DataType::USampler2D, "usampler2D", 1u, 0u, ScalarType::UInt32,
      Family::Sampler },
    { DataType::USampler3D, "usampler3D", 1u, 0u, ScalarType::UInt32,
      Family::Sampler },

    { DataType::Image1D, "image1D", 1u, 0u, ScalarType::Float, Family::Image },
    { DataType::Image2D, "image2D", 1u, 0u, ScalarType::Float, Family::Image },
    { DataType::Image3D, "image3D", 1u, 0u, ScalarType::Float, Family::Image },
    { DataType::ImageCube, "imageCube", 1u, 0u, ScalarType::Float,
      Family::Image },
    { DataType::Image2DArray, "image2DArray", 1u, 0u, ScalarType::Float,
      Family::Image },
    { DataType::IImage2D, "iimage2D", 1u, 0u, ScalarType::Int32,
      Family::Image },
    { DataType::UImage2D, "uimage2D", 1u, 0u, ScalarType::UInt32,
      Family::Image },

    { DataType::Unknown, "unknown", 1u, 0u, ScalarType::Float,
      Family::Unknown },
};

//! \brief The table must hold every type of the enum, Unknown being the last.
static_assert(std::size(TYPES) ==
                  static_cast<std::size_t>(DataType::Unknown) + 1u,
              "a DataType was added without its row in the TYPES table of "
              "Reflection.cpp");

// ----------------------------------------------------------------------------
//! \brief Every row must sit at the position of the type it describes, since
//! that is how a type is looked up.
// ----------------------------------------------------------------------------
constexpr bool tableIsInOrder()
{
    for (std::size_t i = 0u; i < std::size(TYPES); ++i)
    {
        if (TYPES[i].type != static_cast<DataType>(i))
        {
            return false;
        }
    }
    return true;
}

static_assert(tableIsInOrder(),
              "the rows of the TYPES table of Reflection.cpp are not in the "
              "order the DataType enum declares them");

//------------------------------------------------------------------------------
TypeInfo const& infoOf(DataType p_type)
{
    const auto index = static_cast<std::size_t>(p_type);
    if (index >= std::size(TYPES))
    {
        return TYPES[static_cast<std::size_t>(DataType::Unknown)];
    }
    return TYPES[index];
}

} // namespace

//------------------------------------------------------------------------------
const char* toString(DataType p_type)
{
    return infoOf(p_type).name;
}

//------------------------------------------------------------------------------
bool isSampler(DataType p_type)
{
    return infoOf(p_type).family == Family::Sampler;
}

//------------------------------------------------------------------------------
bool isImage(DataType p_type)
{
    return infoOf(p_type).family == Family::Image;
}

//------------------------------------------------------------------------------
bool isMatrix(DataType p_type)
{
    return infoOf(p_type).columns > 1u;
}

//------------------------------------------------------------------------------
std::uint8_t columnsOf(DataType p_type)
{
    return infoOf(p_type).columns;
}

//------------------------------------------------------------------------------
std::uint8_t rowsOf(DataType p_type)
{
    return infoOf(p_type).rows;
}

//------------------------------------------------------------------------------
ScalarType scalarOf(DataType p_type)
{
    return infoOf(p_type).scalar;
}

//------------------------------------------------------------------------------
AttributeFormat attributeFormatOf(DataType p_type)
{
    TypeInfo const& info = infoOf(p_type);

    AttributeFormat format;
    format.scalar = info.scalar;
    // The hardware reads a matrix attribute one column per slot, so a mat4 takes
    // four slots of four components each. That is why rows and columns are kept
    // apart rather than collapsed into a component count.
    format.components = info.rows;
    format.slots = info.columns;
    format.normalized = false;
    format.as_integer = isInteger(info.scalar);
    return format;
}

//------------------------------------------------------------------------------
std::size_t BlockMember::bytes() const
{
    if (elements > 1)
    {
        return array_stride * static_cast<std::size_t>(elements);
    }
    if (isMatrix(type) && (matrix_stride != 0u))
    {
        return matrix_stride * columnsOf(type);
    }
    return sizeOf(scalarOf(type)) * rowsOf(type) * columnsOf(type);
}

//------------------------------------------------------------------------------
BlockMember const* BlockInfo::find(std::string_view p_name) const
{
    auto it = std::find_if(members.begin(),
                           members.end(),
                           [p_name](BlockMember const& p_member) {
                               return p_member.name == p_name;
                           });
    return (it == members.end()) ? nullptr : &(*it);
}

//------------------------------------------------------------------------------
AttributeInfo const* ProgramReflection::attribute(std::string_view p_name) const
{
    auto it = std::find_if(attributes.begin(),
                           attributes.end(),
                           [p_name](AttributeInfo const& p_attribute) {
                               return p_attribute.name == p_name;
                           });
    return (it == attributes.end()) ? nullptr : &(*it);
}

//------------------------------------------------------------------------------
UniformInfo const* ProgramReflection::uniform(std::string_view p_name) const
{
    auto it = std::find_if(uniforms.begin(),
                           uniforms.end(),
                           [p_name](UniformInfo const& p_uniform) {
                               return p_uniform.name == p_name;
                           });
    return (it == uniforms.end()) ? nullptr : &(*it);
}

//------------------------------------------------------------------------------
BlockInfo const* ProgramReflection::uniformBlock(std::string_view p_name) const
{
    auto it = std::find_if(uniform_blocks.begin(),
                           uniform_blocks.end(),
                           [p_name](BlockInfo const& p_block) {
                               return p_block.name == p_name;
                           });
    return (it == uniform_blocks.end()) ? nullptr : &(*it);
}

//------------------------------------------------------------------------------
BlockInfo const* ProgramReflection::storageBlock(std::string_view p_name) const
{
    auto it = std::find_if(storage_blocks.begin(),
                           storage_blocks.end(),
                           [p_name](BlockInfo const& p_block) {
                               return p_block.name == p_name;
                           });
    return (it == storage_blocks.end()) ? nullptr : &(*it);
}

//------------------------------------------------------------------------------
std::vector<UniformInfo const*> ProgramReflection::samplers() const
{
    std::vector<UniformInfo const*> found;
    for (UniformInfo const& one : uniforms)
    {
        if (isSampler(one.type) || isImage(one.type))
        {
            found.emplace_back(&one);
        }
    }
    return found;
}

//------------------------------------------------------------------------------
std::string ProgramReflection::toString() const
{
    std::string text;

    text += "attributes:";
    if (attributes.empty())
    {
        text += " none";
    }
    for (AttributeInfo const& one : attributes)
    {
        text += "\n  location " + std::to_string(one.location) + ": " +
                compages::gpu::toString(one.type) + " " + one.name;
    }

    text += "\nuniforms:";
    if (uniforms.empty())
    {
        text += " none";
    }
    for (UniformInfo const& one : uniforms)
    {
        text += "\n  location " + std::to_string(one.location) + ": " +
                compages::gpu::toString(one.type) + " " + one.name;
        if (one.elements > 1)
        {
            text += "[" + std::to_string(one.elements) + "]";
        }
    }

    auto describeBlocks = [&text](const char* p_what,
                                  std::vector<BlockInfo> const& p_blocks) {
        text += std::string("\n") + p_what + ":";
        if (p_blocks.empty())
        {
            text += " none";
        }
        for (BlockInfo const& block : p_blocks)
        {
            text += "\n  " + block.name + ", binding " +
                    std::to_string(block.binding) + ", " +
                    std::to_string(block.bytes) + " bytes";
            for (BlockMember const& member : block.members)
            {
                text += "\n    offset " + std::to_string(member.offset) + ": " +
                        compages::gpu::toString(member.type) + " " + member.name;
                if (member.elements > 1)
                {
                    text += "[" + std::to_string(member.elements) +
                            "], array stride " +
                            std::to_string(member.array_stride);
                }
                if (isMatrix(member.type))
                {
                    text += ", matrix stride " +
                            std::to_string(member.matrix_stride);
                }
            }
        }
    };
    describeBlocks("uniform blocks", uniform_blocks);
    describeBlocks("storage blocks", storage_blocks);

    if (work_group_size[0] != 0)
    {
        text += "\nwork group size: " + std::to_string(work_group_size[0]) +
                " x " + std::to_string(work_group_size[1]) + " x " +
                std::to_string(work_group_size[2]);
    }

    return text;
}

} // namespace compages::gpu
