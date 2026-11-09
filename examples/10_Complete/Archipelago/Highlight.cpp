// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Highlight.hpp"
#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Meshes.hpp"
#include "10_Complete/Archipelago/Sea.hpp"

#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using compages::renderer::MeshAsset;
using compages::renderer::MeshVertex;
using units::literals::operator""_m;

//! \brief The side of a square of the veil: twice the quad of a fine tile
//! (16 m) under the fine tiles, the quad of a coarse tile (31 m) elsewhere.
//! Between two corners 16 m apart, the bumps of the ground (7 m high every
//! 140 m) rise less than half a metre above a straight line: well under the
//! lift of the veil.
static float fineStep()
{
    return 2.0f * FineTiles::step();
}
static constexpr float COARSE_STEP = MAP / float(BASE_CELLS);
//! \brief The most corners of a square cut by a cell: 4 + one per edge.
static constexpr std::size_t MOST_CORNERS = 24u;

//! \brief How the veil reads the ground: as the screen shows it.
struct Ground
{
    Patch const& base;
    FineTiles const& fine;
    float lift;

    //! \brief A point of the overlay: on the ground as shown (never under
    //! the sea), \p p_extra times the lift over it.
    [[nodiscard]] MeshVertex at(Vector2f p_at, float p_extra = 1.0f) const
    {
        MeshVertex vertex;
        vertex.position =
            Vector3f(p_at.x,
                     std::max(0.0f, fine.groundAt(base, p_at.x, p_at.y)) +
                         (p_extra * lift),
                     p_at.y);
        vertex.normal = Vector3f::UNIT_Y;
        return vertex;
    }
};

//! \brief A small polygon kept on the stack: no memory taken per square.
struct Piece
{
    std::array<Vector2f, MOST_CORNERS> points{};
    std::size_t size = 0u;
};

// ****************************************************************************
//! \brief The outline of a cell, with the side of its edges that is inside.
// ****************************************************************************
struct Outline
{
    std::vector<Vector2f> corners;
    float sign = 1.0f;

    explicit Outline(std::vector<Vector2f> p_corners)
        : corners(std::move(p_corners))
    {
        float area = 0.0f;
        for (std::size_t k = 0u; k < corners.size(); ++k)
        {
            const Vector2f a = corners[k];
            const Vector2f b = corners[(k + 1u) % corners.size()];
            area += (a.x * b.y) - (b.x * a.y);
        }
        sign = (area >= 0.0f) ? 1.0f : -1.0f;
    }

    //! \brief Positive on the inner side of edge \p p_k.
    [[nodiscard]] float side(std::size_t p_k, Vector2f p_p) const
    {
        const Vector2f a = corners[p_k];
        const Vector2f b = corners[(p_k + 1u) % corners.size()];
        return sign *
               (((b.x - a.x) * (p_p.y - a.y)) - ((b.y - a.y) * (p_p.x - a.x)));
    }

    [[nodiscard]] bool contains(Vector2f p_p) const
    {
        for (std::size_t k = 0u; k < corners.size(); ++k)
        {
            if (side(k, p_p) < 0.0f)
            {
                return false;
            }
        }
        return true;
    }

    //! \brief Sutherland-Hodgman: each edge keeps the part of the piece on
    //! its inner side.
    void clip(Piece& p_piece) const
    {
        for (std::size_t k = 0u; (k < corners.size()) && (p_piece.size > 0u);
             ++k)
        {
            Piece kept;
            for (std::size_t n = 0u; n < p_piece.size; ++n)
            {
                const Vector2f p = p_piece.points[n];
                const Vector2f q = p_piece.points[(n + 1u) % p_piece.size];
                const float sp = side(k, p);
                const float sq = side(k, q);
                if ((sp >= 0.0f) && (kept.size < MOST_CORNERS))
                {
                    kept.points[kept.size++] = p;
                }
                if (((sp >= 0.0f) != (sq >= 0.0f)) &&
                    (kept.size < MOST_CORNERS))
                {
                    kept.points[kept.size++] = p + ((q - p) * (sp / (sp - sq)));
                }
            }
            p_piece = kept;
        }
    }
};

// ****************************************************************************
//! \brief The corners of the squares of one step over the box of a region,
//! each made once and shared by the squares around it.
// ****************************************************************************
struct Lattice
{
    float step = 1.0f;
    int i0 = 0;
    int j0 = 0;
    int columns = 0;
    int rows = 0;
    std::vector<std::uint32_t> index;

