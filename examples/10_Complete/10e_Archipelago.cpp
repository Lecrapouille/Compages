// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10e_Archipelago.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>

//! \brief The sizes carry their unit (16.0_km, 2.4_km), turned into the
//! metres of the World by compages::toMetres().
using namespace units::literals;

namespace examples
{

namespace
{

using compages::Quatf;
using compages::toMetres;
using compages::Vector2f;
using compages::Vector3f;
using archipelago::MAP;
using archipelago::Patch;
using archipelago::Place;

// --- The themes and their topics --------------------------------------------

//! \brief An island: its name, where it is and how large (in km, before the
//! seed moves it).
struct Theme
{
    char const* name;
    float x;
    float z;
    float radius;
};

constexpr std::array THEMES = {
    Theme{ "Sciences", -3.9f, -3.2f, 3.0f }, Theme{ "Arts", 3.9f, -3.9f, 2.5f },
    Theme{ "Histoire", 4.1f, 3.0f, 2.7f },   Theme{ "Techniques", -3.7f, 3.7f, 2.6f },
    Theme{ "Société", 0.2f, 0.0f, 1.7f },
};

//! \brief A region: its island, its name, its links (one part each).
struct Topic
{
    std::size_t theme;
    char const* name;
    std::array<char const*, 3> links;
};

constexpr std::array TOPICS = {
    Topic{ 0, "Astronomie", { "fr.wikipedia.org/wiki/Astronomie", "fr.wikipedia.org/wiki/Système_solaire", "fr.wikipedia.org/wiki/Galaxie" } },
    Topic{ 0, "Physique", { "fr.wikipedia.org/wiki/Physique", "fr.wikipedia.org/wiki/Mécanique_quantique", "fr.wikipedia.org/wiki/Relativité_générale" } },
    Topic{ 0, "Chimie", { "fr.wikipedia.org/wiki/Chimie", "fr.wikipedia.org/wiki/Tableau_périodique_des_éléments", "fr.wikipedia.org/wiki/Molécule" } },
    Topic{ 0, "Biologie", { "fr.wikipedia.org/wiki/Biologie", "fr.wikipedia.org/wiki/Cellule_(biologie)", "fr.wikipedia.org/wiki/Évolution_(biologie)" } },
    Topic{ 0, "Mathématiques", { "fr.wikipedia.org/wiki/Mathématiques", "fr.wikipedia.org/wiki/Géométrie", "fr.wikipedia.org/wiki/Théorie_des_nombres" } },
    Topic{ 0, "Géologie", { "fr.wikipedia.org/wiki/Géologie", "fr.wikipedia.org/wiki/Tectonique_des_plaques", "fr.wikipedia.org/wiki/Volcan" } },
    Topic{ 1, "Peinture", { "fr.wikipedia.org/wiki/Peinture", "fr.wikipedia.org/wiki/Impressionnisme", "fr.wikipedia.org/wiki/Claude_Monet" } },
    Topic{ 1, "Musique", { "fr.wikipedia.org/wiki/Musique", "fr.wikipedia.org/wiki/Musique_classique", "fr.wikipedia.org/wiki/Jazz" } },
    Topic{ 1, "Cinéma", { "fr.wikipedia.org/wiki/Cinéma", "fr.wikipedia.org/wiki/Frères_Lumière", "fr.wikipedia.org/wiki/Nouvelle_Vague" } },
    Topic{ 1, "Sculpture", { "fr.wikipedia.org/wiki/Sculpture", "fr.wikipedia.org/wiki/Auguste_Rodin", "fr.wikipedia.org/wiki/Camille_Claudel" } },
    Topic{ 1, "Architecture", { "fr.wikipedia.org/wiki/Architecture", "fr.wikipedia.org/wiki/Architecture_gothique", "fr.wikipedia.org/wiki/Le_Corbusier" } },
    Topic{ 2, "Antiquité", { "fr.wikipedia.org/wiki/Antiquité", "fr.wikipedia.org/wiki/Rome_antique", "fr.wikipedia.org/wiki/Grèce_antique" } },
    Topic{ 2, "Moyen Âge", { "fr.wikipedia.org/wiki/Moyen_Âge", "fr.wikipedia.org/wiki/Charlemagne", "fr.wikipedia.org/wiki/Croisades" } },
    Topic{ 2, "Renaissance", { "fr.wikipedia.org/wiki/Renaissance", "fr.wikipedia.org/wiki/Léonard_de_Vinci", "fr.wikipedia.org/wiki/Imprimerie" } },
    Topic{ 2, "Révolution", { "fr.wikipedia.org/wiki/Révolution_française", "fr.wikipedia.org/wiki/Prise_de_la_Bastille", "fr.wikipedia.org/wiki/Maximilien_de_Robespierre" } },
    Topic{ 2, "Époque contemporaine", { "fr.wikipedia.org/wiki/Époque_contemporaine", "fr.wikipedia.org/wiki/Révolution_industrielle", "fr.wikipedia.org/wiki/Seconde_Guerre_mondiale" } },
    Topic{ 3, "Informatique", { "fr.wikipedia.org/wiki/Informatique", "fr.wikipedia.org/wiki/Algorithme", "fr.wikipedia.org/wiki/Alan_Turing" } },
    Topic{ 3, "Robotique", { "fr.wikipedia.org/wiki/Robotique", "fr.wikipedia.org/wiki/Robot_industriel", "fr.wikipedia.org/wiki/Cinématique_inverse" } },
    Topic{ 3, "Électronique", { "fr.wikipedia.org/wiki/Électronique", "fr.wikipedia.org/wiki/Transistor", "fr.wikipedia.org/wiki/Circuit_intégré" } },
    Topic{ 3, "Aéronautique", { "fr.wikipedia.org/wiki/Aéronautique", "fr.wikipedia.org/wiki/Frères_Wright", "fr.wikipedia.org/wiki/Concorde" } },
    Topic{ 3, "Énergie", { "fr.wikipedia.org/wiki/Énergie", "fr.wikipedia.org/wiki/Énergie_solaire", "fr.wikipedia.org/wiki/Énergie_nucléaire" } },
    Topic{ 4, "Philosophie", { "fr.wikipedia.org/wiki/Philosophie", "fr.wikipedia.org/wiki/René_Descartes", "fr.wikipedia.org/wiki/Siècle_des_Lumières" } },
    Topic{ 4, "Économie", { "fr.wikipedia.org/wiki/Économie", "fr.wikipedia.org/wiki/Adam_Smith", "fr.wikipedia.org/wiki/Monnaie" } },
    Topic{ 4, "Géographie", { "fr.wikipedia.org/wiki/Géographie", "fr.wikipedia.org/wiki/Cartographie", "fr.wikipedia.org/wiki/Archipel" } },
    Topic{ 4, "Langues", { "fr.wikipedia.org/wiki/Langue", "fr.wikipedia.org/wiki/Linguistique", "fr.wikipedia.org/wiki/Langue_française" } },
};

//! \brief The name of the page of a link: what follows the last '/', with
//! spaces for the underscores.
std::string pageName(char const* p_link)
{
    std::string name(p_link);
    name = name.substr(name.rfind('/') + 1u);
    std::replace(name.begin(), name.end(), '_', ' ');
    return name;
}

// --- Levels of detail -------------------------------------------------------

//! \brief The coarse map: one patch of 512 quads (31 m) and 1024 pixels (16
//! m), cut into 8 x 8 tiles of 2 km.
constexpr int TILES = 8;
constexpr int BASE_CELLS = 512;
constexpr int BASE_PIXELS = 1024;
const float TILE = MAP / float(TILES);
//! \brief A fine tile: 256 quads (8 m) and 1024 pixels (2 m). At most
//! DETAILS of them live at once, BUILDING of them are made at once.
constexpr int DETAIL_CELLS = 256;
constexpr int DETAIL_PIXELS = 1024;
constexpr std::size_t DETAILS = 12u;
constexpr std::size_t BUILDING = 2u;
//! \brief The fine tiles show when the camera is closer than this to the
//! ground, around what it looks at.
const float DETAILS_WITHIN = toMetres(7.0_km);
//! \brief The skirt of a fine tile: a wall this deep along its edges, which
//! hides the cracks with the coarse tiles next to it.
const float SKIRT = toMetres(25.0_m);

// --- The view ---------------------------------------------------------------

const float ISLAND_NAMES_BEYOND = toMetres(3.5_km);
const float REGION_NAMES_WITHIN = toMetres(9.0_km);
const float PART_NAMES_WITHIN = toMetres(3.2_km);
const float VISIT_DISTANCE = toMetres(2.4_km);
const float OVERVIEW_DISTANCE = toMetres(17.0_km);

//! \brief The fog, and the background (sRGB). The fog goes through the tone
//! curve of the Scene, the background does not: SKY is what HAZE comes out
//! as on the screen (measured), so the horizon has no seam.
const Vector3f HAZE{ 0.74f, 0.84f, 0.90f };
const Vector3f SKY{ 0.81f, 0.855f, 0.88f };

//! \brief A boat, a bird, a fish are a few metres long: from a few km they
//! would be smaller than a pixel. They are drawn at least this part of
//! their distance to the camera long, like the icons of a map.
constexpr float BOAT_SEEN = 1.0f / 1100.0f;
constexpr float BIRD_SEEN = 1.0f / 300.0f;
constexpr float FISH_SEEN = 1.0f / 600.0f;

constexpr float TURN = 6.2831853f;

//! \brief A turn about the vertical: the heading. Forward (-Z) goes to
//! (-sin, 0, -cos) of the angle.
Quatf heading(float p_radians)
{
    return Quatf::fromAngleAxis(units::angle::radian_t(double(p_radians)), Vector3f::UNIT_Y);
}

//! \brief A turn about a local axis, after the heading.
Quatf about(float p_radians, Vector3f const& p_axis)
{
    return Quatf::fromAngleAxis(units::angle::radian_t(double(p_radians)), p_axis);
}

// ----------------------------------------------------------------------------
//! \brief A mesh of a square of a patch: its corners from (p_i, p_j), p_cells
//! quads along a side; the sea flat at zero.
//! \param[in] p_uv The texture coordinates of its first corner, and their
//!   step from a corner to the next.
//! \param[in] p_skirt Add a wall along the edges, SKIRT deep.
// ----------------------------------------------------------------------------
compages::renderer::MeshAsset meshOf(Patch const& p_patch, int p_i, int p_j, int p_cells,
                                     Vector2f p_uv, float p_uv_step, bool p_skirt)
{
    compages::renderer::MeshAsset mesh;
    const float step = p_patch.size / float(p_patch.cells);
    auto ground = [&](int p_a, int p_b) { return std::max(0.0f, p_patch.height(p_a, p_b)); };
    const int side = p_cells + 1;
    float top = 0.0f;
    for (int j = 0; j < side; ++j)
    {
        for (int i = 0; i < side; ++i)
        {
            const int a = p_i + i;
            const int b = p_j + j;
            compages::renderer::MeshVertex vertex;
            vertex.position = Vector3f(p_patch.x + (float(a) * step), ground(a, b), p_patch.z + (float(b) * step));
            vertex.normal = compages::vector::normalize(
                Vector3f(ground(a - 1, b) - ground(a + 1, b), 2.0f * step, ground(a, b - 1) - ground(a, b + 1)));
            vertex.uv = p_uv + (Vector2f(float(i), float(j)) * p_uv_step);
            mesh.source_vertices.push_back(vertex);
            top = std::max(top, vertex.position.y);
        }
    }
    for (int j = 0; j < p_cells; ++j)
    {
        for (int i = 0; i < p_cells; ++i)
        {
            // Counterclockwise seen from above.
            const std::uint32_t a = std::uint32_t((j * side) + i);
            const std::uint32_t b = a + 1u;
            const std::uint32_t d = a + std::uint32_t(side);
            const std::uint32_t c = d + 1u;
            for (const std::uint32_t index : { a, d, b, b, d, c })
            {
                mesh.source_indices.push_back(index);
            }
        }
    }
    if (p_skirt)
    {
        // Each edge again, SKIRT lower, and a strip between the two, seen
        // from both sides.
        const std::array<std::array<int, 4>, 4> edges = { { { 0, 0, 1, 0 }, { side - 1, 0, 0, 1 },
                                                            { 0, side - 1, 1, 0 }, { 0, 0, 0, 1 } } };
        for (std::array<int, 4> const& edge : edges)
        {
            const std::uint32_t first = std::uint32_t(mesh.source_vertices.size());
            for (int k = 0; k < side; ++k)
            {
                const std::size_t top_index = std::size_t(((edge[1] + (k * edge[3])) * side) + edge[0] + (k * edge[2]));
                compages::renderer::MeshVertex low = mesh.source_vertices[top_index];
                low.position.y -= SKIRT;
                mesh.source_vertices.push_back(low);
            }
            for (int k = 0; k + 1 < side; ++k)
            {
                const std::uint32_t a = std::uint32_t(((edge[1] + (k * edge[3])) * side) + edge[0] + (k * edge[2]));
                const std::uint32_t b = std::uint32_t(((edge[1] + ((k + 1) * edge[3])) * side) + edge[0] + ((k + 1) * edge[2]));
                const std::uint32_t la = first + std::uint32_t(k);
                const std::uint32_t lb = la + 1u;
                for (const std::uint32_t index : { a, b, la, b, lb, la, a, la, b, b, la, lb })
                {
                    mesh.source_indices.push_back(index);
                }
            }
        }
    }
    mesh.index_count = mesh.source_indices.size();
    const float x0 = p_patch.x + (float(p_i) * step);
    const float z0 = p_patch.z + (float(p_j) * step);
    mesh.local_bounds = compages::AABB::fromCorners(
        Vector3f(x0, -SKIRT, z0), Vector3f(x0 + (float(p_cells) * step), top, z0 + (float(p_cells) * step)));
    return mesh;
}

//! \brief A picture of the map for the GPU: sRGB (the alpha, the water,
//! stays linear), with mipmaps for the view from afar.
compages::renderer::TextureAsset pictureOf(std::string p_name, Patch const& p_patch)
{
    compages::renderer::TextureAsset picture;
    picture.name = std::move(p_name);
    picture.description = compages::gpu::TextureDesc::image(std::uint32_t(p_patch.pixels), std::uint32_t(p_patch.pixels),
                                                            compages::gpu::PixelFormat::SRGB8A8);
    picture.description.levels = 0u;
    picture.pixels = p_patch.picture;
    return picture;
}

// --- The sea ----------------------------------------------------------------

// The main() of the map: the PBR shader of the Renderer (its lights, its
// shadow, its fog and its tone curve are kept) with water where the alpha
// of the picture says: the sea from 0 (the shore) to 0.9 (90 m deep), a
// river at 0.94, the land at 1.
//
// The swells are sums of sines. Each one obeys the dispersion of deep
// water, omega^2 = g k: a swell 233 m long runs at 19 m/s with a period of
// 12 s. Where a wave is smaller than a pixel (fwidth of its phase), it
// fades instead of shimmering. The water mirrors the sky more at a grazing
// look (Schlick's Fresnel), the sun glints in it, and bands of surf run to
// the shore. A river gets the short ripples, the sky and the glint.
constexpr char const* SEA_MAIN = R"(
uniform float time;

const float PI = 3.14159265;
const float GRAVITY = 9.81;
// Spread over a wide fan, with wavelengths that share no ratio: the sum
// never repeats into stripes. Direction (x, z), wavelength in metres,
// steepness (amplitude times k).
const int SWELL_COUNT = 8;
const vec4 SWELLS[SWELL_COUNT] = vec4[SWELL_COUNT](vec4( 0.80,  0.60, 233.0, 0.045),
                                                   vec4( 0.20,  0.98, 151.0, 0.040),
                                                   vec4( 0.98, -0.20,  97.0, 0.040),
                                                   vec4( 0.55,  0.83,  61.0, 0.035),
                                                   vec4(-0.30,  0.95,  39.0, 0.030),
                                                   vec4( 0.90,  0.43,  27.0, 0.030),
                                                   vec4( 0.10, -0.99,  17.0, 0.025),
                                                   vec4( 0.71,  0.71,  11.0, 0.020));

out vec4 oColor;

void main()
{
    vec4 texel = hasBaseColorMap ? texture(baseColorMap, vUv) : vec4(1.0);
    vec3 base = texel.rgb * baseColorFactor;
    if (texel.a > 0.97)
    {
        oColor = vec4(present(applyFog(shade(base, normalize(vNormal), vWorldPos), vWorldPos)), opacity);
        return;
    }
    bool river = texel.a > 0.915;
    float depth = river ? 0.5 : texel.a / 0.9;
    vec2 p = vWorldPos.xz;
    vec2 slope = vec2(0.0);
    for (int i = river ? 5 : 0; i < SWELL_COUNT; ++i)
    {
        vec2 d = normalize(SWELLS[i].xy);
        float k = 2.0 * PI / SWELLS[i].z;
        float omega = sqrt(GRAVITY * k);
        float phase = k * dot(d, p) - omega * time + float(i) * 1.7;
        float fade = 1.0 - smoothstep(0.3, 1.2, fwidth(phase));
        slope += d * (SWELLS[i].w * cos(phase) * fade);
    }
    // Gusts: patches where the wind ruffles the water more, drifting.
    slope *= 0.6 + 0.4 * sin(p.x * 0.0021 + time * 0.05) * sin(p.y * 0.0017 - time * 0.04);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 n = normalize(vec3(-slope.x, 1.0, -slope.y));
    vec3 toEye = normalize(cameraPos - vWorldPos);
    vec3 toLight = normalize(-lightDir);
    // The sky in the water, from a normal half way to the vertical: a
    // softer Fresnel, without bright stripes at a grazing look.
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(normalize(n + up), toEye), 0.0), 5.0);
    vec3 water = shade(base, up, vWorldPos);
    water = mix(water, fogColor * 1.1, 0.7 * fresnel);
    float sun = sunVisibility(up, toLight, vWorldPos);
    water += lightColor * (2.5 * sun * pow(max(dot(reflect(-toLight, n), toEye), 0.0), 240.0));
    if (!river)
    {
        // Surf: bands that run toward the shore over the last metres of
        // depth, broken into pieces, and a fringe that never leaves the beach.
        float shallow = 1.0 - smoothstep(0.0, 0.07, depth);
        float bands = sin(depth * 260.0 + time * 1.4);
        float broken = 0.5 + 0.5 * sin(p.x * 0.031 + time * 0.21) * sin(p.y * 0.027 - time * 0.17);
        float foam = shallow * smoothstep(0.55, 0.95, bands) * broken + (1.0 - smoothstep(0.0, 0.015, depth)) * 0.7;
        water = mix(water, vec3(0.85, 0.9, 0.9) * (0.6 + 0.4 * sun), clamp(foam, 0.0, 1.0) * 0.75);
    }
    oColor = vec4(present(applyFog(water, vWorldPos)), opacity);
}
)";

} // namespace

