// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief How a shape is drawn, apart from its data and its shader: depth,
//! transparency, hidden faces, outlines.
//!
//! Should a shape hide what is behind it? Should it be see-through? Should its
//! back be skipped? These questions are answered once, in a RenderState, when a
//! Pipeline is created, and the answer is applied whole every time that
//! pipeline is used.
//!
//! \code
//! compages::gpu::RenderState glass{ .depth_test = true,
//!                                   .depth_write = false,
//!                                   .blend = compages::gpu::Blend::alpha() };
//! \endcode
//!
//! This used to be state set by calling into the graphics API between draws,
//! and therefore state that leaked from one draw to the next: forgetting to
//! turn depth testing off after a pass left the next one wrong. Here there is
//! nothing to restore, because nothing was ever left changed.
//!
//! What it holds:
//! - RenderState: all the settings, with defaults for the simplest drawing.
//! - Blend and its presets (alpha(), additive()...), ColorMask.
//! - CompareFunc, BlendFactor, BlendEquation, CullMode, FrontFace, PolygonMode.
// ****************************************************************************

#include "Compages/GPU/Enums.hpp"

#include <bit>
#include <cstdint>
#include <string>

namespace compages::gpu
{

// ----------------------------------------------------------------------------
//! \brief How a value is compared against the one already there.
//!
//! Used for the depth test, where the value is the distance to the camera and
//! Less means "keep what is nearer".
// ----------------------------------------------------------------------------
enum class CompareFunc
{
    //! \brief Never passes. Nothing is drawn.
    Never,
    //! \brief Passes when the new value is smaller, which for depth means
    //! nearer. What almost every 3D pass wants.
    Less,
    Equal,
    //! \brief Passes when nearer or at the same distance. What a second pass
    //! drawing over the first at the same depth needs.
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    //! \brief Always passes, which is the same as not testing at all.
    Always,
};

// ----------------------------------------------------------------------------
//! \brief Name of a comparison in plain words ("less or equal"), for error
//! messages and for printing a state.
//! \param[in] p_func the comparison to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(CompareFunc p_func);

// ----------------------------------------------------------------------------
//! \brief What a colour is multiplied by before the two are combined.
//!
//! Src is the colour the fragment shader just produced, Dst the one already in
//! the target.
// ----------------------------------------------------------------------------
enum class BlendFactor
{
    //! \brief Throw this colour away.
    Zero,
    //! \brief Take it as it is.
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    //! \brief Weight by how opaque the new colour is.
    SrcAlpha,
    //! \brief Weight by how transparent the new colour is.
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
};

// ----------------------------------------------------------------------------
//! \brief Name of a blend factor in plain words, for printing a state.
//! \param[in] p_factor the value to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(BlendFactor p_factor);

// ----------------------------------------------------------------------------
//! \brief How the two weighted colours are combined.
// ----------------------------------------------------------------------------
enum class BlendEquation
{
    //! \brief The two are added, which is what every ordinary blend does.
    Add,
    //! \brief The one already there is taken away from the new one.
    Subtract,
    //! \brief The new one is taken away from the one already there.
    ReverseSubtract,
    //! \brief The darker of the two is kept.
    Min,
    //! \brief The brighter of the two is kept.
    Max,
};

// ----------------------------------------------------------------------------
//! \brief Name of a blend equation in plain words, for printing a state.
//! \param[in] p_equation the value to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(BlendEquation p_equation);

// ****************************************************************************
//! \brief How a new colour is mixed with the one already on screen: how
//! glass, smoke and glowing sparks are drawn.
//!
//! Without blending, each new colour simply covers the old one, like paint.
//! With blending, the two are mixed, like tinted glass laid over a picture.
//!
//! The four factors and two equations are spelled out because there is no way
//! to guess them, but nobody should have to write them: the named ones below
//! cover what is actually used, and they say what they are for rather than how
//! they work.
//!
//! \code
//! compages::gpu::RenderState state{ .blend = compages::gpu::Blend::alpha() };
//! \endcode
// ****************************************************************************
struct Blend
{
    //! \brief When false the new colour simply replaces the old one, and the
    //! rest of this struct is ignored.
    bool enabled = false;

