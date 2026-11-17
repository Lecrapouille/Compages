// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The rewriting of a compute shader for the GL41 and GLES3 backends, and the
// small preprocessor it uses. Text only: no device needed. The dispatches
// themselves are checked by ComputeTests on those backends.

#include "Compages/GPU/ShaderSource.hpp"
#include "GPU/Internal/ComputeEmulation.hpp"
#include "GPU/Internal/GlslText.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using compages::gpu::detail::emulateCompute;
using compages::gpu::detail::keepActiveLines;

namespace
{

bool has(std::string const& p_text, std::string const& p_part)
{
    return p_text.find(p_part) != std::string::npos;
}

} // namespace

// ----------------------------------------------------------------------------
TEST(ComputeEmulation, RewritesAnInPlaceBlock)
{
    auto out = emulateCompute(R"(#version 450 core
layout(local_size_x = 16) in;
layout(std430, binding = 3) buffer Values { int values[]; };
void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= uint(values.length())) { return; }
    values[i] = (values[i] + int(i) + 1) % 60;
}
)");
    ASSERT_TRUE(bool(out)) << out.error();
    std::string const& text = out.value().text;
    EXPECT_EQ(out.value().local_size[0], 16);
    ASSERT_EQ(out.value().blocks.size(), 1u);
    EXPECT_EQ(out.value().blocks[0].block, "Values");
    EXPECT_EQ(out.value().blocks[0].binding, 3);
    EXPECT_EQ(out.value().blocks[0].stride, 4u);
    EXPECT_TRUE(out.value().blocks[0].written);
    EXPECT_TRUE(has(text, "#define COMPAGES_EMULATED_COMPUTE 1")) << text;
    EXPECT_TRUE(has(text, "compages_out_values[compages_slot(int(i))] = (compages_read_values(int(i)) + int(i) + 1) % 60;")) << text;
    EXPECT_TRUE(has(text, "uint(compages_count_values)")) << text;
    EXPECT_TRUE(has(text, "uint i = compages_GlobalInvocationID.x;")) << text;
    EXPECT_TRUE(has(text, "void compages_main()")) << text;
    // An integer leaves the vertex shader flat.
    EXPECT_TRUE(has(text, "flat out int compages_tf_values_0;")) << text;
    EXPECT_FALSE(has(text, "local_size_x")) << text;
}

// ----------------------------------------------------------------------------
TEST(ComputeEmulation, LaysOutAStructAsStd430)
{
    // vec3 at 0, float at 12, vec2 at 16: 24 bytes, rounded to 32 (a vec3
    // aligns the struct on 16). The record pads the last 8 bytes.
    auto out = emulateCompute(R"(#version 450 core
layout(local_size_x = 64) in;
struct Body { vec3 position; float mass; vec2 spin; };
layout(std430) buffer Bodies { Body bodies[]; };
void main() { bodies[gl_GlobalInvocationID.x].mass *= 2.0; }
)");
    ASSERT_TRUE(bool(out)) << out.error();
    ASSERT_EQ(out.value().blocks.size(), 1u);
    EXPECT_EQ(out.value().blocks[0].stride, 32u);
    std::vector<std::string> const& outputs = out.value().blocks[0].outputs;
    ASSERT_EQ(outputs.size(), 5u);
    EXPECT_EQ(outputs[0], "compages_tf_bodies_0");
    EXPECT_EQ(outputs[3], "compages_pad_bodies_0");
    EXPECT_TRUE(has(out.value().text, "compages_out_bodies[compages_slot(int(compages_GlobalInvocationID.x))].mass *= 2.0;")) << out.value().text;
}

