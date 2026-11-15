// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/ShaderSource.hpp"

#include <utility>

namespace compages::renderer::shaders
{

// ****************************************************************************
//! \file
//! \brief The GLSL of the materials of the Renderer, grouped by feature.
//!
//! Each function returns the two stages of a shader as gpu::ShaderSource. They
//! share pieces that any shader of the program may include too:
//!
//! | Include                      | What it gives                              |
//! | ---------------------------- | ------------------------------------------ |
//! | <compages/skinning.glsl>     | joints, weights, skinMatrix()              |
//! | <compages/scene.glsl>        | lightDir, lightColor, ambient, cameraPos, fog uniforms |
//! | <compages/fog.glsl>          | applyFog(colour, worldPos): plain, or the  |
//! |                              | volumetric fog (FogVolume) when it is on   |
//! | <compages/sun.glsl>          | sunVisibility(n, toLight, worldPos)        |
//! | <compages/lamps.glsl>        | pointLightsContribution(n, worldPos)       |
//! | <compages/shade.glsl>        | shade(base, n, worldPos): sun, sky, lamps; |
//! |                              | shadePbr(albedo, n, worldPos, metallic,    |
//! |                              | roughness): GGX, Smith, Schlick (glTF);    |
//! |                              | bumpNormal(n, worldPos, uv, tangentNormal) |
//! | <compages/present.glsl>      | opacity, present(linear): tone curve, sRGB |
//!
//! The PBR shader leaves four hooks open:
//! - \c deform (vertex): \c void \c deform(inout \c vec4 \c world, inout \c vec3
//!   \c normal), called on the world position and normal before they are
//!   projected. By default it changes nothing.
//! - \c surface (fragment): \c void \c surface(inout \c Surface \c s). It
//!   receives what the material instance describes (\c albedo, \c normal,
//!   \c metallic, \c roughness, \c emission, \c opacity) and changes what it
//!   wants; the Renderer then lights it (sun, lamps, shadows), adds the fog
//!   and the tone curve. It may \c discard. By default it changes nothing.
//!   This is the hook for a new look: the lights stay right without effort.
//! - \c light (fragment): \c vec3 \c light(Surface \c s), the light on the
//!   surface in linear units, before its emission, the fog and the tone
//!   curve. By default \c sceneLight(s) (sun, sky, lamps, shadows). This is
//!   the hook for another model of light (cel shading, a rim): it reads
//!   \c lightDir, \c lightColor, \c ambient, \c cameraPos, \c vWorldPos and
//!   calls \c sunVisibility(), \c pointLightsContribution().
//! - \c main (fragment): the whole main() writing \c oColor, for what is
//!   not a lit surface (a debug view, a picture shown as it is). It may
//!   call \c instanceSurface(), \c surface(), \c light(s) and
//!   \c toScreen(s, lit) (the default main is these four calls). Declared
//!   before it: \c oColor, the uniforms the Renderer sets (baseColorFactor,
//!   baseColorMap, hasBaseColorMap; for the metallic-roughness model of an
//!   instance: pbrUseMetalRoughness, pbrMetallic, pbrRoughness,
//!   pbrMetalRoughnessMap, pbrHasMetalRoughnessMap, pbrNormalMap,
//!   pbrHasNormalMap, pbrNormalScale) and every include above.
//!
//! \code
//! auto pbr = compages::renderer::shaders::pbr();
//! pbr.vertex().include("waves.glsl", wavesGlsl()).hook("deform", DEFORM);
//! pbr.fragment().include("waves.glsl", wavesGlsl()).hook("surface", WATER);
//! \endcode
// ****************************************************************************

inline constexpr int MAX_POINT_LIGHTS = 4;
//! \brief Joint palette uploaded each draw. Mixamo Soldier sits under 60.
inline constexpr int MAX_JOINTS = 64;

// ****************************************************************************
//! \brief The vertex and the fragment stages of one shader, to change before
//! they are compiled.
//!
//! What the functions of this file return. Change a stage with include(),
//! define() and hook() of gpu::ShaderSource, then give the pair to
//! AssetManager::createMaterial(). Copying a pair copies the text: the shader
//! of the library is not changed for anyone else.
//!
//! \code
//! compages::renderer::shaders::ShaderPair pbr = compages::renderer::shaders::pbr();
//! pbr.vertex().include("waves.glsl", wavesGlsl()).hook("deform", DEFORM);
//! pbr.fragment().hook("surface", WATER);
//! \endcode
// ****************************************************************************
class ShaderPair
{
public:

    ShaderPair(compages::gpu::ShaderSource p_vertex,
               compages::gpu::ShaderSource p_fragment)
        : m_vertex(std::move(p_vertex)), m_fragment(std::move(p_fragment))
    {
    }

    //! \brief The vertex stage.
    [[nodiscard]] compages::gpu::ShaderSource& vertex()
    {
        return m_vertex;
    }

    //! \brief The vertex stage.
    [[nodiscard]] compages::gpu::ShaderSource const& vertex() const
    {
        return m_vertex;
    }

    //! \brief The fragment stage.
    [[nodiscard]] compages::gpu::ShaderSource& fragment()
    {
        return m_fragment;
    }

    //! \brief The fragment stage.
    [[nodiscard]] compages::gpu::ShaderSource const& fragment() const
    {
        return m_fragment;
    }

private:

    compages::gpu::ShaderSource m_vertex;
    compages::gpu::ShaderSource m_fragment;
};

// ----------------------------------------------------------------------------
//! \brief A compute shader that may include the pieces of the library
//! (<compages/sun.glsl>, <compages/lamps.glsl>...). It defines
//! COMPAGES_COMPUTE before them (in its own text, after #version): the parts
//! that need a pixel (gl_FragCoord) are then left out.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::gpu::ShaderSource compute(char const* p_name, char const* p_text);

//! \brief One sun, a flat \c color uniform.
[[nodiscard]] ShaderPair lit();

//! \brief glTF-style base colour factor and albedo map; hooks \c deform and
//! \c main.
[[nodiscard]] ShaderPair pbr();

//! \brief A flat colour, no light: helpers, bulbs, labels.
[[nodiscard]] ShaderPair unlit();

//! \brief Linearised eye-space depth as greyscale.
[[nodiscard]] ShaderPair depth();

//! \brief The normal seen from the camera, as RGB.
[[nodiscard]] ShaderPair normals();

//! \brief Depth seen from the sun: the shadow map of the lit materials.
[[nodiscard]] ShaderPair shadow();

//! \brief Distance to a lamp: one face of the shadow cube of a lamp.
[[nodiscard]] ShaderPair lampShadow();

} // namespace compages::renderer::shaders