    //! \brief Weight of the new colour.
    BlendFactor source_color = BlendFactor::One;
    //! \brief Weight of the colour already in the target.
    BlendFactor destination_color = BlendFactor::Zero;
    //! \brief How the two weighted colours are combined.
    BlendEquation color_equation = BlendEquation::Add;

    //! \brief Weight of the new alpha. Separate from the colour so that the
    //! alpha left in a target can stay meaningful for a later pass.
    BlendFactor source_alpha = BlendFactor::One;
    //! \brief Weight of the alpha already in the target.
    BlendFactor destination_alpha = BlendFactor::Zero;
    //! \brief How the two weighted alphas are combined.
    BlendEquation alpha_equation = BlendEquation::Add;

    // ------------------------------------------------------------------------
    //! \brief No blending: the new colour replaces the old one. The default.
    //! \return a disabled blend.
    // ------------------------------------------------------------------------
    [[nodiscard]] static constexpr Blend none()
    {
        return Blend{};
    }

    // ------------------------------------------------------------------------
    //! \brief Ordinary transparency, where the alpha of the new colour says how
    //! much of it to keep.
    //!
    //! What a sprite with soft edges, a font glyph or a window of glass wants.
    //! Note that transparent surfaces have to be drawn back to front for this
    //! to look right, which is a property of the drawing order rather than of
    //! the state.
    //! \return the blend for straight alpha.
    // ------------------------------------------------------------------------
    [[nodiscard]] static constexpr Blend alpha()
    {
        return Blend{ true,
                      BlendFactor::SrcAlpha,
                      BlendFactor::OneMinusSrcAlpha,
                      BlendEquation::Add,
                      BlendFactor::One,
                      BlendFactor::OneMinusSrcAlpha,
                      BlendEquation::Add };
    }

    // ------------------------------------------------------------------------
    //! \brief Transparency for colours whose alpha has already been multiplied
    //! in.
    //!
    //! Worth knowing about because it composes correctly when several
    //! transparent layers are combined, which plain alpha does not, and because
    //! it is what a texture built by a rendering pass usually holds.
    //! \return the blend for premultiplied alpha.
    // ------------------------------------------------------------------------
    [[nodiscard]] static constexpr Blend premultiplied()
    {
        return Blend{ true,
                      BlendFactor::One,
                      BlendFactor::OneMinusSrcAlpha,
                      BlendEquation::Add,
                      BlendFactor::One,
                      BlendFactor::OneMinusSrcAlpha,
                      BlendEquation::Add };
    }

    // ------------------------------------------------------------------------
    //! \brief Light adds to light: colours pile up and never darken.
    //!
    //! What fire, sparks, lens flares and the stars of a galaxy want. Drawing
    //! order does not matter here, which is why particle systems reach for it.
    //! \return the additive blend.
    // ------------------------------------------------------------------------
    [[nodiscard]] static constexpr Blend additive()
    {
        return Blend{ true,
                      BlendFactor::SrcAlpha,
                      BlendFactor::One,
                      BlendEquation::Add,
                      BlendFactor::One,
                      BlendFactor::One,
                      BlendEquation::Add };
    }

    // ------------------------------------------------------------------------
    //! \brief The two colours multiply, which can only darken. What a shadow or
    //! a tint laid over a scene wants.
    //! \return the multiplying blend.
    // ------------------------------------------------------------------------
    [[nodiscard]] static constexpr Blend multiply()
    {
        return Blend{ true,
                      BlendFactor::DstColor,
                      BlendFactor::Zero,
                      BlendEquation::Add,
                      BlendFactor::DstAlpha,
                      BlendFactor::Zero,
                      BlendEquation::Add };
    }