    Lattice(float p_step, Vector2f p_low, Vector2f p_high) : step(p_step)
    {
        const float origin = -0.5f * MAP;
        i0 = int(std::floor((p_low.x - origin) / step));
        j0 = int(std::floor((p_low.y - origin) / step));
        columns = int(std::floor((p_high.x - origin) / step)) - i0 + 2;
        rows = int(std::floor((p_high.y - origin) / step)) - j0 + 2;
        index.assign(std::size_t(columns) * std::size_t(rows), UINT32_MAX);
    }

    [[nodiscard]] Vector2f point(int p_i, int p_j) const
    {
        return { (-0.5f * MAP) + (float(p_i) * step),
                 (-0.5f * MAP) + (float(p_j) * step) };
    }

    //! \brief The vertex of corner (i, j), made the first time.
    std::uint32_t
    vertex(MeshAsset& p_mesh, Ground const& p_ground, int p_i, int p_j)
    {
        std::uint32_t& k =
            index[(std::size_t(p_j - j0) * std::size_t(columns)) +
                  std::size_t(p_i - i0)];
        if (k == UINT32_MAX)
        {
            k = std::uint32_t(p_mesh.source_vertices.size());
            p_mesh.source_vertices.push_back(p_ground.at(point(p_i, p_j)));
        }
        return k;
    }
};

// ----------------------------------------------------------------------------
//! \brief Two triangles a, b, c and a, c, d, seen from above and below.
// ----------------------------------------------------------------------------
static void quad(MeshAsset& p_mesh,
                 std::uint32_t p_a,
                 std::uint32_t p_b,
                 std::uint32_t p_c,
                 std::uint32_t p_d)
{
    p_mesh.source_indices.insert(
        p_mesh.source_indices.end(),
        { p_a, p_b, p_c, p_a, p_c, p_d, p_a, p_c, p_b, p_a, p_d, p_c });
}

// ----------------------------------------------------------------------------
//! \brief The veil of one cell: the squares of the grid of the ground that
//! its polygon covers. A square inside shares its corners with its
//! neighbours; only a square across the outline is cut, with corners of its
//! own.
// ----------------------------------------------------------------------------
static void veilCell(MeshAsset& p_mesh,
                     Land const& p_land,
                     Ground const& p_ground,
                     Lattice& p_lattice,
                     int p_cell)
{
    const Outline outline(p_land.cellPolygon(p_cell));
    Vector2f low(1.0e30f, 1.0e30f);
    Vector2f high(-1.0e30f, -1.0e30f);
    for (Vector2f const& c : outline.corners)
    {
        low = Vector2f(std::min(low.x, c.x), std::min(low.y, c.y));
        high = Vector2f(std::max(high.x, c.x), std::max(high.y, c.y));
    }
    const float origin = -0.5f * MAP;
    const float step = p_lattice.step;
    const auto i0 = int(std::floor((low.x - origin) / step));
    const auto i1 = int(std::floor((high.x - origin) / step));
    const auto j0 = int(std::floor((low.y - origin) / step));
    const auto j1 = int(std::floor((high.y - origin) / step));
    // Which corners of the grid are in the cell: each one tested once, not
    // once per square around it.
    const auto columns = std::size_t(i1 - i0 + 2);
    std::vector<char> inside(columns * std::size_t(j1 - j0 + 2));
    for (int j = j0; j <= j1 + 1; ++j)
    {
        for (int i = i0; i <= i1 + 1; ++i)
        {
            inside[(std::size_t(j - j0) * columns) + std::size_t(i - i0)] =
                outline.contains(p_lattice.point(i, j)) ? 1 : 0;
        }
    }
    auto in = [&](int p_i, int p_j)
    {
        return inside[(std::size_t(p_j - j0) * columns) +
                      std::size_t(p_i - i0)] != 0;
    };
    for (int j = j0; j <= j1; ++j)
    {
        for (int i = i0; i <= i1; ++i)
        {
            if (in(i, j) && in(i + 1, j) && in(i + 1, j + 1) && in(i, j + 1))
            {
                quad(p_mesh,
                     p_lattice.vertex(p_mesh, p_ground, i, j),
                     p_lattice.vertex(p_mesh, p_ground, i + 1, j),
                     p_lattice.vertex(p_mesh, p_ground, i + 1, j + 1),
                     p_lattice.vertex(p_mesh, p_ground, i, j + 1));
                continue;
            }
            const std::array<Vector2f, 4> square = {
                p_lattice.point(i, j),
                p_lattice.point(i + 1, j),
                p_lattice.point(i + 1, j + 1),
                p_lattice.point(i, j + 1)
            };
            Piece piece;
            for (Vector2f const& p : square)
            {
                piece.points[piece.size++] = p;
            }
            outline.clip(piece);
            if (piece.size < 3u)
            {
                continue;
            }
            const auto first = std::uint32_t(p_mesh.source_vertices.size());
            for (std::size_t k = 0u; k < piece.size; ++k)
            {
                p_mesh.source_vertices.push_back(p_ground.at(piece.points[k]));
            }
            for (std::uint32_t k = 1u; (k + 1u) < std::uint32_t(piece.size);
                 ++k)
            {
                p_mesh.source_indices.insert(p_mesh.source_indices.end(),
                                             { first,
                                               first + k,
                                               first + k + 1u,
                                               first,
                                               first + k + 1u,
                                               first + k });
            }
        }
    }
}

