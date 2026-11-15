// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <map>
#include <string>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief One filter of the picture of a view: a piece of GLSL run on every
//! pixel, after the scene is drawn and before the tone curve.
//!
//! The text defines \c vec3 \c effect(vec2 \c uv), the light of the pixel at
//! \c uv (0 to 1 across the view). It reads:
//! - \c screenColor (sampler2D): the light of the view, linear (or what the
//!   filter before it made);
//! - \c screenDepth (sampler2D): the depth, reversed (1 near, 0 far, 0 for
//!   the background);
//! - \c screenSize (vec2): the size of the view in pixels;
//! - its own uniforms, \c uniform \c float, set from \c values.
//!
//! \code
//! compages::renderer::ScreenFilter sepia{
//!     .name = "Sepia",
//!     .glsl = "uniform float amount;\n"
//!             "vec3 effect(vec2 uv) { vec3 c = texture(screenColor, uv).rgb;\n"
//!             "  float y = dot(c, vec3(0.3, 0.59, 0.11));\n"
//!             "  return mix(c, y * vec3(1.2, 1.0, 0.8), amount); }",
//!     .values = { { "amount", 0.8f } } };
//! \endcode
// ****************************************************************************
struct ScreenFilter
{
    //! \brief Shown in the errors of compilation.
    std::string name;
    //! \brief The text: \c vec3 \c effect(vec2 \c uv) and what it needs.
    std::string glsl;
    //! \brief The uniforms of the text, by name.
    std::map<std::string, float> values;
};

// ****************************************************************************
//! \brief Light brighter than the screen can show spills round it, as in a
//! lens: the halo of a bulb, of the sun on metal.
//!
//! Use it when the picture has sources much brighter than white: bulbs seen
//! directly, the sun on metal, sparks. In a dim scene it only blurs. It costs
//! about twelve small passes per view (doc/techniques/bloom.md).
//!
//! \code
//! camera.set(compages::renderer::ScreenEffects{
//!     .filters = {}, .bloom = { .enabled = true, .threshold = 1.0f, .strength = 0.6f } });
//! \endcode
// ****************************************************************************
struct Bloom
{
    bool enabled = false;
    //! \brief The light under this level does not spill (linear units after
    //! the exposure: 1 is the white of the screen).
    float threshold = 1.0f;
    //! \brief How much of the spilled light is added back.
    float strength = 0.5f;
};

// ****************************************************************************
//! \brief The screen effects of a view: put this component on a camera
//! entity. Its picture is then drawn in linear light (RGBA16F), passed
//! through the filters in their order, then the bloom, and only then given
//! the tone curve and the screen colours. Another camera without it is
//! drawn as before: each view chooses.
//!
//! \code
//! m_scene.camera().set(compages::renderer::ScreenEffects{
//!     .filters = { sepia },
//!     .bloom = { .enabled = true, .threshold = 1.0f, .strength = 0.6f } });
//! \endcode
// ****************************************************************************
struct ScreenEffects
{
    //! \brief Run one after the other, each reading what the previous made.
    std::vector<ScreenFilter> filters;
    Bloom bloom;

    //! \brief Is there anything to do?
    [[nodiscard]] bool active() const
    {
        return bloom.enabled || !filters.empty();
    }
};

} // namespace compages::renderer
