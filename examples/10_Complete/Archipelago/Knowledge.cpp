// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Knowledge.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>

namespace examples::archipelago
{

using compages::Vector2f;

//! \brief A colour of the wheel, far from the previous ones (golden angle),
//! light and not too strong.
static std::array<float, 3> hueOf(std::size_t p_rank)
{
    const float h =
        std::fmod((0.61803399f * float(p_rank)) + 0.58f, 1.0f) * 6.0f;
    const float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);
    // The six sectors of the wheel: which channel is full, which rises.
    const std::array<std::array<float, 3>, 6> sectors = {
        { { 1.0f, x, 0.0f },
          { x, 1.0f, 0.0f },
          { 0.0f, 1.0f, x },
          { 0.0f, x, 1.0f },
          { x, 0.0f, 1.0f },
          { 1.0f, 0.0f, x } }
    };
    std::array<float, 3> rgb = sectors[std::size_t(std::clamp(int(h), 0, 5))];
    for (float& c : rgb)
    {
        c = 0.25f + (0.7f * c);
    }
    return rgb;
}

//! \brief The name of a Wikipedia page from its link: what follows the
//! last '/', with spaces for the underscores.
static std::string pageName(std::string const& p_link)
{
    std::string name = p_link.substr(p_link.rfind('/') + 1u);
    std::ranges::replace(name, '_', ' ');
    return name;
}

//! \brief A node of the layout.
struct Node
{
    int parent;
    bool bookmark;
    std::string title;
    std::string url;
    Vector2f at;
};

// ----------------------------------------------------------------------------
//! \brief The nodes of a layout file, or why it is not one.
// ----------------------------------------------------------------------------
static compages::Result<std::vector<Node>>
readNodes(std::filesystem::path const& p_path)
{
    std::ifstream file(p_path);
    if (!file)
    {
        return compages::failure("cannot read " + p_path.string());
    }
    const nlohmann::json json = nlohmann::json::parse(file, nullptr, false);
    if (json.is_discarded() ||
        (json.value("format", "") != "islanded-browser-layout"))
    {
        return compages::failure(
            p_path.string() + " is not a layout of IslandedBrowser (--layout)");
    }
    std::vector<Node> nodes;
    for (nlohmann::json const& n : json.at("nodes"))
    {
        nodes.push_back(
            Node{ n.value("parent", -1),
                  n.value("kind", "") == "bookmark",
                  n.value("title", ""),
                  n.value("url", ""),
                  Vector2f(n.value("x", 0.0f), n.value("y", 0.0f)) });
    }
    return nodes;
}

// ****************************************************************************
//! \brief Groups the bookmarks of the nodes into themes and topics.
// ****************************************************************************
class Gathering
{
public:

    explicit Gathering(std::vector<Node> const& p_nodes)
        : m_nodes(p_nodes),
          m_theme_of(p_nodes.size(), -1),
          m_topic_of(p_nodes.size(), -1)
    {
    }

    //! \brief Add a bookmark to the topic of its region, made if new.
    void add(Knowledge& p_knowledge, int p_bookmark)
    {
        int island = -1;
        int region = -1;
        chain(p_bookmark, island, region);
        if ((island == p_bookmark) || m_nodes[std::size_t(island)].bookmark)
        {
            return;
        }
        const int theme = themeOf(p_knowledge, island);
        // A bookmark right under its island: a region of the island itself.
        const int topic = topicOf(
            p_knowledge, (region == p_bookmark) ? island : region, theme);
        Node const& node = m_nodes[std::size_t(p_bookmark)];
        p_knowledge.topics[std::size_t(topic)].pages.push_back(Page{
            node.title.empty() ? node.url : node.title, node.url, node.at });
    }

private:

    //! \brief The island of a node: its last ancestor; its region: the
    //! ancestor just under the island (-1 when the node is an island).
    void chain(int p_node, int& p_island, int& p_region) const
    {
        const auto count = int(m_nodes.size());
        p_region = -1;
        p_island = p_node;
        for (int depth = 0; depth < count; ++depth)
        {
            const int parent = m_nodes[std::size_t(p_island)].parent;
            if ((parent < 0) || (parent >= count))
            {
                return;
            }
            p_region = p_island;
            p_island = parent;
        }
    }

    int themeOf(Knowledge& p_knowledge, int p_island)
    {
        int& theme = m_theme_of[std::size_t(p_island)];
        if (theme < 0)
        {
            theme = int(p_knowledge.themes.size());
            Node const& node = m_nodes[std::size_t(p_island)];
            p_knowledge.themes.push_back(
                Theme{ node.title, hueOf(p_knowledge.themes.size()), node.at });
        }
        return theme;
    }

    int topicOf(Knowledge& p_knowledge, int p_owner, int p_theme)
    {
        int& topic = m_topic_of[std::size_t(p_owner)];
        if (topic < 0)
        {
            topic = int(p_knowledge.topics.size());
            Node const& node = m_nodes[std::size_t(p_owner)];
            p_knowledge.topics.push_back(
                Topic{ node.title, std::size_t(p_theme), node.at, {} });
        }
        return topic;
    }

