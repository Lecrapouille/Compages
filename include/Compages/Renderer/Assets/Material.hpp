// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/RenderState.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/Renderer/Assets/TextureAsset.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Which built-in shader family a Material was built from.
// ****************************************************************************
enum class ShaderFamily
{
    //! \brief Single directional light, flat \c color uniform.
    Lit,
    //! \brief glTF-style base colour factor + optional albedo map.
    PbrMinimal,
    //! \brief Legacy DepthMaterial: linearised eye-space depth as greyscale.
    Depth,
    //! \brief Legacy NormalsMaterial: world-space normal as RGB.
    Normals,
    //! \brief A flat colour that no light changes: a bulb, a marker.
    Unlit,
};

// ****************************************************************************
//! \brief A texture that every object of a material reads, by the name of its
//! sampler in the shader.
// ****************************************************************************
struct MaterialTexture
{
    //! \brief The \c uniform \c sampler2D of the shader.
    std::string sampler;
    //! \brief The picture.
    TextureAssetId texture;
};

// ****************************************************************************
//! \brief A storage buffer that every object of a material reads, by the name
//! of its block in the shader: values a compute shader wrote, for example.
//! Borrowed: the buffer must live as long as the material draws.
// ****************************************************************************
struct MaterialBuffer
{
    //! \brief The block: "buffer Values { float values[]; }" is "Values".
    std::string block;
    //! \brief The buffer, made with BufferKind::Storage.
    compages::gpu::BufferHandle buffer;
};

// ****************************************************************************
//! \brief What Material::create() builds a material from.
//!
//! \code
//! auto pbr = compages::renderer::shaders::pbr();
//! pbr.fragment().hook("main", WATER);
//! COMPAGES_TRY_ASSIGN(material, compages::renderer::Material::create(
//!     "Water", { .family = compages::renderer::ShaderFamily::PbrMinimal,
//!                .shaders = std::move(pbr),
//!                .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
//!                .see_through = std::nullopt }));
//! \endcode
// ****************************************************************************
struct MaterialDesc
{
    //! \brief Which uniforms the Renderer sets for each draw: the family the
    //! shaders were made from.
    ShaderFamily family;
    //! \brief The two stages, with their includes and hooks.
    shaders::ShaderPair shaders;
    //! \brief How solid instances are drawn.
    compages::gpu::RenderState state;
    //! \brief How see-through instances (opacity below one) are drawn; none:
    //! they are drawn solid.
    std::optional<compages::gpu::RenderState> see_through;
};

// ****************************************************************************
//! \brief A shader family with its render state. The heavy part shared by many
//! objects that draw the same way.
//!
//! A Material owns its \c compages::gpu::Program and its \c compages::gpu::Pipeline. Many
//! MaterialInstances can point at the same Material, and a Material is what a
//! renderer would key its "which pipeline is bound right now" cache on.
// ****************************************************************************
struct Material
{
    //! \brief A short label, used in logs.
    std::string name;
    //! \brief Which ShaderLib sources this material was built from.
    ShaderFamily family = ShaderFamily::Lit;
    //! \brief The linked GLSL program.
    compages::gpu::Program program;
    //! \brief The pipeline: program + vertex layout + render state, checked
    //! against each other once at load time.
    compages::gpu::Pipeline pipeline;
    //! \brief The same program for see-through instances (opacity below
    //! one): blended over what is behind, without writing the depth. Empty
    //! for a material made by hand, whose instances are then drawn solid.
    compages::gpu::Pipeline see_through;
    //! \brief Pictures the Renderer binds for this material, on the units it
    //! leaves free (FIRST_TEXTURE_UNIT on, MAX_TEXTURES of them), and whose
    //! sampler it sets.
    std::vector<MaterialTexture> textures;
    //! \brief Storage buffers the Renderer binds for this material.
    std::vector<MaterialBuffer> buffers;
    //! \brief The program of the material drawing the shadow of the sun, depth
    //! only: made by create() when the hook "deform" moves the corners, so
    //! that the shadow has the shape the picture has. Empty: the shadow pass
    //! draws the mesh as it is. Such a material casts no shadow of a lamp.
    compages::gpu::Pipeline casts_shadow;

    //! \brief The first texture unit of textures: 0 is the albedo map, 1 and 7
    //! the shadows.
    static constexpr std::uint32_t FIRST_TEXTURE_UNIT = 2u;
    //! \brief How many textures a material may have.
    static constexpr std::size_t MAX_TEXTURES = 5u;

    // ------------------------------------------------------------------------
    //! \brief Compile the shaders and make the pipelines, for vertices of
    //! MeshVertex: the only layout the Renderer draws.
    //! \param[in] p_name A short label, used in logs.
    //! \param[in] p_desc The family, the shaders, the render states.
    //! \return The material, or the log of the stage that failed.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Material> create(std::string p_name,
                                                 MaterialDesc const& p_desc);
};

// ****************************************************************************
//! \brief Per-object parameters of a Material.
//!
//! A MaterialInstance holds only the values that vary between two objects
//! drawn with the same shader: a colour, texture bindings, tuning uniforms.
//! It does not own the pipeline, does not compile a shader and does not change
//! render state.
//!
//! Two objects sharing the same MaterialInstance are drawn identically. The
//! renderer batches them together.
// ****************************************************************************
struct MaterialInstance
{
    //! \brief Which shader/pipeline family this instance draws with.
    MaterialId material;
    //! \brief The base colour uploaded to the shader's \c color uniform, when
    //! the shader declares one. Linear: a Look gives sRGB, the Scene converts.
    compages::Vector3f color{ 1.0f, 1.0f, 1.0f };
    //! \brief PBR base colour factor (\c baseColorFactor in glTF).
    compages::Vector3f base_color_factor{ 1.0f, 1.0f, 1.0f };
    //! \brief Optional albedo map. Empty means the factor alone is used.
    TextureAssetId base_color_texture{};
    //! \brief Light it with the metallic-roughness model of glTF (shadePbr():
    //! a glint as wide as the roughness, metals tinting it) rather than the
    //! simple model of the looks (shade()). Set by the glTF loader.
    bool metal_roughness = false;
    //! \brief 0 a dielectric (plastic, wood, stone), 1 a metal; times the
    //! blue channel of \c metal_roughness_texture.
    float metallic_factor = 0.0f;
    //! \brief 0 a mirror, 1 matte; times the green channel of
    //! \c metal_roughness_texture.
    float roughness_factor = 1.0f;
    //! \brief Optional map of glTF: G the roughness, B the metallic. Linear.
    TextureAssetId metal_roughness_texture{};
    //! \brief Optional normal map (tangent space, +y up, as glTF). Linear.
    TextureAssetId normal_texture{};
    //! \brief How strong the bumps of the normal map are (glTF scale).
    float normal_scale = 1.0f;
    //! \brief DepthMaterial near/far smoothstep range, in eye-space metres.
    float depth_near = 1.0f;
    float depth_far = 100.0f;
    //! \brief One is solid. Below one, the object is drawn after the solid
    //! ones, farthest first, blended over what is behind it.
    float opacity = 1.0f;
};

} // namespace compages::renderer