// ----------------------------------------------------------------------------
Archipelago::~Archipelago()
{
    // The threads read the land: they end before it goes.
    if (m_loading.valid())
    {
        m_loading.wait();
    }
    for (Detail& detail : m_details)
    {
        if (detail.job.valid())
        {
            detail.job.wait();
        }
    }
}

// ----------------------------------------------------------------------------
std::string Archipelago::description() const
{
    return "A map of knowledge after Chréage: islands of Voronoi cells whose "
           "relief grows by uplift and river erosion, regions and parts made "
           "of cells, rivers that run down from cell to cell and dig their "
           "valleys in the mesh. Made in a thread behind a progress bar; "
           "near the camera, finer tiles (8 m, 2 m a pixel) replace the "
           "coarse ones. The sea is the PBR shader with another main(). Left "
           "drag moves, right drag turns, the wheel zooms toward the mouse; "
           "a click flies to a part.";
}

// ----------------------------------------------------------------------------
compages::Status Archipelago::setUp()
{
    const std::filesystem::path font = dataPath("fonts/DejaVuSans.ttf");
    if (font.empty())
    {
        return compages::failure("fonts/DejaVuSans.ttf is missing from external/Compages-data/");
    }
    m_scene.font(font);
    m_scene.background(SKY.x, SKY.y, SKY.z).ambient(0.32f, 0.36f, 0.42f);
    m_scene.environment().fog_color = HAZE;
    // exp(-(density * d)^2): clear over the archipelago, haze at the horizon.
    m_scene.environment().fog_density = 1.0f / toMetres(70.0_km);
    // The shadow picture spans the 16 km of the map: 4 m a pixel.
    m_scene.environment().shadow_resolution = 4096u;
    // A low sun from the south-west: the relief shows, the sea glints.
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.6f)
        .position(-0.55f * MAP, 0.45f * MAP, 0.65f * MAP)
        .lookAt(0.0f, 0.0f, 0.0f);

    for (Theme const& theme : THEMES)
    {
        m_island_names.push_back(m_world.entity(theme.name));
    }
    for (Topic const& topic : TOPICS)
    {
        m_region_names.push_back(m_world.entity(topic.name));
        m_part_names.emplace_back();
        for (char const* link : topic.links)
        {
            m_part_names.back().push_back(m_world.entity(pageName(link)));
        }
    }

    m_camera = m_scene.camera().position(0.0f, 0.75f * OVERVIEW_DISTANCE, 0.66f * OVERVIEW_DISTANCE);
    compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
    lens.near_plane = toMetres(2.0_m);
    lens.far_plane = toMetres(400.0_km);
    // A Rig: left drag moves the map, right drag turns, the wheel zooms
    // toward the mouse.
    m_camera.add<compages::world::Rig>(Vector3f::ZERO);
    compages::world::RigController& rig = m_camera.get<compages::world::Rig>().controller;
    rig.min_distance = toMetres(150.0_m);
    rig.max_distance = toMetres(60.0_km);
    rig.max_pitch = -0.12f;
    m_camera.set(compages::renderer::Caption{ .text = "",
                                              .color = { 1.0f, 0.97f, 0.90f },
                                              .pixels = 17.0f,
                                              .at = { 12.0f, 10.0f } });

    COMPAGES_TRY(makeSea());
    // The land is made in a thread: setUp() returns at once, the gallery
    // shows how far it has gone (progress()), draw() shows it when ready.
    startLoading();
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Archipelago::startLoading()
{
    archipelago::Recipe recipe;
    recipe.seed = std::uint32_t(m_seed);
    recipe.erosion_steps = m_erosion_steps;
    for (Theme const& theme : THEMES)
    {
        recipe.centres.emplace_back(1000.0f * theme.x, 1000.0f * theme.z);
        recipe.radii.push_back(1000.0f * theme.radius);
    }
    for (Topic const& topic : TOPICS)
    {
        recipe.region_islands.push_back(topic.theme);
        recipe.region_parts.push_back(topic.links.size());
    }
    m_report = std::make_shared<Report>();
    std::shared_ptr<Report> report = m_report;
    m_loading = std::async(
        std::launch::async,
        [recipe, report]()
        {
            auto tell = [&](float p_done, char const* p_step)
            {
                std::lock_guard<std::mutex> lock(report->mutex);
                report->done = p_done;
                report->step = p_step;
            };
            Loaded loaded;
            loaded.land = archipelago::makeLand(recipe, [&](float p_done, char const* p_step) { tell(0.4f * p_done, p_step); });
            loaded.base = archipelago::makePatch(loaded.land, -0.5f * MAP, -0.5f * MAP, MAP, BASE_CELLS, BASE_PIXELS,
                                                 [&](float p_done, char const* p_step)
                                                 { tell(0.4f + (0.6f * p_done), p_step); });
            return loaded;
        });
}