    //! \brief Same effect on screen: two disabled blends are equal whatever
    //! their factors. Decides whether two pipelines can share their state.
    [[nodiscard]] friend constexpr bool operator==(Blend const& p_left,
                                                   Blend const& p_right)
    {
        // Two blends that are both off behave the same whatever their factors,
        // which matters because this is what decides whether two pipelines can
        // share their state.
        if (!p_left.enabled && !p_right.enabled)
        {
            return true;
        }
        return (p_left.enabled == p_right.enabled) &&
               (p_left.source_color == p_right.source_color) &&
               (p_left.destination_color == p_right.destination_color) &&
               (p_left.color_equation == p_right.color_equation) &&
               (p_left.source_alpha == p_right.source_alpha) &&
               (p_left.destination_alpha == p_right.destination_alpha) &&
               (p_left.alpha_equation == p_right.alpha_equation);
    }

    // ------------------------------------------------------------------------
    //! \brief A readable description, naming the preset when it is one
    //! ("alpha") and listing the factors otherwise.
    //! \return one line, such as "alpha" or "off".
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string toString() const;
};

// ----------------------------------------------------------------------------
//! \brief Which side of a triangle is not drawn.
//!
//! Half the triangles of a closed shape face away from the camera and are
//! hidden by the ones facing it, so dropping them halves the work for free. It
//! is off by default, because on a shape that is not closed, or one whose
//! triangles were wound the other way, it makes surfaces vanish and the cause
//! is not obvious.
// ----------------------------------------------------------------------------
enum class CullMode
{
    //! \brief Draw both sides. The default, and what a flat shape or a
    //! two sided surface needs.
    None,
    //! \brief Drop the triangles facing away. What a closed mesh wants.
    Back,
    //! \brief Drop the triangles facing the camera. Used to see the inside of a
    //! shape, and by some shadow techniques.
    Front,
};

// ----------------------------------------------------------------------------
//! \brief Name of a cull mode in plain words, for printing a state.
//! \param[in] p_mode the value to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(CullMode p_mode);

// ----------------------------------------------------------------------------
//! \brief Which winding order means a triangle faces the camera.
// ----------------------------------------------------------------------------
enum class FrontFace
{
    //! \brief The usual convention, and what every mesh in this project uses.
    CounterClockwise,
    //! \brief The other one. Some file formats and some mirrored transforms
    //! need it.
    Clockwise,
};

// ----------------------------------------------------------------------------
//! \brief Name of a winding order in plain words, for printing a state.
//! \param[in] p_face the value to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(FrontFace p_face);

// ----------------------------------------------------------------------------
//! \brief Whether triangles are filled in, outlined, or shown as corners.
// ----------------------------------------------------------------------------
enum class PolygonMode
{
    //! \brief Filled in. What drawing normally means.
    Fill,
    //! \brief Only the edges, which is how a wireframe is drawn without having
    //! to build a separate line mesh.
    Line,
    //! \brief Only the corners.
    Point,
};

// ----------------------------------------------------------------------------
//! \brief Name of a polygon mode in plain words, for printing a state.
//! \param[in] p_mode the value to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(PolygonMode p_mode);

// ****************************************************************************
//! \brief Which colour channels a pass is allowed to write.
//!
//! Turning all four off is how a pass fills the depth buffer without touching
//! the picture, which is what a shadow pass and a depth prepass do.
//!
//! \code
//! compages::gpu::RenderState depth_only{ .depth_test = true,
//!                                        .color_mask =
//!                                        compages::gpu::ColorMask::none() };
//! \endcode
// ****************************************************************************
struct ColorMask
{
    //! \brief May the red channel be written?
    bool red = true;
    //! \brief May the green channel be written?
    bool green = true;
    //! \brief May the blue channel be written?
    bool blue = true;
    //! \brief May the alpha channel be written? Turned off alone, it keeps the
    //! alpha a previous pass left in the target.
    bool alpha = true;

    // ------------------------------------------------------------------------
    //! \brief Write nothing. For a pass that only cares about depth.
    //! \return a mask with every channel off.
    // ------------------------------------------------------------------------
    [[nodiscard]] static constexpr ColorMask none()
    {
        return ColorMask{ false, false, false, false };
    }

