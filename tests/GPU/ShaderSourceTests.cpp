// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"

#include <fstream>

using namespace tests;
using compages::gpu::ShaderSource;

namespace
{

constexpr const char* MAIN = R"(#version 450 core
#include <colour.glsl>
out vec4 oColor;
void main() { oColor = vec4(colour(), 1.0); }
)";

//! \brief Resolve and fail the test when it does not.
compages::gpu::ResolvedSource resolved(ShaderSource const& p_source)
{
    auto result = p_source.resolve();
    EXPECT_TRUE(bool(result)) << result.error();
    return result ? result.take() : compages::gpu::ResolvedSource{};
}

//! \brief A folder of the test, emptied first.
std::filesystem::path freshFolder(std::string const& p_name)
{
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path() / ("compages-" + p_name);
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    return folder;
}

void write(std::filesystem::path const& p_path, std::string const& p_text)
{
    std::ofstream(p_path) << p_text;
}

} // namespace

// The included text replaces the line, each piece gets a #line number, and the
// line after the include goes back to its place in the main text.
TEST(ShaderSource, PutsAGivenIncludeIn)
{
    auto source = ShaderSource::fromText("main.frag", MAIN)
                      .include("colour.glsl", "vec3 colour() { return vec3(1.0); }");
    const compages::gpu::ResolvedSource out = resolved(source);

    EXPECT_EQ(out.text.rfind("#version 450 core\n", 0u), 0u);
    EXPECT_THAT(out.text, HasSubstr("#line 1 1\nvec3 colour()"));
    EXPECT_THAT(out.text, HasSubstr("#line 3 0\nout vec4 oColor;"));
    EXPECT_THAT(out.text, Not(HasSubstr("#include")));
    ASSERT_EQ(out.pieces.size(), 2u);
    EXPECT_EQ(out.pieces[0], "main.frag");
    EXPECT_EQ(out.pieces[1], "colour.glsl");
}

TEST(ShaderSource, PutsTheDefinesJustAfterTheVersion)
{
    auto source = ShaderSource::fromText("main.frag", MAIN)
                      .include("colour.glsl", "vec3 colour() { return vec3(N); }")
                      .define("N", "0.5")
                      .define("FLAG", "");
    const compages::gpu::ResolvedSource out = resolved(source);

    EXPECT_EQ(out.text.rfind("#version 450 core\n#define N 0.5\n#define FLAG\n#line 2 0\n", 0u),
              0u)
        << out.text;
}

TEST(ShaderSource, RefusesATextWithoutVersion)
{
    auto result = ShaderSource::fromText("bare.frag", "void main() {}").resolve();
    ASSERT_FALSE(bool(result));
    EXPECT_THAT(result.error(), HasSubstr("bare.frag"));
    EXPECT_THAT(result.error(), HasSubstr("#version"));
}

TEST(ShaderSource, RefusesAnIncludeThatIsNowhere)
{
    auto result = ShaderSource::fromText("main.frag", MAIN).resolve();
    ASSERT_FALSE(bool(result));
    EXPECT_THAT(result.error(), HasSubstr("colour.glsl"));
}

// A name met twice is put in once, like a header with #pragma once.
TEST(ShaderSource, PutsEachNameInOnce)
{
    auto source = ShaderSource::fromText("main.frag", "#version 450 core\n#include <a>\n#include <b>\n")
                      .include("a", "#include <common>\nA\n")
                      .include("b", "#include <common>\nB\n")
                      .include("common", "COMMON\n");
    const compages::gpu::ResolvedSource out = resolved(source);

    EXPECT_EQ(out.text.find("COMMON"), out.text.rfind("COMMON"));
    EXPECT_EQ(out.pieces.size(), 4u);
}

TEST(ShaderSource, ReportsACircle)
{
    auto source = ShaderSource::fromText("main.frag", "#version 450 core\n#include <a>\n")
                      .include("a", "#include <b>\n")
                      .include("b", "#include <a>\n");
    auto result = source.resolve();
    ASSERT_FALSE(bool(result));
    EXPECT_THAT(result.error(), HasSubstr("circle"));
    EXPECT_THAT(result.error(), HasSubstr("a -> b -> a"));
}

TEST(ShaderSource, RefusesAVersionInAnInclude)
{
    auto source = ShaderSource::fromText("main.frag", MAIN)
                      .include("colour.glsl", "#version 450 core\nvec3 colour();\n");
    auto result = source.resolve();
    ASSERT_FALSE(bool(result));
    EXPECT_THAT(result.error(), HasSubstr("colour.glsl"));
}

