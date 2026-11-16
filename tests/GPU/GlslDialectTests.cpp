// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The translation of GLSL 4.50 for the GL41 and GLES3 backends. Text only:
// no device needed.

#include "GPU/Internal/GlslDialect.hpp"

#include <gtest/gtest.h>

using compages::gpu::detail::GlslDialect;
using compages::gpu::detail::translateGlsl;

namespace
{

constexpr const char* SHADER = R"(#version 450 core
layout(std140, binding = 2) uniform Camera { mat4 view; } camera;
// layout(binding = 7) uniform sampler2D commented;
layout(binding = 10) uniform sampler3D fog;
layout(location = 0) out vec4 oColor;
void main() { oColor = texture(fog, vec3(0.5)) * camera.view[0]; }
)";

} // namespace

// ----------------------------------------------------------------------------
TEST(GlslDialect, DesktopLowersTheVersionAndMovesTheBindings)
{
    auto out = translateGlsl(SHADER, GlslDialect::Desktop410);

    EXPECT_EQ(out.source_version, 450);
    EXPECT_EQ(out.text.rfind("#version 410 core\n", 0u), 0u) << out.text;
    EXPECT_NE(out.text.find("#define COMPAGES_NO_STORAGE 1\n#line 2\n"), std::string::npos) << out.text;
    // The block keeps its other qualifiers; the sampler loses its layout.
    EXPECT_NE(out.text.find("layout(std140) uniform Camera"), std::string::npos) << out.text;
    EXPECT_NE(out.text.find("\n uniform sampler3D fog;"), std::string::npos) << out.text;
    EXPECT_NE(out.text.find("layout(location = 0) out vec4 oColor;"), std::string::npos);
    // A comment is never changed.
    EXPECT_NE(out.text.find("// layout(binding = 7) uniform sampler2D commented;"), std::string::npos);

    ASSERT_EQ(out.bindings.size(), 2u);
    EXPECT_EQ(out.bindings[0].name, "Camera");
    EXPECT_EQ(out.bindings[0].binding, 2);
    EXPECT_TRUE(out.bindings[0].block);
    EXPECT_EQ(out.bindings[1].name, "fog");
    EXPECT_EQ(out.bindings[1].binding, 10);
    EXPECT_FALSE(out.bindings[1].block);
}

// ----------------------------------------------------------------------------
TEST(GlslDialect, EsGetsItsVersionAndPrecisions)
{
    auto out = translateGlsl(SHADER, GlslDialect::Es300);

    EXPECT_EQ(out.text.rfind("#version 300 es\nprecision highp float;", 0u), 0u) << out.text;
    EXPECT_NE(out.text.find("precision highp sampler3D;"), std::string::npos);
    EXPECT_EQ(out.bindings.size(), 2u);
}

// ----------------------------------------------------------------------------
TEST(GlslDialect, LineNumbersFollowTheSource)
{
    // After the version and the extensions; the next line keeps its number.
    auto out = translateGlsl("#version 450 core\n#extension GL_X : enable\nvoid main() {}\n",
                             GlslDialect::Desktop410);
    EXPECT_NE(out.text.find("#extension GL_X : enable\n#define COMPAGES_NO_STORAGE 1\n#line 3\nvoid main"),
              std::string::npos)
        << out.text;

    // A #line of the source counts: the line after `#line 40` is line 40.
    auto moved = translateGlsl("#line 40\n#version 450 core\nvoid main() {}\n", GlslDialect::Desktop410);
    EXPECT_NE(moved.text.find("#define COMPAGES_NO_STORAGE 1\n#line 41\nvoid main"), std::string::npos)
        << moved.text;
}

// ----------------------------------------------------------------------------
TEST(GlslDialect, KeepsWhatHasNoEquivalent)
{
    // A storage block keeps its binding: the driver refuses it with its own
    // words. An older desktop version is kept.
    auto out = translateGlsl("#version 330 core\nlayout(std430, binding = 1) buffer B { float v[]; };\n",
                             GlslDialect::Desktop410);
    EXPECT_EQ(out.text.rfind("#version 330 core\n", 0u), 0u);
    EXPECT_NE(out.text.find("layout(std430, binding = 1) buffer B"), std::string::npos);
    EXPECT_TRUE(out.bindings.empty());

    // Already ES: unchanged version.
    auto es = translateGlsl("#version 300 es\nvoid main() {}\n", GlslDialect::Es300);
    EXPECT_EQ(es.text.rfind("#version 300 es\nprecision", 0u), 0u);
}
