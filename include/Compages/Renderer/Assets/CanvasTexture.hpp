// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/World/World.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace compages::renderer
{

class AssetManager;
class Scene;

// ****************************************************************************
//! \brief An RGBA8 image on the CPU that can be stamped and pushed to the GPU.
//!
//! Typical use: a painting canvas on a mesh. Pixels live in main memory; call
//! \ref sync after \ref paintLocal, \ref paintWorld, or \ref paintLine to upload
//! dirty regions to the \c TextureAsset registered with \ref bind.
//!
//! \code
//! compages::renderer::CanvasTexture canvas;
//! COMPAGES_TRY(canvas.create(512u, 512u));
//! canvas.setMapping(compages::core::AABB::fromCorners(
//!     { -0.5f, -0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }));
//! COMPAGES_TRY(canvas.bind(scene.assets(), "easel"));
//! scene.look(easel_entity, canvas.texture());
//! // each frame while the brush touches the surface:
//! canvas.paintWorld(hit.point, easel_entity.worldMatrix(), color, 0.02f);
//! COMPAGES_TRY(canvas.sync());
//! \endcode
//!
//! See doc/CanvasTexture.md and example \c 39a_CanvasPaint.
// ****************************************************************************
class CanvasTexture
{
public:

    //! \brief One pixel in row-major order, bottom row first (OpenGL convention).
    struct Color
    {
        std::uint8_t r = 255u;
        std::uint8_t g = 255u;
        std::uint8_t b = 255u;
        std::uint8_t a = 255u;

        [[nodiscard]] static Color rgb(std::uint8_t p_red,
                                       std::uint8_t p_green,
                                       std::uint8_t p_blue)
        {
            return Color{ p_red, p_green, p_blue, 255u };
        }

        [[nodiscard]] static Color white()
        {
            return Color{};
        }
    };

    // ------------------------------------------------------------------------
    //! \brief Allocate the CPU buffer and clear it to \p p_fill.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status create(std::uint32_t p_width,
                                std::uint32_t p_height,
                                Color p_fill = Color::white());

    [[nodiscard]] std::uint32_t width() const
    {
        return m_width;
    }
    [[nodiscard]] std::uint32_t height() const
    {
        return m_height;
    }

    // ------------------------------------------------------------------------
    //! \brief How local mesh coordinates map to texture coordinates.
    //!
    //! Defaults match a unit \c scene.plane(): X and Y from -0.5 to 0.5 at
    //! \c z = 0. Use \c flip_v when the picture appears upside down on your mesh.
    // ------------------------------------------------------------------------
    void setMapping(compages::core::AABB p_local_bounds,
                    bool p_flip_u = false,
                    bool p_flip_v = false);

    // ------------------------------------------------------------------------
    //! \brief Register or replace the GPU texture in an \c AssetManager.
    //!
    //! Re-registering under the same name updates the asset in place and keeps
    //! the same \ref texture id when the size is unchanged.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind(AssetManager& p_assets, std::string p_name);

    //! \brief Id returned by the last successful \ref bind.
    [[nodiscard]] TextureAssetId texture() const
    {
        return m_texture_id;
    }

    // ------------------------------------------------------------------------
    //! \brief Fill the whole canvas on the CPU and mark it dirty.
    // ------------------------------------------------------------------------
    void clear(Color p_fill = Color::white());

    // ------------------------------------------------------------------------
    //! \brief Stamp a filled disk in local surface coordinates (metres on the mesh).
    // ------------------------------------------------------------------------
    void paintLocal(compages::core::Vector3f p_local_point,
                    Color p_color,
                    float p_brush_radius_local);

    // ------------------------------------------------------------------------
    //! \brief Stamp at a world-space point using the canvas entity's world matrix.
    // ------------------------------------------------------------------------
    void paintWorld(compages::core::Vector3f p_world_point,
                    compages::core::Matrix44f const& p_canvas_world,
                    Color p_color,
                    float p_brush_radius_world);

    //! \brief Interpolate stamps so fast motion stays continuous.
    void paintLineWorld(compages::core::Vector3f p_from,
                        compages::core::Vector3f p_to,
                        compages::core::Matrix44f const& p_canvas_world,
                        Color p_color,
                        float p_brush_radius_world);

    // ------------------------------------------------------------------------
    //! \brief Upload dirty pixels to the bound \c TextureAsset.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status sync();

    //! \brief Assign \ref texture to a drawn entity (\c PbrMinimal albedo map).
    void applyLook(Scene& p_scene, compages::world::EntityId p_entity);

private:

    void stampDisk(int p_center_x, int p_center_y, int p_pixel_radius, Color p_color);
    void markDirty(std::uint32_t p_x0,
                   std::uint32_t p_y0,
                   std::uint32_t p_x1,
                   std::uint32_t p_y1);
    [[nodiscard]] bool localToPixel(compages::core::Vector3f p_local,
                                    int& p_px,
                                    int& p_py) const;
    [[nodiscard]] int
    worldRadiusToPixels(float p_radius_world, float p_world_extent_x) const;

    std::vector<std::uint8_t> m_pixels;
    std::uint32_t m_width = 0u;
    std::uint32_t m_height = 0u;
    compages::core::AABB m_local_bounds =
        compages::core::AABB::fromCorners({ -0.5f, -0.5f, 0.0f },
                                          { 0.5f, 0.5f, 0.0f });
    bool m_flip_u = false;
    bool m_flip_v = false;

    AssetManager* m_assets = nullptr;
    TextureAssetId m_texture_id{};
    std::string m_asset_name;

    bool m_dirty = false;
    bool m_dirty_all = false;
    std::uint32_t m_dirty_x0 = 0u;
    std::uint32_t m_dirty_y0 = 0u;
    std::uint32_t m_dirty_x1 = 0u;
    std::uint32_t m_dirty_y1 = 0u;
};

} // namespace compages::renderer
