//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/Renderer/Assets/StlLoader.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{

//! \brief A unit square in XY made of two triangles facing +z.
constexpr char const* ASCII_SQUARE = R"(solid square
  facet normal 0 0 1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 1 1 0
    endloop
  endfacet
  facet normal 0 0 0
    outer loop
      vertex 0 0 0
      vertex 1 1 0
      vertex 0 1 0
    endloop
  endfacet
endsolid square
)";

std::span<const std::byte> bytesOf(std::string const& p_text)
{
    return std::as_bytes(std::span<const char>(p_text.data(), p_text.size()));
}

void put(std::vector<std::byte>& p_bytes, float p_value)
{
    std::byte raw[sizeof(float)];
    std::memcpy(raw, &p_value, sizeof(float));
    p_bytes.insert(p_bytes.end(), raw, raw + sizeof(float));
}

//! \brief The same square as a binary STL, whose header starts with "solid"
//! as some exporters write it.
std::vector<std::byte> binarySquare()
{
    std::vector<std::byte> bytes(80u, std::byte{ 0 });
    const char header[] = "solid but binary";
    std::memcpy(bytes.data(), header, sizeof(header) - 1u);
    const std::uint32_t count = 2u;
    std::byte raw[sizeof(count)];
    std::memcpy(raw, &count, sizeof(count));
    bytes.insert(bytes.end(), raw, raw + sizeof(count));

    const float facets[2][12] = {
        { 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0 },
        { 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0 },
    };
    for (auto const& facet : facets)
    {
        for (float value : facet)
        {
            put(bytes, value);
        }
        bytes.push_back(std::byte{ 0 });
        bytes.push_back(std::byte{ 0 });
    }
    return bytes;
}

void expectSquare(compages::renderer::MeshAsset const& p_mesh)
{
    // Six corners, four distinct: the diagonal is shared.
    ASSERT_EQ(p_mesh.source_indices.size(), 6u);
    ASSERT_EQ(p_mesh.index_count, 6u);
    EXPECT_EQ(p_mesh.source_vertices.size(), 4u);
    for (compages::renderer::MeshVertex const& vertex : p_mesh.source_vertices)
    {
        EXPECT_NEAR(vertex.normal.z, 1.0f, 1.0e-6f);
    }
    EXPECT_NEAR(p_mesh.local_bounds.extent().x, 0.5f, 1.0e-6f);
    EXPECT_NEAR(p_mesh.local_bounds.extent().y, 0.5f, 1.0e-6f);
}

} // namespace

//------------------------------------------------------------------------------
TEST(StlLoader, ReadsAsciiAndMergesSharedCorners)
{
    auto mesh = compages::renderer::parseStl(bytesOf(ASCII_SQUARE));
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    expectSquare(mesh.value());
}

//------------------------------------------------------------------------------
TEST(StlLoader, ReadsBinaryEvenWhenItsHeaderSaysSolid)
{
    const std::vector<std::byte> bytes = binarySquare();
    auto mesh = compages::renderer::parseStl(bytes);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    expectSquare(mesh.value());
}

//------------------------------------------------------------------------------
TEST(StlLoader, KeepsSharpEdgesApart)
{
    // Two faces of a cube meeting at a right angle: the corners of the edge
    // have the same position but not the same normal.
    const std::string text = R"(solid edge
facet normal 0 0 1
 outer loop
  vertex 0 0 0
  vertex 1 0 0
  vertex 1 1 0
 endloop
endfacet
facet normal 0 -1 0
 outer loop
  vertex 0 0 0
  vertex 1 0 -1
  vertex 1 0 0
 endloop
endfacet
endsolid edge
)";
    auto mesh = compages::renderer::parseStl(bytesOf(text));
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    EXPECT_EQ(mesh.value().source_vertices.size(), 6u);
}

//------------------------------------------------------------------------------
TEST(StlLoader, LoadsAFile)
{
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "compages_square.stl";
    {
        std::ofstream file(path, std::ios::binary);
        file << ASCII_SQUARE;
    }
    auto mesh = compages::renderer::loadStl(path.string());
    std::filesystem::remove(path);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    expectSquare(mesh.value());
}

//------------------------------------------------------------------------------
TEST(StlLoader, RejectsWhatIsNotAnStl)
{
    EXPECT_FALSE(compages::renderer::parseStl(bytesOf("hello world")));
    EXPECT_FALSE(compages::renderer::parseStl(bytesOf("solid empty\nendsolid empty\n")));
    EXPECT_FALSE(compages::renderer::parseStl(bytesOf(
        "solid bad\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\n"
        "endloop\nendfacet\nendsolid bad\n")));

    // A binary file cut short no longer matches its facet count.
    std::vector<std::byte> truncated = binarySquare();
    truncated.resize(truncated.size() - 10u);
    std::memcpy(truncated.data(), "xxxxx", 5u);
    EXPECT_FALSE(compages::renderer::parseStl(truncated));

    auto missing = compages::renderer::loadStl("/does/not/exist.stl");
    ASSERT_FALSE(missing);
    EXPECT_NE(missing.error().find("/does/not/exist.stl"), std::string::npos);
}