// ----------------------------------------------------------------------------
//! \brief The veil of a region: its cells under the fine tiles on the fine
//! grid, the others on the coarse one.
// ----------------------------------------------------------------------------
static MeshAsset veilOf(Land const& p_land,
                        Ground const& p_ground,
                        int p_region,
                        Vector2f p_low,
                        Vector2f p_high)
{
    MeshAsset veil;
    Lattice fine(fineStep(), p_low, p_high);
    Lattice coarse(COARSE_STEP, p_low, p_high);
    // About one corner per square of the box, and 12 indices per square.
    veil.source_vertices.reserve(fine.index.size() / 2u);
    veil.source_indices.reserve(6u * fine.index.size());
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (p_land.regions_of[c] != p_region)
        {
            continue;
        }
        const Vector2f site = p_land.sites[c];
        const bool detailed = (p_ground.fine.shownMask() &
                               FineTiles::bitAt(site.x, site.y)) != 0u;
        veilCell(veil, p_land, p_ground, detailed ? fine : coarse, int(c));
    }
    return veil;
}

// ----------------------------------------------------------------------------
//! \brief The band: a ribbon \p p_width wide along each segment of the
//! outline, cut in pieces no longer than the fine step so that it follows
//! the ground, seen from both sides.
// ----------------------------------------------------------------------------
static MeshAsset
bandOf(Region const& p_region, Ground const& p_ground, float p_width)
{
    MeshAsset band;
    std::vector<Vector3f> const& border = p_region.border;
    for (std::size_t k = 0u; (k + 1u) < border.size(); k += 2u)
    {
        const Vector2f a(border[k].x, border[k].z);
        const Vector2f b(border[k + 1u].x, border[k + 1u].z);
        const Vector2f along = b - a;
        const float length = std::max(std::hypot(along.x, along.y), 1.0e-3f);
        const Vector2f across =
            Vector2f(-along.y, along.x) * (0.5f * p_width / length);
        const int pieces = std::max(1, int(std::ceil(length / fineStep())));
        for (int s = 0; s < pieces; ++s)
        {
            const Vector2f p = a + (along * (float(s) / float(pieces)));
            const Vector2f q = a + (along * (float(s + 1) / float(pieces)));
            const auto first = std::uint32_t(band.source_vertices.size());
            for (Vector2f const& point :
                 { p - across, p + across, q + across, q - across })
            {
                band.source_vertices.push_back(p_ground.at(point, 1.5f));
            }
            quad(band, first, first + 1u, first + 2u, first + 3u);
        }
    }
    return band;
}

// ----------------------------------------------------------------------------
//! \brief A box around the vertices of a mesh, and its index count.
// ----------------------------------------------------------------------------
static void finish(MeshAsset& p_mesh)
{
    p_mesh.index_count = p_mesh.source_indices.size();
    Vector3f low(1.0e9f, 1.0e9f, 1.0e9f);
    Vector3f high(-1.0e9f, -1.0e9f, -1.0e9f);
    for (MeshVertex const& vertex : p_mesh.source_vertices)
    {
        low = Vector3f(std::min(low.x, vertex.position.x),
                       std::min(low.y, vertex.position.y),
                       std::min(low.z, vertex.position.z));
        high = Vector3f(std::max(high.x, vertex.position.x),
                        std::max(high.y, vertex.position.y),
                        std::max(high.z, vertex.position.z));
    }
    p_mesh.local_bounds = compages::AABB::fromCorners(low, high);
}

