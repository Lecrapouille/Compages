// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

#include <cstdint>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{

// ****************************************************************************
//! \brief The curve that turns the light computed by the shaders into
//! colours a screen can show.
// ****************************************************************************
enum class ToneMapping
{
    //! \brief Clamp at one: what is too bright turns flat white.
    None,
    //! \brief x / (1 + x): soft, a bit grey.
    Reinhard,
    //! \brief The filmic curve of the film industry (ACES): contrast, and
    //! bright colours that stay coloured.
    Aces,
};



// ****************************************************************************
//! \brief Fog that the light crosses: shafts of the sun between pillars,
//! halos round the lamps, shadows in the air (FogVolume). Off by default: the
//! plain fog of Environment::fog_density is then used.
//!
//! The most expensive light effect of Compages: use it when the air is part
//! of the picture (a dusty hall, a foggy street), not for distance haze.
//!
//! \code
//! compages::renderer::VolumetricFog& fog = scene.environment().volumetric;
//! fog.enabled = true;
//! fog.density = 0.035f;     // light taken per metre, at fog.height
//! fog.falloff = 0.25f;      // thinner higher up
//! fog.anisotropy = 0.6f;    // light sent mostly forward
//! \endcode
// ****************************************************************************
struct VolumetricFog
{
    //! \brief On: the volumetric fog replaces the plain one.
    bool enabled = false;
    //! \brief How much light one metre of fog takes away at the height
    //! \c height (the extinction, per metre): 0.02 a light haze, 0.2 thick.
    float density = 0.04f;
    //! \brief The fog thins out above \c height:
    //! density * exp(-falloff * (y - height)). Zero: as thick everywhere.
    float height = 0.0f;
    float falloff = 0.2f;
    //! \brief The colour of the fog (sRGB): the part of the light it takes
    //! that it sends on rather than absorbs.
    compages::Vector3f color{ 0.9f, 0.9f, 0.95f };
    //! \brief How the light is sent on (Henyey-Greenstein): 0 evenly, toward
    //! 1 mostly forward (a bright halo when looking toward a light).
    float anisotropy = 0.5f;
    //! \brief Clumps of fog, from 0 (even) to 1, drifting with the wind
    //! (metres per second).
    float noise = 0.5f;
    compages::Vector3f wind{ 0.4f, 0.0f, 0.15f };
    //! \brief The fog is computed up to this distance; beyond, it stays as
    //! it is at that distance.
    float distance = 60.0f;
};

// ****************************************************************************
//! \brief The space around the entities, as one view of a Scene draws it:
//! ambient light, fog, shadows, the tone curve, the exposure.
//!
//! A Scene keeps one (Scene::environment()); change its fields at any frame.
//! It belongs to the Scene, not to the World: two Scenes of one World may
//! light it differently. The sky box is set with Scene::skybox().
//!
//! \code
//! compages::renderer::Environment& env = scene.environment();
//! env.fog_color = { 0.6f, 0.65f, 0.7f };
//! env.fog_density = 0.02f;
//! env.exposure = 1.5f;
//! env.shadows = false;        // a chart or a flat field
//! \endcode
// ****************************************************************************
struct Environment
{
    //! \brief A dim colour multiplied by the material colour to fake the light
    //! that comes from every direction.
    compages::Vector3f ambient{ 0.15f, 0.15f, 0.18f };

    //! \brief Direction the built-in lit shader receives when no directional
    //! light is present in the compages::world::World. Points toward the light (i.e. the
    //! opposite of what the photons travel).
    compages::Vector3f default_light_direction{ 0.4f, 0.8f, 0.6f };

    //! \brief Colour of that light when no directional light is present. Black
    //! leaves the scene to its ambient and its lamps: a cave, a night.
    compages::Vector3f default_light_color{ 1.0f, 1.0f, 1.0f };

    //! \brief What the distance fades to, and how fast. A density of zero is
    //! no fog; around 0.1 hides what is twenty units away.
    compages::Vector3f fog_color{ 0.0f, 0.0f, 0.0f };
    float fog_density = 0.0f;

    //! \brief The sun casts shadows: a depth picture of the scene seen from
    //! the sun, fitted to the objects that cast (not those with
    //! \c RenderFlags::NoShadowCaster, such as a floor).
    bool shadows = true;
    //! \brief Pixels across the shadow picture: sharper, but more memory.
    std::uint32_t shadow_resolution = 2048u;
    //! \brief Pixels across each of the six faces of the shadow cube of a
    //! lamp (PointLight::shadows). Zero turns the shadows of lamps off.
    std::uint32_t lamp_shadow_resolution = 512u;
    //! \brief How the light, which has no upper limit, becomes a screen
    //! colour between 0 and 1. ACES keeps the hue of bright surfaces and
    //! gives contrast, like a film; Reinhard is softer; None clamps.
    ToneMapping tone_mapping = ToneMapping::Aces;
    //! \brief Multiplies the light before the tone curve: above one is
    //! brighter, like opening the lens of a camera.
    float exposure = 1.0f;
    //! \brief Fog that the light crosses (off by default).
    VolumetricFog volumetric{};
};

} // namespace compages::renderer