    //! \brief Same four channels allowed.
    [[nodiscard]] friend constexpr bool operator==(ColorMask const& p_left,
                                                   ColorMask const& p_right)
    {
        return (p_left.red == p_right.red) && (p_left.green == p_right.green) &&
               (p_left.blue == p_right.blue) && (p_left.alpha == p_right.alpha);
    }
};

// ****************************************************************************
//! \brief The settings of one way of drawing: hide what is behind, mix
//! colours, skip the back of shapes, draw outlines only...
//!
//! Think of the switches on a camera rather than the scene in front of it.
//! The same mesh with the same shader looks solid, transparent or wireframe
//! depending only on these settings.
//!
//! Every field has a default, and the defaults describe the simplest thing that
//! can be drawn: filled triangles, no depth test, no blending, both sides
//! visible. A pass says only what it needs to differ.
//!
//! \code
//! compages::gpu::RenderState opaque{ .depth_test = true,
//!                                    .cull = compages::gpu::CullMode::Back };
//!
//! compages::gpu::RenderState wireframe{ .depth_test = true,
//!                             .polygon = compages::gpu::PolygonMode::Line };
//!
//! compages::gpu::RenderState sprites{
//!     .blend = compages::gpu::Blend::alpha() };
//! \endcode
//!
//! \note Designated initializers require the fields to be given in the order
//! they are declared here, which the compiler checks.
// ****************************************************************************
struct RenderState
{
    //! \brief What the vertices are taken to draw. Part of the state rather
    //! than of the draw call, so that a pipeline is a complete description of
    //! one way of drawing: the same mesh drawn as triangles and as points is
    //! two pipelines, and neither can be confused for the other.
    Primitive primitive = Primitive::Triangles;

    //! \brief Should a fragment further away than what is already there be
    //! dropped? Off by default, because a pass that does not need it should not
    //! pay for it, and because a target without a depth buffer cannot do it.
    bool depth_test = false;

    //! \brief Should the distance of a fragment that passed be recorded? Turned
    //! off by a pass that must respect the depth already there without adding
    //! to it, which is what transparent surfaces do.
    bool depth_write = true;

    //! \brief How the distance is compared. Only consulted when depth_test is
    //! on.
    CompareFunc depth_func = CompareFunc::Less;

    //! \brief How a new colour is combined with the one already there.
    Blend blend = Blend::none();

    //! \brief Which side of a triangle is not drawn.
    CullMode cull = CullMode::None;

    //! \brief Which winding order faces the camera.
    FrontFace front_face = FrontFace::CounterClockwise;

    //! \brief Filled, outlined, or corners only.
    PolygonMode polygon = PolygonMode::Fill;

    //! \brief Which colour channels may be written.
    ColorMask color_mask{};

    //! \brief How wide a line is, in pixels.
    //!
    //! \note Only 1.0 is guaranteed. A modern core profile driver is allowed to
    //! refuse anything else, and several do, so a thick line has to be built
    //! out of triangles rather than asked for here. Kept because it does work
    //! on most desktop drivers and a wireframe view is the one place it is
    //! worth trying.
    float line_width = 1.0f;

    //! \brief Every field equal, the line width compared bit for bit. Two
    //! equal states let pipelines share what the driver built for them.
    [[nodiscard]] friend constexpr bool operator==(RenderState const& p_left,
                                                   RenderState const& p_right)
    {
        return (p_left.primitive == p_right.primitive) &&
               (p_left.depth_test == p_right.depth_test) &&
               (p_left.depth_write == p_right.depth_write) &&
               (p_left.depth_func == p_right.depth_func) &&
               (p_left.blend == p_right.blend) &&
               (p_left.cull == p_right.cull) &&
               (p_left.front_face == p_right.front_face) &&
               (p_left.polygon == p_right.polygon) &&
               (p_left.color_mask == p_right.color_mask) &&
               // Compared as the bytes that were asked for rather than as
               // numbers: the question is whether two pipelines want the same
               // state, not whether two widths are close enough to each other.
               (std::bit_cast<std::uint32_t>(p_left.line_width) ==
                std::bit_cast<std::uint32_t>(p_right.line_width));
    }

    // ------------------------------------------------------------------------
    //! \brief A readable description, for the overlay of the examples and for
    //! explaining why two pipelines are not the same.
    //! \return one line per setting.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string toString() const;
};

} // namespace compages::gpu