// ----------------------------------------------------------------------------
void Highlight::create(compages::renderer::Scene& p_scene)
{
    m_veil =
        p_scene.mesh(hiddenMesh(),
                     "Region veil",
                     compages::renderer::seeThrough(
                         compages::renderer::unlit(1.0f, 0.97f, 0.85f), 0.25f));
    m_band = p_scene.mesh(hiddenMesh(),
                          "Region band",
                          compages::renderer::unlit(1.0f, 0.82f, 0.48f));
    for (compages::world::Entity overlay : { m_veil, m_band })
    {
        overlay.get<compages::renderer::MeshRenderer>().flags =
            compages::renderer::RenderFlags::NoShadowCaster;
        overlay.enable(false);
    }
}

// ----------------------------------------------------------------------------
//! \brief Hide a mesh, or send it to the GPU.
// ----------------------------------------------------------------------------
static void send(compages::renderer::Scene& p_scene,
                 compages::world::Entity p_entity,
                 MeshAsset p_mesh)
{
    if (p_mesh.source_vertices.empty())
    {
        p_entity.enable(false);
        return;
    }
    p_entity.enable(true);
    finish(p_mesh);
    if (const compages::Status sent =
            replaceMesh(p_scene, p_entity, std::move(p_mesh));
        !sent)
    {
        compages::gpu::reportError(sent.error());
    }
}

// ----------------------------------------------------------------------------
//! The veil is the heavy part: it is made again only when the region
//! changes, when fine tiles appear or go under it, or when the camera has
//! moved so much that the lift should change. The band is light: it follows
//! every change of the distance.
// ----------------------------------------------------------------------------
void Highlight::show(compages::renderer::Scene& p_scene,
                     Land const& p_land,
                     Patch const& p_base,
                     FineTiles const& p_fine,
                     int p_region,
                     float p_distance)
{
    const bool changed = (p_region != m_region);
    if (changed)
    {
        m_region = p_region;
        m_width = 0.0f;
        m_lift = 0.0f;
    }
    const bool shown =
        (p_region >= 0) && (std::size_t(p_region) < p_land.regions.size());
    if (!shown)
    {
        m_veil.enable(false);
        m_band.enable(false);
        return;
    }
    if (changed)
    {
        // The box of the region, and the tiles of the map it is on.
        m_low = Vector2f(1.0e30f, 1.0e30f);
        m_high = Vector2f(-1.0e30f, -1.0e30f);
        for (std::size_t c = 0u; c < CELL_COUNT; ++c)
        {
            if (p_land.regions_of[c] == p_region)
            {
                const Vector2f s = p_land.sites[c];
                m_low = Vector2f(std::min(m_low.x, s.x - CELL_SIZE),
                                 std::min(m_low.y, s.y - CELL_SIZE));
                m_high = Vector2f(std::max(m_high.x, s.x + CELL_SIZE),
                                  std::max(m_high.y, s.y + CELL_SIZE));
            }
        }
        m_tiles = 0u;
        for (float z = m_low.y; z < m_high.y + TILE; z += TILE)
        {
            for (float x = m_low.x; x < m_high.x + TILE; x += TILE)
            {
                m_tiles |= FineTiles::bitAt(std::min(x, m_high.x),
                                            std::min(z, m_high.y));
            }
        }
    }

    // The band keeps about 3 pixels wide from where the camera is; the
    // overlay floats a little higher from afar, where the depth is coarser.
    const float width =
        std::max(compages::toMetres(3.0_m), 0.0035f * p_distance);
    const float lift = compages::toMetres(2.0_m) + (0.002f * p_distance);
    const std::uint64_t under = p_fine.shownMask() & m_tiles;
    const bool veil = changed || (under != m_under) || (lift > 1.5f * m_lift) ||
                      (lift < 0.67f * m_lift);
    const bool band = veil || (std::abs(width - m_width) > 0.15f * m_width);
    if (veil)
    {
        m_under = under;
        m_lift = lift;
        send(p_scene,
             m_veil,
             veilOf(p_land,
                    Ground{ p_base, p_fine, lift },
                    p_region,
                    m_low,
                    m_high));
    }
    if (band)
    {
        m_width = width;
        send(p_scene,
             m_band,
             bandOf(p_land.regions[std::size_t(p_region)],
                    Ground{ p_base, p_fine, m_lift },
                    width));
    }
}

} // namespace examples::archipelago
