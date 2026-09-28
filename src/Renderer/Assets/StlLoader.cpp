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

#include "Compages/Renderer/Assets/StlLoader.hpp"
#include "Compages/GPU/Device.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace compages::renderer
{

namespace
{

constexpr std::size_t BINARY_HEADER_BYTES = 80u;
constexpr std::size_t BINARY_PREAMBLE_BYTES = BINARY_HEADER_BYTES + 4u;
constexpr std::size_t BINARY_FACET_BYTES = 50u;

// ****************************************************************************
//! \brief One corner as read from the file, before merging.
// ****************************************************************************
struct Corner
{
    Vector3f position;
    Vector3f normal;
};

// ****************************************************************************
//! \brief The six floats of a corner, compared and hashed bit for bit.
// ****************************************************************************
struct CornerKey
{
    std::array<std::uint32_t, 6u> bits;

    explicit CornerKey(Corner const& p_corner)
    {
        const std::array<float, 6u> values = {
            p_corner.position.x, p_corner.position.y, p_corner.position.z,
            p_corner.normal.x,   p_corner.normal.y,   p_corner.normal.z
        };
        std::memcpy(bits.data(), values.data(), sizeof(values));
    }

    bool operator==(CornerKey const& p_other) const = default;
};

struct CornerHash
{
    std::size_t operator()(CornerKey const& p_key) const
    {
        std::size_t seed = 0u;
        for (std::uint32_t const word : p_key.bits)
        {
            seed ^= std::size_t(word) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
        }
        return seed;
    }
};

//------------------------------------------------------------------------------
//! \brief The normal of the facet, or that of its triangle when the file
//! stored a null one.
Vector3f facetNormal(Vector3f const& p_stored,
                     Vector3f const& p_a,
                     Vector3f const& p_b,
                     Vector3f const& p_c)
{
    constexpr float TINY = 1e-12f;
    if (compages::vector::squaredNorm(p_stored) > TINY)
    {
        return compages::vector::normalize(p_stored);
    }
    const Vector3f computed = compages::vector::cross(p_b - p_a, p_c - p_a);
    if (compages::vector::squaredNorm(computed) > TINY)
    {
        return compages::vector::normalize(computed);
    }
    return Vector3f(0.0f, 0.0f, 1.0f);
}

//------------------------------------------------------------------------------
void addFacet(std::vector<Corner>& p_corners,
              Vector3f const& p_normal,
              Vector3f const& p_a,
              Vector3f const& p_b,
              Vector3f const& p_c)
{
    const Vector3f normal = facetNormal(p_normal, p_a, p_b, p_c);
    p_corners.push_back(Corner{ p_a, normal });
    p_corners.push_back(Corner{ p_b, normal });
    p_corners.push_back(Corner{ p_c, normal });
}

//------------------------------------------------------------------------------
Vector3f readVector(std::byte const* p_at)
{
    std::array<float, 3u> xyz;
    std::memcpy(xyz.data(), p_at, sizeof(xyz));
    return Vector3f(xyz[0], xyz[1], xyz[2]);
}

//------------------------------------------------------------------------------
//! \brief A binary STL is an 80-byte header, a facet count, then 50 bytes per
//! facet. Its size is the only reliable sign: some binary files start with
//! "solid" too.
bool isBinary(std::span<const std::byte> p_bytes)
{
    if (p_bytes.size() < BINARY_PREAMBLE_BYTES)
    {
        return false;
    }
    std::uint32_t count = 0u;
    std::memcpy(&count, p_bytes.data() + BINARY_HEADER_BYTES, sizeof(count));
    return p_bytes.size() ==
           BINARY_PREAMBLE_BYTES + (std::size_t(count) * BINARY_FACET_BYTES);
}

//------------------------------------------------------------------------------
std::vector<Corner> readBinary(std::span<const std::byte> p_bytes)
{
    std::uint32_t count = 0u;
    std::memcpy(&count, p_bytes.data() + BINARY_HEADER_BYTES, sizeof(count));
    std::vector<Corner> corners;
    corners.reserve(std::size_t(count) * 3u);
    std::byte const* facet = p_bytes.data() + BINARY_PREAMBLE_BYTES;
    for (std::uint32_t i = 0u; i < count; ++i)
    {
        addFacet(corners,
                 readVector(facet),
                 readVector(facet + 12u),
                 readVector(facet + 24u),
                 readVector(facet + 36u));
        facet += BINARY_FACET_BYTES;
    }
    return corners;
}

//------------------------------------------------------------------------------
compages::Result<std::vector<Corner>>
readAscii(std::span<const std::byte> p_bytes)
{
    const std::string_view text(reinterpret_cast<char const*>(p_bytes.data()),
                                p_bytes.size());
    std::istringstream in{ std::string(text) };
    std::string word;
    if (!(in >> word) || (word != "solid"))
    {
        return compages::failure(
            "neither a binary STL (wrong size for its facet count) nor an "
            "ASCII one (no 'solid' keyword)");
    }

    std::vector<Corner> corners;
    Vector3f normal(0.0f, 0.0f, 0.0f);
    std::array<Vector3f, 3u> triangle;
    std::size_t in_triangle = 0u;
    while (in >> word)
    {
        if (word == "facet")
        {
            if (!(in >> word) || (word != "normal") ||
                !(in >> normal.x >> normal.y >> normal.z))
            {
                return compages::failure("malformed 'facet normal' in STL");
            }
            in_triangle = 0u;
        }
        else if (word == "vertex")
        {
            Vector3f& corner = triangle[in_triangle % 3u];
            if (!(in >> corner.x >> corner.y >> corner.z))
            {
                return compages::failure("malformed 'vertex' in STL");
            }
            if (++in_triangle == 3u)
            {
                addFacet(corners, normal, triangle[0], triangle[1], triangle[2]);
            }
            else if (in_triangle > 3u)
            {
                return compages::failure("an STL facet has more than three "
                                         "vertices");
            }
        }
        else if (word == "endfacet")
        {
            if (in_triangle != 3u)
            {
                return compages::failure("an STL facet has fewer than three "
                                         "vertices");
            }
        }
    }
    return corners;
}

//------------------------------------------------------------------------------
MeshAsset merge(std::vector<Corner> const& p_corners)
{
    MeshAsset mesh;
    std::unordered_map<CornerKey, std::uint32_t, CornerHash> seen;
    seen.reserve(p_corners.size());
    mesh.source_indices.reserve(p_corners.size());
    AABB bounds;
    for (Corner const& corner : p_corners)
    {
        const auto [slot, added] = seen.try_emplace(
            CornerKey(corner),
            static_cast<std::uint32_t>(mesh.source_vertices.size()));
        if (added)
        {
            MeshVertex vertex;
            vertex.position = corner.position;
            vertex.normal = corner.normal;
            mesh.source_vertices.push_back(vertex);
            bounds.expand(corner.position);
        }
        mesh.source_indices.push_back(slot->second);
    }
    mesh.index_count = mesh.source_indices.size();
    mesh.local_bounds = bounds;
    return mesh;
}

} // namespace

//------------------------------------------------------------------------------
compages::Result<MeshAsset> parseStl(std::span<const std::byte> p_bytes)
{
    std::vector<Corner> corners;
    if (isBinary(p_bytes))
    {
        corners = readBinary(p_bytes);
    }
    else
    {
        auto ascii = readAscii(p_bytes);
        if (!ascii)
        {
            return compages::failure(ascii.error());
        }
        corners = ascii.take();
    }
    if (corners.empty())
    {
        return compages::failure("the STL holds no triangle");
    }

    MeshAsset mesh = merge(corners);
    if (compages::gpu::initialized())
    {
        COMPAGES_TRY(mesh.upload());
    }
    return mesh;
}

//------------------------------------------------------------------------------
compages::Result<MeshAsset> loadStl(std::string const& p_path)
{
    std::ifstream file(p_path, std::ios::binary);
    if (!file)
    {
        return compages::failure("cannot open the STL file '" + p_path + "'");
    }
    const std::vector<char> content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    auto mesh = parseStl(std::as_bytes(std::span<const char>(content)));
    if (!mesh)
    {
        return compages::failure("'" + p_path + "': " + mesh.error());
    }
    return mesh;
}

} // namespace compages::renderer
