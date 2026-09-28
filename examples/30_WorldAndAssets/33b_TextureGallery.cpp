// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/33b_TextureGallery.hpp"
#include "Common/DataPath.hpp"

#include <array>

namespace examples
{

struct Picture
{
    char const* file;
    Vector3f fallback;
};

const std::array<Picture, 9u> PICTURES{ {
    { "grassFlowers.png", { 0.45f, 0.75f, 0.35f } },
    { "rocks.png", { 0.55f, 0.52f, 0.48f } },
    { "mud.png", { 0.45f, 0.32f, 0.18f } },
    { "grassy2.png", { 0.35f, 0.65f, 0.30f } },
    { "wooden-crate.jpg", { 0.62f, 0.42f, 0.22f } },
    { "cowboy.png", { 0.70f, 0.55f, 0.40f } },
    { "fields.png", { 0.50f, 0.70f, 0.25f } },
    { "hazard.png", { 0.85f, 0.75f, 0.15f } },
    { "tree-01.png", { 0.30f, 0.55f, 0.25f } },
} };

std::string TextureGallery::description() const
{
    return "Nine pictures of the data repository on a grid of turning boxes; "
           "a missing file falls back to a plain colour.";
}

compages::gpu::Status TextureGallery::setUp()
{
    // A slow orbit, so the boxes show more than their front face.
    m_scene.background(0.08f, 0.10f, 0.14f).ambient(0.16f, 0.16f, 0.18f);
    m_scene.camera().position(0.0f, 5.0f, 10.0f).add<compages::world::Orbit>();
    m_scene.activeCamera().get<compages::world::Orbit>().spin = 0.35f;
    m_scene.sun();

    // Three by three. A missing file is a plain colour of the same family,
    // so the grid is still full.
    for (std::size_t i = 0u; i < PICTURES.size(); ++i)
    {
        Picture const& picture = PICTURES[i];
        const std::string path = dataPath(picture.file);
        const compages::renderer::Look look = path.empty() ? compages::renderer::color(picture.fallback.x,
                                                             picture.fallback.y,
                                                             picture.fallback.z)
                                              : compages::renderer::texture(path);
        const float column = float(i % 3u) - 1.0f;
        const float row = float(i / 3u) - 1.0f;
        m_boxes.emplace_back(m_scene.box(picture.file, look)
                              .position(column * 3.0f, 0.6f, row * 3.0f)
                              .scale(1.4f));
    }
    return m_scene.prepare();
}

void TextureGallery::draw(Frame const& p_frame)
{
    // The same turn for every box, so the grid stays aligned.
    for (compages::world::Entity& box : m_boxes)
    {
        box.rotate(0.5f * p_frame.elapsed, { 0.3f, 1.0f, 0.0f });
    }
    m_scene.draw(p_frame);
}

} // namespace examples
