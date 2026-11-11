// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Fields/Field.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/Renderer/Visualization/FieldLook.hpp"
#include "Compages/World/Entity.hpp"

#include <optional>
#include <span>
#include <string>
#include <unordered_map>

namespace compages::renderer
{

class Scene;

// ****************************************************************************
//! \brief The fields a Scene shows (Scene::show() calls it): a mesh of the
//! grid, a material made from the PBR hooks, and the storage buffers of the
//! values given from the CPU.
// ****************************************************************************
class FieldViews
{
public:

    compages::world::Entity show(Scene& p_scene,
                                 compages::Field<float> const& p_field,
                                 FieldLook const& p_look,
                                 std::string p_name);
    [[nodiscard]] Status show(Scene& p_scene,
                              compages::world::EntityId p_entity,
                              compages::Field<float> const& p_field,
                              FieldLook const& p_look);
    compages::world::Entity show(Scene& p_scene,
                                 compages::Grid2D const& p_grid,
                                 compages::gpu::StorageBuffer<float> const& p_values,
                                 FieldLook const& p_look,
                                 std::string p_name);
    [[nodiscard]] Status show(Scene& p_scene,
                              compages::world::EntityId p_entity,
                              compages::gpu::StorageBuffer<float> const& p_values,
                              FieldLook const& p_look);

    compages::world::Entity show(Scene& p_scene,
                                 compages::Field<float, compages::Grid3D> const& p_volume,
                                 FieldLook const& p_look,
                                 std::string p_name);
    [[nodiscard]] Status show(Scene& p_scene,
                              compages::world::EntityId p_entity,
                              compages::Field<float, compages::Grid3D> const& p_volume,
                              FieldLook const& p_look);

    compages::world::Entity show(Scene& p_scene,
                                 compages::Field<compages::Vector2f> const& p_field,
                                 ArrowLook const& p_look,
                                 std::string p_name);
    [[nodiscard]] Status show(Scene& p_scene,
                              compages::world::EntityId p_entity,
                              compages::Field<compages::Vector2f> const& p_field,
                              ArrowLook const& p_look);

private:

    //! \brief The buffer of a field given from the CPU, written with these
    //! values (made the first time).
    [[nodiscard]] Result<compages::gpu::BufferHandle>
    values(compages::world::EntityId p_entity, std::span<const float> p_values);

    //! \brief The entity, the mesh and the material of a field.
    compages::world::Entity grid(Scene& p_scene,
                                 compages::Grid2D const& p_grid,
                                 FieldLook const& p_look,
                                 std::string p_name);

    //! \brief The values and the look of a field on its material.
    [[nodiscard]] Status apply(Scene& p_scene,
                               compages::world::EntityId p_entity,
                               compages::gpu::BufferHandle p_values,
                               std::size_t p_count,
                               FieldLook const& p_look);

    std::unordered_map<compages::world::EntityId, compages::gpu::StorageBuffer<float>> m_values;
};

//! \brief Scene::probe(): the point and the value under a pixel.
[[nodiscard]] std::optional<FieldProbe> probeField(Scene const& p_scene,
                                                   compages::Field<float> const& p_field,
                                                   FieldLook const& p_look,
                                                   compages::Vector2f p_pixel);

} // namespace compages::renderer
