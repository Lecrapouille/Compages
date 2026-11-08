// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Components/Text.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief The letters of a font, baked once into one picture of distance
//! fields: each pixel says how far it is from the edge of the letter (128 on
//! the edge, more inside). Read with a linear filter, the edge stays sharp
//! at any size, and a halo is one more threshold.
// ****************************************************************************
struct FontAtlas
{
    //! \brief Height of the letters in the atlas, in pixels: what a label of
    //! \c Label::pixels is scaled from.
    static constexpr float BAKE_PIXELS = 48.0f;
    //! \brief Pixels of distance field around each letter: as wide as the
    //! widest halo.
    static constexpr int PADDING = 6;
    //! \brief Steps of the field per pixel of distance (128 / PADDING).
    static constexpr float STEPS_PER_PIXEL = 128.0f / float(PADDING);
    //! \brief The first and the last letter baked (Latin-1). The slot of
    //! \c DOT holds a disc, which marks the point of a label.
    static constexpr char32_t FIRST = 32u;
    static constexpr char32_t LAST = 255u;
    static constexpr char32_t DOT = 127u;

    //! \brief One letter: where it is in the atlas and how it is placed.
    struct Glyph
    {
        //! \brief Its rectangle in the atlas, in pixels, from the first row.
        std::uint32_t x = 0u;
        std::uint32_t y = 0u;
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
        //! \brief From the pen on the baseline to the top left corner, in
        //! baked pixels, y going down (as stb_truetype says it).
        float left = 0.0f;
        float top = 0.0f;
        //! \brief How far the pen moves after it, in baked pixels.
        float advance = 0.0f;
    };

    //! \brief Top of the tallest letters and bottom of the lowest, from the
    //! baseline, in baked pixels (descent is negative).
    float ascent = 0.0f;
    float descent = 0.0f;
    //! \brief The letters from FIRST to LAST; a letter the font lacks has a
    //! size of zero and only advances.
    std::array<Glyph, LAST - FIRST + 1u> glyphs{};
    //! \brief Size of the atlas, and its pixels, one byte each, first row
    //! first (row 0 is v = 0).
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::vector<std::byte> pixels;

    //! \brief The letter for a code point, '?' when it was not baked.
    //! \param[in] p_code A Unicode code point.
    //! \return Its glyph.
    [[nodiscard]] Glyph const& glyph(char32_t p_code) const;
};

// ----------------------------------------------------------------------------
//! \brief Bake the letters of a TrueType or OpenType font file.
//! \param[in] p_file The font file (.ttf, .otf).
//! \return The atlas, or why not: a missing file, a file that is not a font.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<FontAtlas> bakeFont(std::filesystem::path const& p_file);

// ----------------------------------------------------------------------------
//! \brief Read UTF-8 text as code points.
//! \param[in] p_text The text; a broken sequence becomes U+FFFD.
//! \return One code point per letter.
// ----------------------------------------------------------------------------
[[nodiscard]] std::vector<char32_t> decodeUtf8(std::string_view p_text);

// ****************************************************************************
//! \brief Draws the Caption and Label components: projects the points of the
//! labels on the CPU, lays out their letters in pixels, skips the labels
//! that would cover text already laid out, and draws all the text in one
//! call over the picture. What Scene::render() calls after the debug lines.
// ****************************************************************************
class TextDraw
{
public:

    //! \brief Bake a font and send it to the GPU.
    //! \param[in] p_file The font file (.ttf, .otf).
    //! \return Why it failed, if it did.
    [[nodiscard]] Status loadFont(std::filesystem::path const& p_file);

    //! \brief Lay out a label, if it is in front of the camera and not too
    //! close.
    //! \param[in] p_label What to write.
    //! \param[in] p_point Where its entity is, in the World.
    //! \param[in] p_camera The camera of the picture being drawn.
    void add(Label const& p_label,
             compages::Vector3f const& p_point,
             CameraFrame const& p_camera);

    //! \brief Lay out a caption on the picture. Captions are laid out before
    //! the labels, which are then not written over them.
    //! \param[in] p_caption What to write, and where.
    //! \param[in] p_camera The camera of the picture being drawn.
    void add(Caption const& p_caption, CameraFrame const& p_camera);

    //! \brief Draw the labels laid out since the last call, then forget them.
    //! \param[in] p_camera The camera of the picture (its viewport).
    //! \return An error when there are labels but no font.
    [[nodiscard]] Status flush(CameraFrame const& p_camera);

private:

    //! \brief A corner of a letter, in pixels of the picture.
    struct LabelVertex
    {
        compages::Vector2f position;
        compages::Vector2f uv;
        compages::Vector3f color;
        //! \brief The field value where the halo ends: lower for a smaller
        //! label, so that the halo is about as wide in pixels.
        float halo;
    };

    //! \brief The rectangle a label takes on the picture, in pixels.
    struct Room
    {
        float left;
        float bottom;
        float right;
        float top;
    };

    //! \brief The field value where the halo ends, for letters drawn at
    //! \c p_scale times their baked size: about 1.5 pixels wide.
    [[nodiscard]] static float haloFor(float p_scale);

    //! \brief How wide a line is on the picture, in pixels.
    [[nodiscard]] float width(std::vector<char32_t> const& p_codes, float p_scale) const;

    //! \brief Take a room on the picture, if no text is there yet.
    //! \return false when it covers text already laid out.
    [[nodiscard]] bool reserve(Room const& p_room);

    //! \brief Lay out one line of letters from a pen on its baseline.
    void line(std::vector<char32_t> const& p_codes,
              float p_pen,
              float p_baseline,
              float p_scale,
              compages::Vector3f const& p_color);

    //! \brief Two triangles for one letter.
    void quad(FontAtlas::Glyph const& p_glyph,
              float p_left,
              float p_top,
              float p_scale,
              compages::Vector3f const& p_color,
              float p_halo);

    FontAtlas m_atlas;
    compages::gpu::Texture m_texture;
    compages::gpu::Program m_program;
    compages::gpu::Pipeline m_pipeline;
    compages::gpu::VertexBuffer<LabelVertex> m_vertices;
    std::vector<LabelVertex> m_pending;
    //! \brief The rooms of the labels written this picture.
    std::vector<Room> m_taken;
    //! \brief A label was asked for before any font was loaded.
    bool m_missing_font = false;
};

} // namespace compages::renderer
