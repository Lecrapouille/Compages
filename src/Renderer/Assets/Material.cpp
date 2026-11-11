// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"

namespace compages::renderer
{

// ----------------------------------------------------------------------------
Result<Material> Material::create(std::string p_name,
                                  MaterialDesc const& p_desc)
{
    Material material;
    material.name = std::move(p_name);
    material.family = p_desc.family;
    COMPAGES_TRY(material.program.load(p_desc.shaders.vertex(),
                                       p_desc.shaders.fragment()));
    COMPAGES_TRY_ASSIGN(material.pipeline,
                        compages::gpu::Pipeline::create<MeshVertex>(
                            material.program, p_desc.state));
    if (p_desc.see_through)
    {
        COMPAGES_TRY_ASSIGN(material.see_through,
                            compages::gpu::Pipeline::create<MeshVertex>(
                                material.program, *p_desc.see_through));
    }
    return material;
}

} // namespace compages::renderer
