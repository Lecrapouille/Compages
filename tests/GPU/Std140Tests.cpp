// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"


#include "Compages/GPU/Core/Std140.hpp"



// ----------------------------------------------------------------------------
// The structs below are the ones a real program writes. GPU_STD140 is used at
// file scope, as it must be, and every one of these compiling is itself the test:
// a struct that did not match would stop the build.
// ----------------------------------------------------------------------------

//! \brief Two matrices, the easiest case: both align on 16 and occupy 64.
struct Camera
{
    compages::core::Matrix44f projection;
    compages::core::Matrix44f view;
};
GPU_STD140(Camera, projection, view);

//! \brief The arrangement that works: the vector of three first, the lone float
//! in the four bytes it leaves behind.
struct Light
{
    compages::core::Vector3f direction;
    float intensity;
};
GPU_STD140(Light, direction, intensity);

//! \brief A block ending on a vector of four, so its size is already a multiple
//! of 16 with nothing to add.
struct Material
{
    compages::core::Vector4f albedo;
    compages::core::Vector4f emissive;
};
GPU_STD140(Material, albedo, emissive);

// The rules themselves, which everything else is built on. The vector of three is
// the one worth pinning down: it aligns on 16 but occupies only 12, and that gap
// is where most std140 surprises come from.
TEST(Std140, KnowsTheAlignmentAndSizeOfEachType)
{

    ASSERT_EQ(compages::gpu::std140::Rules<float>::alignment, 4u);
    ASSERT_EQ(compages::gpu::std140::Rules<float>::size, 4u);

    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector2f>::alignment, 8u);
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector2f>::size, 8u);

    // Aligns on 16, occupies 12: the four bytes after it can hold a float.
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector3f>::alignment, 16u);
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector3f>::size, 12u);

    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector4f>::alignment, 16u);
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector4f>::size, 16u);

    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Matrix44f>::alignment, 16u);
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Matrix44f>::size, 64u);
}

// A matrix of three is the trap nobody expects: std140 pads each of its three
// columns up to 16 bytes, so it occupies 48 where C++ gives it 36. Copying such a
// struct wholesale would shift everything after it.
TEST(Std140, KnowsAMatrixOfThreeOccupiesFortyEightBytes)
{
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Matrix33f>::size, 48u);
    ASSERT_EQ(sizeof(compages::core::Matrix33f), 36u);
}

// An array is the other trap: every element is padded up to 16 bytes, so ten
// floats occupy 160 bytes rather than 40.
TEST(Std140, KnowsEveryArrayElementIsPaddedToSixteenBytes)
{

    ASSERT_EQ(compages::gpu::std140::Rules<float[10]>::size, 160u);
    ASSERT_EQ(sizeof(float[10]), 40u);

    // A vector of four already fills 16, so an array of them is the one case
    // where C++ and std140 agree.
    ASSERT_EQ(compages::gpu::std140::Rules<compages::core::Vector4f[10]>::size, 160u);
    ASSERT_EQ(sizeof(compages::core::Vector4f[10]), 160u);
}

TEST(Std140, RoundsUpTheWayTheRulesDo)
{

    ASSERT_EQ(compages::gpu::std140::roundUp(0u, 16u), 0u);
    ASSERT_EQ(compages::gpu::std140::roundUp(1u, 16u), 16u);
    ASSERT_EQ(compages::gpu::std140::roundUp(16u, 16u), 16u);
    ASSERT_EQ(compages::gpu::std140::roundUp(17u, 16u), 32u);
    ASSERT_EQ(compages::gpu::std140::roundUp(12u, 4u), 12u);
}

// The three structs above were accepted, which is the whole point: they can be
// copied into a uniform buffer in one go.
TEST(Std140, AcceptsTheStructsThatMatch)
{
    ASSERT_TRUE(compages::gpu::std140::conforms<Camera>());
    ASSERT_TRUE(compages::gpu::std140::conforms<Light>());
    ASSERT_TRUE(compages::gpu::std140::conforms<Material>());

    static_assert(compages::gpu::std140::conforms<Camera>());
    static_assert(compages::gpu::std140::conforms<Light>());
}