// Files: "name" beside the file that includes it first, then the directories;
// every file read is listed for a hot reload.
TEST(ShaderSource, ReadsFilesBesideAndInDirectories)
{
    const std::filesystem::path folder = freshFolder("shader-source");
    std::filesystem::create_directories(folder / "common");
    write(folder / "main.frag", "#version 450 core\n#include \"near.glsl\"\n#include <far.glsl>\n");
    write(folder / "near.glsl", "NEAR\n");
    write(folder / "common" / "far.glsl", "FAR\n");

    auto source = ShaderSource::fromFile(folder / "main.frag").directory(folder / "common");
    const compages::gpu::ResolvedSource out = resolved(source);

    EXPECT_THAT(out.text, HasSubstr("NEAR"));
    EXPECT_THAT(out.text, HasSubstr("FAR"));
    EXPECT_EQ(out.files.size(), 3u);
    std::filesystem::remove_all(folder);
}

TEST(ShaderSource, AGivenIncludeWinsOverAFile)
{
    const std::filesystem::path folder = freshFolder("shader-source-wins");
    write(folder / "colour.glsl", "FROM_FILE\n");
    auto source = ShaderSource::fromText("main.frag", MAIN)
                      .directory(folder)
                      .include("colour.glsl", "FROM_CPP\n");
    const compages::gpu::ResolvedSource out = resolved(source);

    EXPECT_THAT(out.text, HasSubstr("FROM_CPP"));
    EXPECT_THAT(out.text, Not(HasSubstr("FROM_FILE")));
    std::filesystem::remove_all(folder);
}

TEST(ShaderSource, AHookHasADefaultThatCanBeReplaced)
{
    constexpr const char* TEXT = "#version 450 core\n#include <hook/deform>\n";
    auto plain = ShaderSource::fromText("pbr.vert", TEXT).declareHook("deform", "DEFAULT\n");
    EXPECT_THAT(resolved(plain).text, HasSubstr("DEFAULT"));

    auto waves = ShaderSource::fromText("pbr.vert", TEXT)
                     .declareHook("deform", "DEFAULT\n")
                     .hook("deform", "WAVES\n");
    const compages::gpu::ResolvedSource out = resolved(waves);
    EXPECT_THAT(out.text, HasSubstr("WAVES"));
    EXPECT_THAT(out.text, Not(HasSubstr("DEFAULT")));
}

// A typo in a hook name is an error that lists the hooks there are.
TEST(ShaderSource, RefusesAnUnknownHook)
{
    auto source = ShaderSource::fromText("pbr.vert", "#version 450 core\n#include <hook/deform>\n")
                      .declareHook("deform", "\n")
                      .hook("deforme", "WAVES\n");
    auto result = source.resolve();
    ASSERT_FALSE(bool(result));
    EXPECT_THAT(result.error(), HasSubstr("deforme"));
    EXPECT_THAT(result.error(), HasSubstr("known: deform"));
}

TEST(ShaderSource, ExplainsTheLogWithThePieceNames)
{
    compages::gpu::ResolvedSource out;
    out.pieces = { "main.frag", "water/waves.glsl" };
    EXPECT_EQ(out.explain("1:12(5): error: oops"), "water/waves.glsl:12(5): error: oops\n");
    EXPECT_EQ(out.explain("1(12) : error C0000: oops"), "water/waves.glsl:12 : error C0000: oops\n");
    EXPECT_EQ(out.explain("ERROR: 0:3: oops"), "ERROR: main.frag:3: oops\n");
    // A number that is no piece stays.
    EXPECT_EQ(out.explain("7:3: oops"), "7:3: oops\n");
}

// ****************************************************************************
//! \brief The same, through the driver.
// ****************************************************************************
class ShaderSourceGpuTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        if (IsSkipped())
        {
            return;
        }
        auto ready = compages::gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();
    }

    void TearDown() override
    {
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }
};

TEST_F(ShaderSourceGpuTest, AnErrorInAnIncludeNamesIt)
{
    auto source = ShaderSource::fromText("main.frag", MAIN)
                      .include("colour.glsl", "vec3 colour()\n{\n    return no_such_thing;\n}\n");
    auto shader = compages::gpu::Shader::fromSource(compages::gpu::ShaderStage::Fragment, source);
    ASSERT_FALSE(bool(shader));
    EXPECT_THAT(shader.error(), HasSubstr("colour.glsl:3"));
    EXPECT_EQ(compages::gpu::liveShaders(), 0u);
}

TEST_F(ShaderSourceGpuTest, LoadsAProgram)
{
    auto vertex = ShaderSource::fromText("main.vert", R"(#version 450 core
#include <position.glsl>
void main() { gl_Position = vec4(position, 1.0); }
)")
                      .include("position.glsl", "in vec3 position;\n");
    auto fragment = ShaderSource::fromText("main.frag", MAIN)
                        .include("colour.glsl", "vec3 colour() { return vec3(1.0); }\n");
    compages::gpu::Program program;
    auto loaded = program.load(vertex, fragment);
    ASSERT_TRUE(bool(loaded)) << loaded.error();
    EXPECT_TRUE(program.valid());
}