    std::vector<Node> const& m_nodes;
    std::vector<int> m_theme_of;
    std::vector<int> m_topic_of;
};

// ----------------------------------------------------------------------------
compages::Result<Knowledge> loadKnowledge(std::filesystem::path const& p_path)
{
    compages::Result<std::vector<Node>> nodes = readNodes(p_path);
    if (!nodes)
    {
        return compages::failure(nodes.error());
    }
    Knowledge knowledge;
    knowledge.source = p_path.string();
    Gathering gathering(nodes.value());
    for (std::size_t k = 0u; k < nodes.value().size(); ++k)
    {
        if (nodes.value()[k].bookmark)
        {
            gathering.add(knowledge, int(k));
        }
    }
    if (knowledge.themes.empty())
    {
        return compages::failure(p_path.string() + " has no bookmark");
    }
    if (knowledge.themes.size() > 120u)
    {
        return compages::failure(p_path.string() +
                                 " has more than 120 islands");
    }
    return knowledge;
}

// ----------------------------------------------------------------------------
Knowledge builtinKnowledge()
{
    struct Island
    {
        char const* name;
        float x;
        float z;
        float radius;
        std::array<float, 3> hue;
    };
    static constexpr std::array ISLANDS = {
        Island{ "Sciences", -3.9f, -3.2f, 3.0f, { 0.35f, 0.45f, 0.95f } },
        Island{ "Arts", 3.9f, -3.9f, 2.5f, { 0.95f, 0.35f, 0.55f } },
        Island{ "Histoire", 4.1f, 3.0f, 2.7f, { 0.95f, 0.62f, 0.25f } },
        Island{ "Techniques", -3.7f, 3.7f, 2.6f, { 0.20f, 0.75f, 0.75f } },
        Island{ "Société", 0.2f, 0.0f, 1.7f, { 0.50f, 0.85f, 0.30f } },
    };
    struct Region
    {
        std::size_t island;
        char const* name;
        std::array<char const*, 3> links;
    };
    static constexpr std::array REGIONS = {
        Region{ 0,
                "Astronomie",
                { "fr.wikipedia.org/wiki/Astronomie",
                  "fr.wikipedia.org/wiki/Système_solaire",
                  "fr.wikipedia.org/wiki/Galaxie" } },
        Region{ 0,
                "Physique",
                { "fr.wikipedia.org/wiki/Physique",
                  "fr.wikipedia.org/wiki/Mécanique_quantique",
                  "fr.wikipedia.org/wiki/Relativité_générale" } },
        Region{ 0,
                "Chimie",
                { "fr.wikipedia.org/wiki/Chimie",
                  "fr.wikipedia.org/wiki/Tableau_périodique_des_éléments",
                  "fr.wikipedia.org/wiki/Molécule" } },
        Region{ 0,
                "Biologie",
                { "fr.wikipedia.org/wiki/Biologie",
                  "fr.wikipedia.org/wiki/Cellule_(biologie)",
                  "fr.wikipedia.org/wiki/Évolution_(biologie)" } },
        Region{ 0,
                "Mathématiques",
                { "fr.wikipedia.org/wiki/Mathématiques",
                  "fr.wikipedia.org/wiki/Géométrie",
                  "fr.wikipedia.org/wiki/Théorie_des_nombres" } },
        Region{ 0,
                "Géologie",
                { "fr.wikipedia.org/wiki/Géologie",
                  "fr.wikipedia.org/wiki/Tectonique_des_plaques",
                  "fr.wikipedia.org/wiki/Volcan" } },
        Region{ 1,
                "Peinture",
                { "fr.wikipedia.org/wiki/Peinture",
                  "fr.wikipedia.org/wiki/Impressionnisme",
                  "fr.wikipedia.org/wiki/Claude_Monet" } },
        Region{ 1,
                "Musique",
                { "fr.wikipedia.org/wiki/Musique",
                  "fr.wikipedia.org/wiki/Musique_classique",
                  "fr.wikipedia.org/wiki/Jazz" } },
        Region{ 1,
                "Cinéma",
                { "fr.wikipedia.org/wiki/Cinéma",
                  "fr.wikipedia.org/wiki/Frères_Lumière",
                  "fr.wikipedia.org/wiki/Nouvelle_Vague" } },
        Region{ 1,
                "Sculpture",
                { "fr.wikipedia.org/wiki/Sculpture",
                  "fr.wikipedia.org/wiki/Auguste_Rodin",
                  "fr.wikipedia.org/wiki/Camille_Claudel" } },
        Region{ 1,
                "Architecture",
                { "fr.wikipedia.org/wiki/Architecture",
                  "fr.wikipedia.org/wiki/Architecture_gothique",
                  "fr.wikipedia.org/wiki/Le_Corbusier" } },
        Region{ 2,
                "Antiquité",
                { "fr.wikipedia.org/wiki/Antiquité",
                  "fr.wikipedia.org/wiki/Rome_antique",
                  "fr.wikipedia.org/wiki/Grèce_antique" } },
        Region{ 2,
                "Moyen Âge",
                { "fr.wikipedia.org/wiki/Moyen_Âge",
                  "fr.wikipedia.org/wiki/Charlemagne",
                  "fr.wikipedia.org/wiki/Croisades" } },
        Region{ 2,
                "Renaissance",
                { "fr.wikipedia.org/wiki/Renaissance",
                  "fr.wikipedia.org/wiki/Léonard_de_Vinci",
                  "fr.wikipedia.org/wiki/Imprimerie" } },
        Region{ 2,
                "Révolution",
                { "fr.wikipedia.org/wiki/Révolution_française",
                  "fr.wikipedia.org/wiki/Prise_de_la_Bastille",
                  "fr.wikipedia.org/wiki/Maximilien_de_Robespierre" } },
        Region{ 2,
                "Époque contemporaine",
                { "fr.wikipedia.org/wiki/Époque_contemporaine",
                  "fr.wikipedia.org/wiki/Révolution_industrielle",
                  "fr.wikipedia.org/wiki/Seconde_Guerre_mondiale" } },
        Region{ 3,
                "Informatique",
                { "fr.wikipedia.org/wiki/Informatique",
                  "fr.wikipedia.org/wiki/Algorithme",
                  "fr.wikipedia.org/wiki/Alan_Turing" } },
        Region{ 3,
                "Robotique",
                { "fr.wikipedia.org/wiki/Robotique",
                  "fr.wikipedia.org/wiki/Robot_industriel",
                  "fr.wikipedia.org/wiki/Cinématique_inverse" } },
        Region{ 3,
                "Électronique",
                { "fr.wikipedia.org/wiki/Électronique",
                  "fr.wikipedia.org/wiki/Transistor",
                  "fr.wikipedia.org/wiki/Circuit_intégré" } },
        Region{ 3,
                "Aéronautique",
                { "fr.wikipedia.org/wiki/Aéronautique",
                  "fr.wikipedia.org/wiki/Frères_Wright",
                  "fr.wikipedia.org/wiki/Concorde" } },
        Region{ 3,
                "Énergie",
                { "fr.wikipedia.org/wiki/Énergie",
                  "fr.wikipedia.org/wiki/Énergie_solaire",
                  "fr.wikipedia.org/wiki/Énergie_nucléaire" } },
        Region{ 4,
                "Philosophie",
                { "fr.wikipedia.org/wiki/Philosophie",
                  "fr.wikipedia.org/wiki/René_Descartes",
                  "fr.wikipedia.org/wiki/Siècle_des_Lumières" } },
        Region{ 4,
                "Économie",
                { "fr.wikipedia.org/wiki/Économie",
                  "fr.wikipedia.org/wiki/Adam_Smith",
                  "fr.wikipedia.org/wiki/Monnaie" } },
        Region{ 4,
                "Géographie",
                { "fr.wikipedia.org/wiki/Géographie",
                  "fr.wikipedia.org/wiki/Cartographie",
                  "fr.wikipedia.org/wiki/Archipel" } },
        Region{ 4,
                "Langues",
                { "fr.wikipedia.org/wiki/Langue",
                  "fr.wikipedia.org/wiki/Linguistique",
                  "fr.wikipedia.org/wiki/Langue_française" } },
    };

    // The layout by hand: the regions of an island on a circle around it,
    // the links of a region on a small circle around it.
    Knowledge knowledge;
    knowledge.source = "built in";
    for (Island const& island : ISLANDS)
    {
        knowledge.themes.push_back(
            Theme{ island.name, island.hue, Vector2f(island.x, island.z) });
    }
    std::array<std::size_t, ISLANDS.size()> ranks{};
    for (std::size_t r = 0u; r < REGIONS.size(); ++r)
    {
        Region const& region = REGIONS[r];
        Island const& island = ISLANDS[region.island];
        // The rank of the region in its island: the golden angle apart.
        const float angle = 2.39996323f * float(ranks[region.island]);
        ++ranks[region.island];
        const Vector2f node = Vector2f(island.x, island.z) +
                              (Vector2f(std::cos(angle), std::sin(angle)) *
                               (0.5f * island.radius));
        Topic topic{ region.name, region.island, node, {} };
        for (std::size_t k = 0u; k < region.links.size(); ++k)
        {
            const float a = angle + (2.0944f * float(k));
            topic.pages.push_back(
                Page{ pageName(region.links[k]),
                      std::string("https://") + region.links[k],
                      node + (Vector2f(std::cos(a), std::sin(a)) *
                              (0.15f * island.radius)) });
        }
        knowledge.topics.push_back(std::move(topic));
    }
    return knowledge;
}

} // namespace examples::archipelago