// A struct nobody checked must not be treated as if it had been: this is what
// makes a typed uniform block refuse to compile until GPU_STD140 has been written.
TEST(Std140, RefusesAStructThatWasNeverChecked)
{
    struct Unchecked
    {
        float value;
    };

    ASSERT_FALSE(compages::gpu::std140::described<Unchecked>());
    ASSERT_FALSE(compages::gpu::std140::conforms<Unchecked>());
}

// The case the whole file exists for. A lone float before a vector of three puts
// the vector at byte 4 in C++ and at byte 16 in std140. The mismatch is found
// without needing a struct that fails to compile, by describing the members the
// way the macro does.
TEST(Std140, CatchesTheFloatBeforeAVectorOfThree)
{

    // What C++ does with { float radius; compages::core::Vector3f position; }
    const std::array<compages::gpu::std140::Member, 2> members{
        compages::gpu::std140::Member{ "radius", 0u, 4u, 4u },
        compages::gpu::std140::Member{ "position", 4u, 16u, 12u },
    };

    ASSERT_EQ(compages::gpu::std140::firstMismatch(members), 1u);
    // std140 wants the vector at 16, and gives the whole block 32 bytes.
    ASSERT_EQ(compages::gpu::std140::expectedOffset(4u, members[1]), 16u);
    ASSERT_EQ(compages::gpu::std140::blockSize(members), 32u);
}

// The same two members the other way round agree with std140, which is why the
// advice is to put the wide members first.
TEST(Std140, AcceptsTheSameMembersInTheRightOrder)
{

    const std::array<compages::gpu::std140::Member, 2> members{
        compages::gpu::std140::Member{ "position", 0u, 16u, 12u },
        compages::gpu::std140::Member{ "radius", 12u, 4u, 4u },
    };

    ASSERT_EQ(compages::gpu::std140::firstMismatch(members), 2u);
    ASSERT_EQ(compages::gpu::std140::blockSize(members), 16u);
}

TEST(Std140, RoundsTheWholeBlockUpToSixteenBytes)
{

    // A single float occupies 4 bytes, but the block it sits in occupies 16.
    const std::array<compages::gpu::std140::Member, 1> members{ compages::gpu::std140::Member{ "value", 0u, 4u, 4u } };

    ASSERT_EQ(compages::gpu::std140::blockSize(members), 16u);
}

// A static_assert message cannot name the guilty member, so this text is the only
// thing that tells the reader where to look. It must name the member, both
// offsets, and what to do about it.
TEST(Std140, ExplainsWhereEachMemberSitsAndWhereItShould)
{
    const std::string text = compages::gpu::std140::explain<Light>();

    ASSERT_THAT(text, HasSubstr("direction"));
    ASSERT_THAT(text, HasSubstr("intensity"));
    ASSERT_THAT(text, HasSubstr("C++ puts it at byte"));
    ASSERT_THAT(text, HasSubstr("std140 wants byte"));
    ASSERT_THAT(text, HasSubstr(std::to_string(sizeof(Light))));
    // Nothing is wrong with this one, so nothing should be flagged.
    ASSERT_THAT(text, Not(HasSubstr("mismatch")));
}

// The offsets recorded by the macro must be the ones the compiler really chose,
// otherwise the check would pass on a struct that does not match.
TEST(Std140, RecordsTheOffsetsTheCompilerChose)
{
    auto const& members = compages::gpu::std140::Description<Light>::members;

    ASSERT_EQ(members.size(), 2u);
    ASSERT_EQ(members[0].offset, offsetof(Light, direction));
    ASSERT_EQ(members[1].offset, offsetof(Light, intensity));
    ASSERT_STREQ(members[0].name, "direction");
    ASSERT_STREQ(members[1].name, "intensity");
}

TEST(Std140, RecordsEveryMemberOfALongerBlock)
{
    ASSERT_EQ(compages::gpu::std140::Description<Camera>::members.size(), 2u);
    ASSERT_EQ(compages::gpu::std140::Description<Material>::members.size(), 2u);
}