// ----------------------------------------------------------------------------
Archipelago::Progress Archipelago::progress() const
{
    if (!m_loading.valid())
    {
        return { 1.0f, {} };
    }
    std::lock_guard<std::mutex> lock(m_report->mutex);
    // 1 only once draw() has shown the land.
    return { std::min(m_report->done, 0.999f), m_report->step };
}

// ----------------------------------------------------------------------------
compages::Status Archipelago::makeSea()
{
    // The PBR shader of the Renderer up to its main(), then ours: the
    // renderer still sets every uniform it knows (lights, shadow, fog).
    std::string source = compages::renderer::shaders::pbrFragment();
    source = source.substr(0u, source.find("out vec4 oColor;")) + SEA_MAIN;
    compages::renderer::Material material;
    material.name = "ArchipelagoSea";
    material.family = compages::renderer::ShaderFamily::PbrMinimal;
    COMPAGES_TRY(material.program.loadSources(compages::renderer::shaders::pbrVertex(), source));
    COMPAGES_TRY_ASSIGN(material.pipeline,
                        compages::gpu::Pipeline::create<compages::renderer::MeshVertex>(
                            material.program,
                            compages::gpu::RenderState{ .depth_test = true, .cull = compages::gpu::CullMode::Back }));
    COMPAGES_TRY_ASSIGN(m_sea, m_scene.assets().addMaterial(material.name, std::move(material)));
    return compages::success();
}