// ----------------------------------------------------------------------------
TEST(ComputeEmulation, SeveralWrittenBlocksGoToSeparateBuffers)
{
    auto out = emulateCompute(R"(#version 450 core
layout(local_size_x = 64) in;
layout(std430, binding = 0) readonly buffer In { vec2 velocityIn[]; };
layout(std430, binding = 1) writeonly buffer VOut { vec2 velocityOut[]; };
layout(std430, binding = 2) writeonly buffer SOut { float scalarOut[]; };
void main()
{
    uint k = gl_GlobalInvocationID.x;
    velocityOut[k] = velocityIn[k + 1u];
    scalarOut[k] = velocityIn[k].x;
}
)");
    ASSERT_TRUE(bool(out)) << out.error();
    EXPECT_TRUE(out.value().separate);
    EXPECT_FALSE(out.value().blocks[0].written);
    EXPECT_TRUE(has(out.value().text, "compages_out_velocityOut[compages_slot(int(k))] = compages_read_velocityIn(int(k + 1u));"))
        << out.value().text;
}

// ----------------------------------------------------------------------------
TEST(ComputeEmulation, OnlyTheBlocksOfTheActiveBranchExist)
{
    // One source, several programs: the block of step 1 is not in step 2.
    auto out = emulateCompute(R"(#version 450 core
#define STEP 2
layout(local_size_x = 64) in;
#if (STEP == 1)
layout(std430, binding = 0) buffer A { float a[]; };
#elif (STEP == 2) || defined(OTHER)
layout(std430, binding = 1) buffer B { float b[]; };
#endif
void main()
{
#if STEP == 1
    a[gl_GlobalInvocationID.x] = 1.0;
#else
    b[gl_GlobalInvocationID.x] = 2.0;
#endif
}
)");
    ASSERT_TRUE(bool(out)) << out.error();
    ASSERT_EQ(out.value().blocks.size(), 1u);
    EXPECT_EQ(out.value().blocks[0].block, "B");
}

// ----------------------------------------------------------------------------
TEST(ComputeEmulation, RefusesWhatHasNoEquivalent)
{
    auto shared = emulateCompute("#version 450 core\nlayout(local_size_x = 64) in;\n"
                                 "layout(std430) buffer B { float b[]; };\nshared float tile[64];\n"
                                 "void main() { b[0] = tile[0]; }\n");
    ASSERT_FALSE(bool(shared));
    EXPECT_TRUE(has(shared.error(), "COMPAGES_FOR_EACH")) << shared.error();

    auto atomic = emulateCompute("#version 450 core\nlayout(local_size_x = 64) in;\n"
                                 "layout(std430) buffer B { uint b[]; };\n"
                                 "void main() { atomicAdd(b[0], 1u); }\n");
    ASSERT_FALSE(bool(atomic));
    EXPECT_TRUE(has(atomic.error(), "atomicAdd")) << atomic.error();

    // An invocation that writes several elements (05g_NeuralNetwork) would
    // lose all but one: refused. One fixed index is checked when running.
    auto several = emulateCompute("#version 450 core\nlayout(local_size_x = 64) in;\n"
                                  "layout(std430) buffer G { float g[]; };\n"
                                  "void main() { uint b = gl_GlobalInvocationID.x * 2u; g[b] = 1.0; g[b + 1u] = 2.0; }\n");
    ASSERT_FALSE(bool(several));
    EXPECT_TRUE(has(several.error(), "several indices")) << several.error();
    auto fixed = emulateCompute("#version 450 core\nlayout(local_size_x = 64) in;\n"
                                "layout(std430) buffer S { float stats[]; };\n"
                                "void main() { stats[0] = 1.0; }\n");
    ASSERT_TRUE(bool(fixed)) << fixed.error();
    EXPECT_TRUE(has(fixed.value().text, "compages_out_stats[compages_slot(int(0))] = 1.0;")) << fixed.value().text;

    auto image = emulateCompute("#version 450 core\nlayout(local_size_x = 8, local_size_y = 8) in;\n"
                                "layout(rgba8) writeonly uniform image2D picture;\n"
                                "layout(std430) buffer B { float b[]; };\n"
                                "void main() { b[0] = 1.0; imageStore(picture, ivec2(0), vec4(1.0)); }\n");
    ASSERT_FALSE(bool(image));
    EXPECT_TRUE(has(image.error(), "image")) << image.error();
}