// ----------------------------------------------------------------------------
compages::Status Archipelago::wearSea(compages::world::Entity p_entity)
{
    compages::renderer::MaterialInstance* instance =
        m_scene.assets().materialInstance(p_entity.get<compages::renderer::MeshRenderer>().material_instance);
    if (instance == nullptr)
    {
        return compages::failure("A tile of the map has no material instance");
    }
    instance->material = m_sea;
    return compages::success();
}

// ----------------------------------------------------------------------------
compages::Status Archipelago::finishLoading(Loaded p_loaded)
{
    // The fine tiles of the old land are made from it: they end first.
    dropDetails();
    m_land = std::move(p_loaded.land);
    m_base = std::move(p_loaded.base);

    // --- The coarse map: 8 x 8 tiles of one patch, one picture -------------
    constexpr int CELLS_PER_TILE = BASE_CELLS / TILES;
    const bool first = m_tiles.empty();
    if (first)
    {
        COMPAGES_TRY_ASSIGN(m_picture, m_scene.assets().addTexture("Archipelago map", pictureOf("Archipelago map", m_base)));
    }
    else
    {
        compages::renderer::TextureAsset* picture = m_scene.assets().texture(m_picture);
        if (picture == nullptr)
        {
            return compages::failure("The map lost its picture");
        }
        COMPAGES_TRY(picture->texture.write(m_base.picture));
        COMPAGES_TRY(picture->texture.generateMipmaps());
        picture->pixels = m_base.picture;
    }
    for (int tz = 0; tz < TILES; ++tz)
    {
        for (int tx = 0; tx < TILES; ++tx)
        {
            const int i = tx * CELLS_PER_TILE;
            const int j = tz * CELLS_PER_TILE;
            compages::renderer::MeshAsset mesh =
                meshOf(m_base, i, j, CELLS_PER_TILE, Vector2f(float(i), float(j)) * (1.0f / float(BASE_CELLS)),
                       1.0f / float(BASE_CELLS), false);
            if (first)
            {
                compages::world::Entity tile = m_scene.mesh(std::move(mesh), "Map tile");
                m_scene.look(tile, m_picture);
                COMPAGES_TRY(wearSea(tile));
                m_tiles.push_back(tile);
            }
            else
            {
                compages::world::Entity tile = m_tiles[std::size_t((tz * TILES) + tx)];
                compages::renderer::MeshAsset* current =
                    m_scene.assets().mesh(tile.get<compages::renderer::MeshRenderer>().mesh);
                if (current == nullptr)
                {
                    return compages::failure("A tile of the map lost its mesh");
                }
                current->source_vertices = std::move(mesh.source_vertices);
                current->local_bounds = mesh.local_bounds;
                current->releaseGpu();
                COMPAGES_TRY(current->upload());
                tile.enable(true);
            }
        }
    }
    if (first)
    {
        // The ocean around the map: four trapezoids out to 150 km, reading
        // the deep water of the edge of the picture. Outside the shadows:
        // the shadow picture stays fitted to the islands.
        const float far = toMetres(150.0_km);
        const float near = 0.5f * MAP;
        compages::renderer::MeshAsset skirt;
        const std::array<Vector2f, 4> inner = { Vector2f(-near, -near), Vector2f(near, -near), Vector2f(near, near),
                                                Vector2f(-near, near) };
        for (Vector2f const& corner : inner)
        {
            for (const float reach : { near, far })
            {
                compages::renderer::MeshVertex vertex;
                const Vector2f p = corner * (reach / near);
                vertex.position = Vector3f(p.x, 0.0f, p.y);
                vertex.normal = Vector3f::UNIT_Y;
                vertex.uv = Vector2f((corner.x > 0.0f) ? 1.0f : 0.0f, (corner.y > 0.0f) ? 1.0f : 0.0f);
                skirt.source_vertices.push_back(vertex);
            }
        }
        for (std::uint32_t k = 0u; k < 4u; ++k)
        {
            const std::uint32_t a = 2u * k;
            const std::uint32_t b = 2u * ((k + 1u) % 4u);
            for (const std::uint32_t index : { a, b, a + 1u, b, b + 1u, a + 1u })
            {
                skirt.source_indices.push_back(index);
            }
        }
        skirt.index_count = skirt.source_indices.size();
        skirt.local_bounds = compages::AABB::fromCorners(Vector3f(-far, -1.0f, -far), Vector3f(far, 1.0f, far));
        m_ocean = m_scene.mesh(std::move(skirt), "Ocean");
        m_scene.look(m_ocean, m_picture);
        COMPAGES_TRY(wearSea(m_ocean));
        m_ocean.get<compages::renderer::MeshRenderer>().flags = compages::renderer::RenderFlags::NoShadowCaster;
    }

    // --- The names ---------------------------------------------------------
    for (std::size_t k = 0u; k < m_island_names.size(); ++k)
    {
        m_island_names[k]
            .position(m_land.islands[k].peak + Vector3f(0.0f, toMetres(80.0_m), 0.0f))
            .set(compages::renderer::Label{ .text = THEMES[k].name,
                                            .color = { 1.0f, 0.97f, 0.88f },
                                            .pixels = 24.0f,
                                            .hide_closer_than = ISLAND_NAMES_BEYOND });
    }
    for (std::size_t r = 0u; r < m_region_names.size(); ++r)
    {
        archipelago::Region const& region = m_land.regions[r];
        m_region_names[r].position(region.site.x, std::max(0.0f, m_land.heightAt(region.site.x, region.site.y)) +
                                                      toMetres(30.0_m),
                                   region.site.y);
        for (std::size_t part = 0u; part < region.parts.size(); ++part)
        {
            const Vector2f site = region.parts[part].site;
            m_part_names[r][part].position(site.x, std::max(0.0f, m_land.heightAt(site.x, site.y)) + toMetres(20.0_m),
                                           site.y);
        }
    }
    m_hovered = {};
    m_chosen = {};
    placeAnimals();
    m_ready = true;
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
float Archipelago::baseHeight(float p_x, float p_z) const
{
    const float step = m_base.size / float(m_base.cells);
    const float gx = std::clamp((p_x - m_base.x) / step, 0.0f, float(m_base.cells) - 0.001f);
    const float gz = std::clamp((p_z - m_base.z) / step, 0.0f, float(m_base.cells) - 0.001f);
    const int i = int(gx);
    const int j = int(gz);
    const float tx = gx - float(i);
    const float tz = gz - float(j);
    return (m_base.height(i, j) * (1.0f - tx) * (1.0f - tz)) + (m_base.height(i + 1, j) * tx * (1.0f - tz)) +
           (m_base.height(i, j + 1) * (1.0f - tx) * tz) + (m_base.height(i + 1, j + 1) * tx * tz);
}

// ----------------------------------------------------------------------------
void Archipelago::dropDetails()
{
    for (Detail& detail : m_details)
    {
        if (detail.job.valid())
        {
            detail.job.wait();
            (void)detail.job.get();
        }
        detail.tile = -1;
        detail.ready = false;
        if (detail.entity)
        {
            detail.entity.enable(false);
        }
    }
}

// ----------------------------------------------------------------------------
void Archipelago::updateDetails()
{
    ++m_clock;
    // The tiles wanted: around what the camera looks at, when it is close
    // enough, the nearest first.
    compages::world::RigController const& rig = m_camera.get<compages::world::Rig>().controller;
    const Vector3f target = rig.currentTarget();
    const float distance = rig.currentDistance();
    std::vector<std::pair<float, int>> wanted;
    if (m_details_on && (distance < DETAILS_WITHIN))
    {
        const float radius = std::clamp(0.9f * distance, toMetres(1.5_km), toMetres(3.5_km));
        for (int tile = 0; tile < TILES * TILES; ++tile)
        {
            const float x0 = (-0.5f * MAP) + (float(tile % TILES) * TILE);
            const float z0 = (-0.5f * MAP) + (float(tile / TILES) * TILE);
            const float dx = std::max({ x0 - target.x, 0.0f, target.x - (x0 + TILE) });
            const float dz = std::max({ z0 - target.z, 0.0f, target.z - (z0 + TILE) });
            const float gap = std::sqrt((dx * dx) + (dz * dz));
            if (gap < radius)
            {
                wanted.emplace_back(gap, tile);
            }
        }
        std::sort(wanted.begin(), wanted.end());
        wanted.resize(std::min(wanted.size(), DETAILS - BUILDING - 1u));
    }
    auto isWanted = [&](int p_tile)
    { return std::any_of(wanted.begin(), wanted.end(), [&](auto const& p_w) { return p_w.second == p_tile; }); };

    // The tiles made since the last frame go to the GPU.
    for (std::size_t k = 0u; k < m_details.size(); ++k)
    {
        Detail& detail = m_details[k];
        if (!detail.job.valid() || (detail.job.wait_for(std::chrono::seconds(0)) != std::future_status::ready))
        {
            continue;
        }
        const Patch patch = detail.job.get();
        compages::renderer::MeshAsset mesh = meshOf(patch, 0, 0, patch.cells, Vector2f(0.0f, 0.0f),
                                                    1.0f / float(patch.cells), true);
        compages::Status sent = compages::success();
        if (!detail.entity)
        {
            const std::string name = "Archipelago tile " + std::to_string(k);
            auto texture = m_scene.assets().addTexture(name, pictureOf(name, patch));
            if (!texture)
            {
                compages::gpu::reportError(texture.error());
                continue;
            }
            detail.texture = texture.value();
            detail.entity = m_scene.mesh(std::move(mesh), "Detail tile");
            m_scene.look(detail.entity, detail.texture);
            sent = wearSea(detail.entity);
            if (sent)
            {
                sent = m_scene.prepare();
            }
        }
        else
        {
            compages::renderer::MeshAsset* current =
                m_scene.assets().mesh(detail.entity.get<compages::renderer::MeshRenderer>().mesh);
            compages::renderer::TextureAsset* picture = m_scene.assets().texture(detail.texture);
            if ((current != nullptr) && (picture != nullptr))
            {
                current->source_vertices = std::move(mesh.source_vertices);
                current->source_indices = std::move(mesh.source_indices);
                current->index_count = mesh.index_count;
                current->local_bounds = mesh.local_bounds;
                current->releaseGpu();
                sent = current->upload();
                if (sent)
                {
                    sent = picture->texture.write(patch.picture);
                }
                if (sent)
                {
                    sent = picture->texture.generateMipmaps();
                }
            }
        }
        if (!sent)
        {
            compages::gpu::reportError(sent.error());
            detail.tile = -1;
            continue;
        }
        detail.ready = true;
    }

    // The wanted tiles nobody makes yet: a free slot, or the one unused for
    // the longest; at most BUILDING at once.
    std::size_t building = std::size_t(
        std::count_if(m_details.begin(), m_details.end(), [](Detail const& p_d) { return p_d.job.valid(); }));
    for (auto const& [gap, tile] : wanted)
    {
        (void)gap;
        const bool known = std::any_of(m_details.begin(), m_details.end(), [&](Detail const& p_d) { return p_d.tile == tile; });
        if (known || (building >= BUILDING))
        {
            continue;
        }
        Detail* slot = nullptr;
        if (m_details.size() < DETAILS)
        {
            m_details.emplace_back();
            slot = &m_details.back();
        }
        else
        {
            for (Detail& detail : m_details)
            {
                if (!detail.job.valid() && !isWanted(detail.tile) && ((slot == nullptr) || (detail.used < slot->used)))
                {
                    slot = &detail;
                }
            }
        }
        if (slot == nullptr)
        {
            break;
        }
        slot->tile = tile;
        slot->ready = false;
        const float x0 = (-0.5f * MAP) + (float(tile % TILES) * TILE);
        const float z0 = (-0.5f * MAP) + (float(tile / TILES) * TILE);
        archipelago::Land const* land = &m_land;
        slot->job = std::async(std::launch::async, [land, x0, z0]()
                               { return archipelago::makePatch(*land, x0, z0, TILE, DETAIL_CELLS, DETAIL_PIXELS, {}); });
        ++building;
    }

    // A fine tile ready and wanted takes the place of its coarse one.
    std::vector<char> fine(std::size_t(TILES * TILES), 0);
    for (Detail& detail : m_details)
    {
        const bool shown = detail.ready && (detail.tile >= 0) && isWanted(detail.tile);
        if (detail.entity)
        {
            detail.entity.enable(shown);
        }
        if (shown)
        {
            detail.used = m_clock;
            fine[std::size_t(detail.tile)] = 1;
        }
    }
    for (std::size_t tile = 0u; tile < m_tiles.size(); ++tile)
    {
        m_tiles[tile].enable(fine[tile] == 0);
    }
}

// ----------------------------------------------------------------------------
void Archipelago::placeAnimals()
{
    std::mt19937 random(std::uint32_t(m_seed) + 1000u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    auto isSea = [&](Vector2f p_point, float p_depth)
    {
        return (std::abs(p_point.x) < 0.48f * MAP) && (std::abs(p_point.y) < 0.48f * MAP) &&
               (baseHeight(p_point.x, p_point.y) < -p_depth);
    };

    // --- Boats: round an island, where the water is 8 m deep or more, at a
    // little distance from that line. The loop is smoothed; a loop that
    // would cross land (another island) is not sailed.
    if (m_boats.empty())
    {
        const compages::renderer::Look wood = compages::renderer::color(0.45f, 0.30f, 0.20f);
        const compages::renderer::Look white = compages::renderer::color(0.95f, 0.95f, 0.92f);
        for (int k = 0; k < 7; ++k)
        {
            Boat boat{};
            boat.root = m_world.entity("Boat");
            m_scene.box("Hull", wood).parent(boat.root).position(0.0f, 0.6f, 0.0f).scale(3.2f, 1.6f, 11.0f);
            m_scene.box("Cabin", white).parent(boat.root).position(0.0f, 1.8f, 1.8f).scale(2.2f, 1.0f, 3.0f);
            m_scene.cylinder("Mast", wood).parent(boat.root).position(0.0f, 6.0f, -1.0f).scale(0.3f, 9.0f, 0.3f);
            m_scene.box("Sail", white).parent(boat.root).position(0.0f, 6.0f, 0.6f).scale(0.12f, 7.0f, 3.4f);
            m_boats.push_back(boat);
        }
    }
    for (Boat& boat : m_boats)
    {
        boat.radii.clear();
        for (int attempt = 0; (attempt < 12) && boat.radii.empty(); ++attempt)
        {
            archipelago::Island const& island = m_land.islands[random() % m_land.islands.size()];
            const float margin = 150.0f + (350.0f * unit(random));
            std::vector<float> radii(64u, 0.0f);
            for (std::size_t a = 0u; a < radii.size(); ++a)
            {
                const float angle = TURN * float(a) / float(radii.size());
                const Vector2f way(std::cos(angle), std::sin(angle));
                float r = 0.3f * island.radius;
                while ((r < 2.5f * island.radius) && !isSea(island.centre + (way * r), 8.0f))
                {
                    r += 40.0f;
                }
                radii[a] = r + margin;
            }
            for (int pass = 0; pass < 4; ++pass)
            {
                std::vector<float> smooth(radii);
                for (std::size_t a = 0u; a < radii.size(); ++a)
                {
                    smooth[a] = std::max(radii[a],
                                         0.25f * (radii[(a + 63u) % 64u] + (2.0f * radii[a]) + radii[(a + 1u) % 64u]));
                }
                radii = smooth;
            }
            bool clear = true;
            for (std::size_t a = 0u; (a < radii.size()) && clear; ++a)
            {
                const float angle = TURN * float(a) / float(radii.size());
                clear = isSea(island.centre + (Vector2f(std::cos(angle), std::sin(angle)) * radii[a]), 4.0f);
            }
            if (clear)
            {
                boat.centre = island.centre;
                boat.radii = radii;
            }
        }
        boat.angle = TURN * unit(random);
        // 5 to 9 m/s (10 to 17 knots), either way round.
        boat.speed = (5.0f + (4.0f * unit(random))) * ((unit(random) < 0.5f) ? -1.0f : 1.0f);
        boat.root.position(0.0f, -100.0f, 0.0f);   // under the sea until it sails
    }

    // --- Birds: gulls circling over the islands, three per island ---------
    if (m_birds.empty())
    {
        const compages::renderer::Look feathers = compages::renderer::color(0.93f, 0.93f, 0.95f);
        const compages::renderer::Look tips = compages::renderer::color(0.35f, 0.36f, 0.40f);
        for (int k = 0; k < 15; ++k)
        {
            Bird bird{};
            bird.root = m_world.entity("Gull");
            m_scene.box("Body", feathers).parent(bird.root).scale(0.25f, 0.22f, 0.8f);
            bird.left = m_world.entity("Left wing").parent(bird.root).position(-0.12f, 0.05f, 0.0f);
            bird.right = m_world.entity("Right wing").parent(bird.root).position(0.12f, 0.05f, 0.0f);
            m_scene.box("Wing", feathers).parent(bird.left).position(-0.35f, 0.0f, 0.0f).scale(0.7f, 0.04f, 0.3f);
            m_scene.box("Tip", tips).parent(bird.left).position(-0.85f, 0.0f, 0.0f).scale(0.3f, 0.04f, 0.22f);
            m_scene.box("Wing", feathers).parent(bird.right).position(0.35f, 0.0f, 0.0f).scale(0.7f, 0.04f, 0.3f);
            m_scene.box("Tip", tips).parent(bird.right).position(0.85f, 0.0f, 0.0f).scale(0.3f, 0.04f, 0.22f);
            m_birds.push_back(bird);
        }
    }
    for (std::size_t k = 0u; k < m_birds.size(); ++k)
    {
        Bird& bird = m_birds[k];
        archipelago::Island const& island = m_land.islands[(k / 3u) % m_land.islands.size()];
        bird.centre = Vector3f(island.centre.x + (300.0f * (unit(random) - 0.5f)),
                               island.peak.y + 120.0f + (120.0f * unit(random)),
                               island.centre.y + (300.0f * (unit(random) - 0.5f)));
        bird.radius = (0.35f + (0.3f * unit(random))) * island.radius;
        bird.angle = TURN * unit(random);
        bird.speed = 11.0f + (4.0f * unit(random));   // m/s, a gull
        bird.phase = TURN * unit(random);
    }

    // --- Fish: schools in the lagoons, 2 to 10 m deep ----------------------
    if (m_fish.empty())
    {
        for (int k = 0; k < 24; ++k)
        {
            Fish fish{};
            // Seen through the water: a dark shape just under the surface.
            fish.shadow = m_scene.sphere(
                "Fish", compages::renderer::seeThrough(compages::renderer::unlit(0.03f, 0.12f, 0.16f), 0.45f));
            fish.shadow.get<compages::renderer::MeshRenderer>().flags = compages::renderer::RenderFlags::NoShadowCaster;
            // Out of the water: silver, and lit.
            fish.jumper = m_scene.sphere("Jumping fish", compages::renderer::color(0.75f, 0.80f, 0.85f));
            m_fish.push_back(fish);
        }
    }
    std::vector<Vector2f> lagoons;
    for (int tries = 0; (tries < 20000) && (lagoons.size() < 6u); ++tries)
    {
        const Vector2f p((unit(random) - 0.5f) * MAP, (unit(random) - 0.5f) * MAP);
        const float h = baseHeight(p.x, p.y);
        if ((h < -2.0f) && (h > -10.0f))
        {
            lagoons.push_back(p);
        }
    }
    for (std::size_t k = 0u; k < m_fish.size(); ++k)
    {
        Fish& fish = m_fish[k];
        fish.centre = lagoons.empty() ? Vector2f(0.0f, 0.0f) : lagoons[k % lagoons.size()];
        fish.radius = 15.0f + (40.0f * unit(random));
        fish.angle = TURN * unit(random);
        fish.speed = 0.8f + (1.2f * unit(random));   // m/s
        fish.wait = 2.0f + (20.0f * unit(random));
        fish.jump = -1.0f;
    }
}

// ----------------------------------------------------------------------------
void Archipelago::animate(float p_dt)
{
    const Vector3f eye = m_camera.position();
    // At least the size the eye needs from where it is.
    auto seen = [&](Vector3f const& p_at, float p_part)
    { return std::max(1.0f, compages::vector::norm(p_at - eye) * p_part); };

    for (Boat& boat : m_boats)
    {
        if (boat.radii.empty())
        {
            continue;
        }
        // The radius between the two samples of the loop around the angle.
        auto radius = [&](float p_angle)
        {
            const float at = (std::fmod(p_angle + TURN, TURN) / TURN) * 64.0f;
            const float base = std::floor(at);
            const std::size_t a = std::size_t(int(base) & 63);
            return boat.radii[a] + ((boat.radii[(a + 1u) & 63u] - boat.radii[a]) * (at - base));
        };
        auto pointAt = [&](float p_angle)
        { return boat.centre + (Vector2f(std::cos(p_angle), std::sin(p_angle)) * radius(p_angle)); };
        boat.angle = std::fmod(boat.angle + ((boat.speed * p_dt) / radius(boat.angle)) + TURN, TURN);
        const Vector2f here = pointAt(boat.angle);
        const Vector2f ahead = pointAt(boat.angle + ((boat.speed > 0.0f) ? 0.01f : -0.01f)) - here;
        const Vector3f at(here.x, 0.0f, here.y);
        // The bow (-Z) toward where it goes; it rolls on the swell.
        const float roll = 0.06f * std::sin((1.3f * m_time) + (40.0f * boat.angle));
        boat.root.position(at)
            .rotation(heading(std::atan2(-ahead.x, -ahead.y)) * about(roll, Vector3f(0.0f, 0.0f, 1.0f)))
            .scale(seen(at, BOAT_SEEN));
    }

    for (Bird& bird : m_birds)
    {
        bird.angle = std::fmod(bird.angle + ((bird.speed * p_dt) / bird.radius), TURN);
        const Vector3f at = bird.centre + Vector3f(bird.radius * std::cos(bird.angle),
                                                   8.0f * std::sin((0.7f * m_time) + bird.phase),
                                                   bird.radius * std::sin(bird.angle));
        // Along the circle (it goes toward (-sin, cos) of its angle),
        // leaning into the turn.
        bird.root.position(at)
            .rotation(heading(3.14159265f - bird.angle) * about(0.35f, Vector3f(0.0f, 0.0f, 1.0f)))
            .scale(seen(at, BIRD_SEEN));
        // Flaps for a while, then glides with the wings held a little up.
        const float beat = std::sin((7.0f * m_time) + bird.phase);
        const float flapping = std::clamp(2.0f * std::sin((0.4f * m_time) + bird.phase), 0.0f, 1.0f);
        const float lift = 0.1f + (0.55f * flapping * beat);
        bird.left.rotation(-lift, Vector3f(0.0f, 0.0f, 1.0f));
        bird.right.rotation(lift, Vector3f(0.0f, 0.0f, 1.0f));
    }

    for (Fish& fish : m_fish)
    {
        fish.angle = std::fmod(fish.angle + ((fish.speed * p_dt) / fish.radius), TURN);
        const Vector2f here = fish.centre + (Vector2f(std::cos(fish.angle), std::sin(fish.angle)) * fish.radius);
        const Vector3f at(here.x, 0.05f, here.y);
        const float size = seen(at, FISH_SEEN);
        const Quatf way = heading(3.14159265f - fish.angle);
        fish.shadow.position(at).rotation(way).scale(0.35f * size, 0.1f * size, size);
        // A jump: a parabola a few times its length high, nose up then down.
        fish.wait -= p_dt;
        if ((fish.wait <= 0.0f) && (fish.jump < 0.0f))
        {
            fish.jump = 0.0f;
        }
        if (fish.jump >= 0.0f)
        {
            fish.jump += p_dt / 0.9f;
            const float t = std::min(fish.jump, 1.0f);
            fish.jumper.position(here.x, 4.0f * size * t * (1.0f - t), here.y)
                .rotation(way * about(1.2f * (0.5f - t), Vector3f(1.0f, 0.0f, 0.0f)))
                .scale(0.3f * size, 0.25f * size, size);
            if (fish.jump >= 1.0f)
            {
                fish.jump = -1.0f;
                fish.wait = 6.0f + std::fmod(fish.angle * 97.0f, 20.0f);
            }
        }
        else
        {
            fish.jumper.position(here.x, -50.0f, here.y);   // under the sea
        }
    }
}

// ----------------------------------------------------------------------------
Place Archipelago::placeUnder(Vector2f p_mouse) const
{
    compages::renderer::CameraFrame const& camera = m_scene.lastCamera();
    if ((camera.viewport_width == 0u) || (camera.viewport_height == 0u))
    {
        return {};
    }
    const compages::Ray ray = camera.screenRay(p_mouse.x, p_mouse.y, camera.viewport_width, camera.viewport_height);
    if (ray.direction.y >= -1.0e-4f)
    {
        return {};   // toward the sky
    }
    // March from where the ray goes below the highest peak down to the sea,
    // in steps of half a coarse quad, then halve the last step.
    const float quad = MAP / float(BASE_CELLS);
    const float begin = std::max(0.0f, ((1.2f * archipelago::PEAK) - ray.origin.y) / ray.direction.y);
    const float end = -ray.origin.y / ray.direction.y;
    const float step = 0.5f * quad / std::max(std::abs(ray.direction.y), 0.05f);
    const int steps = std::min(8000, int((end - begin) / step) + 1);
    auto below = [&](float p_t)
    {
        const Vector3f p = ray.pointAt(p_t);
        return p.y <= std::max(0.0f, baseHeight(p.x, p.z));
    };
    float hit = end;
    for (int k = 0; k <= steps; ++k)
    {
        const float t = std::min(begin + (float(k) * step), end);
        if (below(t))
        {
            float low = std::max(begin, t - step);
            float high = t;
            for (int halve = 0; halve < 8; ++halve)
            {
                const float middle = 0.5f * (low + high);
                (below(middle) ? high : low) = middle;
            }
            hit = high;
            break;
        }
    }
    const Vector3f point = ray.pointAt(hit);
    if ((std::abs(point.x) >= 0.5f * MAP) || (std::abs(point.z) >= 0.5f * MAP) ||
        (baseHeight(point.x, point.z) <= 0.0f))
    {
        return {};
    }
    return m_land.placeOf(point.x, point.z);
}

// ----------------------------------------------------------------------------
void Archipelago::flyTo(int p_region, int p_part)
{
    compages::world::RigController& rig = m_camera.get<compages::world::Rig>().controller;
    m_chosen = Place{ .region = p_region, .part = p_part };
    if (p_region < 0)
    {
        rig.flyTo(Vector3f::ZERO, OVERVIEW_DISTANCE);
        return;
    }
    archipelago::Region const& region = m_land.regions[std::size_t(p_region)];
    const Vector2f site = (p_part >= 0) ? region.parts[std::size_t(p_part)].site : region.site;
    rig.flyTo(Vector3f(site.x, 0.0f, site.y), (p_part >= 0) ? VISIT_DISTANCE : 1.5f * VISIT_DISTANCE);
}

// ----------------------------------------------------------------------------
void Archipelago::updateText()
{
    const Vector3f eye = m_camera.position();
    // Set a component when shown, remove it when not.
    auto show = [](compages::world::Entity p_entity, bool p_shown, auto const& p_component)
    {
        using Component = std::decay_t<decltype(p_component)>;
        if (p_shown)
        {
            p_entity.set(p_component);
        }
        else if (p_entity.has<Component>())
        {
            p_entity.remove<Component>();
        }
    };
    for (std::size_t r = 0u; r < m_region_names.size(); ++r)
    {
        Topic const& topic = TOPICS[r];
        compages::world::Entity marker = m_region_names[r];
        const bool hovered = (m_hovered.region == int(r));
        const bool tooltip = hovered || (m_chosen.region == int(r));
        const float distance = compages::vector::norm(marker.position() - eye);
        // The links, the one of the part under the mouse (or chosen) marked.
        std::string links;
        for (std::size_t k = 0u; k < topic.links.size(); ++k)
        {
            const bool marked = hovered ? (m_hovered.part == int(k)) : (m_chosen.part == int(k));
            links += std::string(links.empty() ? "" : "\n") + (marked ? "» " : "   ") + topic.links[k];
        }
        show(marker, tooltip,
             compages::renderer::Tooltip{ .title = topic.name,
                                          .text = links,
                                          .color = { 0.90f, 0.94f, 0.98f },
                                          .accent = { 1.0f, 0.82f, 0.48f },
                                          .background = { 0.05f, 0.10f, 0.17f },
                                          .opacity = 0.86f,
                                          .pixels = 14.0f });
        show(marker, !tooltip && m_names && (distance < REGION_NAMES_WITHIN),
             compages::renderer::Label{ .text = topic.name,
                                        .color = { 1.0f, 1.0f, 1.0f },
                                        .pixels = 15.0f,
                                        .hide_closer_than = 0.0f });
        for (std::size_t k = 0u; k < m_part_names[r].size(); ++k)
        {
            compages::world::Entity part = m_part_names[r][k];
            const bool near = compages::vector::norm(part.position() - eye) < PART_NAMES_WITHIN;
            // A part named as its region (its first link, often) is
            // already named.
            show(part, m_names && near && (pageName(topic.links[k]) != topic.name),
                 compages::renderer::Label{ .text = pageName(topic.links[k]),
                                            .color = { 0.88f, 0.92f, 0.95f },
                                            .pixels = 12.0f,
                                            .hide_closer_than = 0.0f });
        }
    }
}

// ----------------------------------------------------------------------------
void Archipelago::draw(compages::world::ViewFrame const& p_frame)
{
    m_time += p_frame.elapsed;
    if (compages::renderer::Material* sea = m_scene.assets().material(m_sea))
    {
        sea->program.set("time", m_time);
    }

    // The land made in the thread, shown when it is ready.
    if (m_loading.valid() && (m_loading.wait_for(std::chrono::seconds(0)) == std::future_status::ready))
    {
        const compages::Status shown = finishLoading(m_loading.get());
        if (!shown)
        {
            compages::gpu::reportError(shown.error());
        }
    }
    if (!m_ready)
    {
        m_camera.get<compages::renderer::Caption>().text = "";
        m_scene.draw(p_frame);
        return;
    }
    updateDetails();

    // A click is a press and a release at about the same place; a drag
    // moves the map.
    compages::world::Input const& input = p_frame.input;
    if (input.mouse_left_pressed)
    {
        m_press = input.mouse;
    }
    if (m_was_down && !input.mouse_left && input.mouse_over &&
        (compages::vector::norm(input.mouse - m_press) < 4.0f))
    {
        const Place place = placeUnder(input.mouse);
        if (place.region >= 0)
        {
            flyTo(place.region, place.part);
        }
        else
        {
            m_chosen = {};
        }
    }
    m_was_down = input.mouse_left;
    m_hovered = (input.mouse_over && !input.mouse_left) ? placeUnder(input.mouse) : Place{};
    updateText();
    if (m_animals)
    {
        animate(p_frame.elapsed * m_time_scale);
    }

    // Outlines: the chosen region, the region under the mouse and its part.
    if (m_borders)
    {
        auto lines = [&](std::vector<Vector3f> const& p_segments, Vector3f const& p_colour)
        {
            for (std::size_t k = 0u; (k + 1u) < p_segments.size(); k += 2u)
            {
                m_scene.debug().line(p_segments[k], p_segments[k + 1u], p_colour);
            }
        };
        if (m_chosen.region >= 0)
        {
            lines(m_land.regions[std::size_t(m_chosen.region)].border, { 1.0f, 0.82f, 0.48f });
        }
        if (m_hovered.region >= 0)
        {
            archipelago::Region const& region = m_land.regions[std::size_t(m_hovered.region)];
            lines(region.border, { 0.9f, 0.95f, 1.0f });
            if (m_hovered.part >= 0)
            {
                lines(region.parts[std::size_t(m_hovered.part)].border, { 1.0f, 1.0f, 1.0f });
            }
        }
    }

    // What is under the mouse, where the camera is.
    compages::world::RigController const& rig = m_camera.get<compages::world::Rig>().controller;
    const Place shown = (m_hovered.region >= 0) ? m_hovered : m_chosen;
    std::string where = "Archipel";
    if (shown.region >= 0)
    {
        Topic const& topic = TOPICS[std::size_t(shown.region)];
        where = std::string(THEMES[topic.theme].name) + " · " + topic.name;
        if ((shown.part >= 0) && (pageName(topic.links[std::size_t(shown.part)]) != topic.name))
        {
            where += " · " + pageName(topic.links[std::size_t(shown.part)]);
        }
    }
    char camera[64];
    std::snprintf(camera, sizeof(camera), "    camera %.1f km%s", double(rig.currentDistance()) / 1000.0,
                  rig.flying() ? "    flying" : "");
    m_camera.get<compages::renderer::Caption>().text = where + camera;
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Archipelago::controls()
{
    ImGui::InputInt("Seed", &m_seed);
    ImGui::SliderInt("Erosion steps", &m_erosion_steps, 0, 300);
    ImGui::SetItemTooltip("Steps of 40 000 years: the rivers cut deeper valleys.");
    ImGui::BeginDisabled(m_loading.valid());
    if (ImGui::Button("Generate"))
    {
        startLoading();
    }
    ImGui::EndDisabled();
    if (!m_ready)
    {
        return;
    }

    // Every region, by island, to fly to.
    std::vector<std::string> names = { "The whole archipelago" };
    for (Topic const& topic : TOPICS)
    {
        names.push_back(std::string(THEMES[topic.theme].name) + " / " + topic.name);
    }
    const int chosen = m_chosen.region + 1;
    if (ImGui::BeginCombo("Fly to", names[std::size_t(chosen)].c_str()))
    {
        for (std::size_t k = 0u; k < names.size(); ++k)
        {
            if (ImGui::Selectable(names[k].c_str(), int(k) == chosen))
            {
                flyTo(int(k) - 1, -1);
            }
        }
        ImGui::EndCombo();
    }
    compages::world::RigController& rig = m_camera.get<compages::world::Rig>().controller;
    ImGui::SliderFloat("Flight pace (s)", &rig.flight_pace, 0.2f, 2.0f, "%.2f");
    ImGui::Checkbox("Borders", &m_borders);
    ImGui::SameLine();
    ImGui::Checkbox("Names", &m_names);
    ImGui::SameLine();
    ImGui::Checkbox("Animals", &m_animals);
    ImGui::SameLine();
    ImGui::Checkbox("Fine tiles", &m_details_on);
    ImGui::SliderFloat("Animals' time", &m_time_scale, 1.0f, 30.0f, "x %.0f");
    const std::size_t fine = std::size_t(
        std::count_if(m_details.begin(), m_details.end(), [](Detail const& p_d) { return p_d.ready && p_d.entity.enabled(); }));
    ImGui::Text("Fine tiles shown: %zu (8 m, 2 m a pixel)", fine);
    ImGui::TextWrapped("Left drag: move the map. Right drag: turn. Wheel: zoom toward "
                       "the mouse. Click a part of a region: fly there. W A S D, Q E, "
                       "R F also move, turn, zoom.");
}

} // namespace examples