// ----------------------------------------------------------------------------
TEST(ComputeEmulation, TilesGlslIsADirectLoopWithoutCompute)
{
    // The include of the library resolves without being given, and its
    // shared branch is gone once COMPAGES_EMULATED_COMPUTE is set.
    auto resolved = compages::gpu::ShaderSource::fromText("t.comp", "#version 450 core\n#include <compages/tiles.glsl>\n")
                        .resolve();
    ASSERT_TRUE(bool(resolved)) << resolved.error();
    std::string error;
    const std::string active = keepActiveLines(resolved.value().text, { "COMPAGES_EMULATED_COMPUTE" }, error);
    ASSERT_TRUE(error.empty()) << error;
    EXPECT_FALSE(has(active, "shared")) << active;
    EXPECT_FALSE(has(active, "barrier")) << active;
    EXPECT_TRUE(has(active, "for (uint INDEX = 0u; INDEX < uint(COUNT); ++INDEX)")) << active;
}

// ----------------------------------------------------------------------------
TEST(GlslText, KeepActiveLinesKeepsTheLineNumbers)
{
    const std::string source = "a\n#ifdef X\nb\n#else\nc\n#endif\n#if 2 * 3 == 6 && !defined(Y)\nd\n#endif\n";
    std::string error;
    const std::string out = keepActiveLines(source, { "X" }, error);
    ASSERT_TRUE(error.empty()) << error;
    EXPECT_EQ(std::count(out.begin(), out.end(), '\n'), std::count(source.begin(), source.end(), '\n'));
    EXPECT_TRUE(has(out, "b"));
    EXPECT_FALSE(has(out, "c"));
    EXPECT_TRUE(has(out, "d"));
    EXPECT_FALSE(has(out, "#"));

    std::string broken;
    (void)keepActiveLines("#if (1\n#endif\n", {}, broken);
    EXPECT_FALSE(broken.empty());
}

// ----------------------------------------------------------------------------
// A drawing shader reading a storage block: reads rewritten, main kept,
// nothing captured; a write refused.
// ----------------------------------------------------------------------------
TEST(ComputeEmulation, ADrawingShaderReadsItsBlocksFromCopies)
{
    constexpr const char* FRAGMENT = R"(#version 450 core
layout(std430, binding = 9) readonly buffer Lamps { vec4 lamps[]; };
out vec4 oColor;
void main()
{
    oColor = (lamps.length() > 0) ? lamps[1] : vec4(0.0);
}
)";
    EXPECT_TRUE(compages::gpu::detail::declaresStorageBlock(FRAGMENT));
    EXPECT_FALSE(compages::gpu::detail::declaresStorageBlock("#version 450 core\n// buffer B { float b[]; };\nvoid main() {}\n"));

    auto out = compages::gpu::detail::emulateStorageReads(FRAGMENT, {});
    ASSERT_TRUE(bool(out)) << out.error();
    std::string const& text = out.value().text;
    ASSERT_EQ(out.value().blocks.size(), 1u);
    EXPECT_EQ(out.value().blocks[0].binding, 9);
    EXPECT_FALSE(out.value().blocks[0].written);
    EXPECT_TRUE(has(text, "oColor = (compages_count_lamps > 0) ? compages_read_lamps(int(1)) : vec4(0.0);")) << text;
    EXPECT_TRUE(has(text, "void main()")) << text;
    EXPECT_FALSE(has(text, "compages_main")) << text;
    EXPECT_FALSE(has(text, "compages_tf_")) << text;

    auto writes = compages::gpu::detail::emulateStorageReads(
        "#version 450 core\nlayout(std430) buffer B { float b[]; };\nvoid main() { b[0] = 1.0; }\n", {});
    ASSERT_FALSE(bool(writes));
    EXPECT_TRUE(has(writes.error(), "OpenGL 4.3")) << writes.error();
}
