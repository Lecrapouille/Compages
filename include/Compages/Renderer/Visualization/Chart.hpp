// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Procedural/CanvasTexture.hpp"

#include <deque>
#include <string>
#include <vector>

namespace compages::renderer
{

class Scene;

// ****************************************************************************
//! \brief The size and the axes of a Chart.
// ****************************************************************************
struct ChartDesc
{
    //! \brief Its picture, in pixels.
    std::uint32_t width;
    std::uint32_t height;
    //! \brief The time shown: the last \c seconds, the newest on the right.
    float seconds;
    //! \brief The values at the bottom and at the top.
    float low;
    float high;
};

// ****************************************************************************
//! \brief Curves of values over time, drawn into a picture: a temperature
//! followed at a point, an energy, a speed. Put the picture on any mesh (a
//! board in the scene) with applyLook().
//!
//! \code
//! COMPAGES_TRY(m_chart.create(m_scene, { .width = 512, .height = 256,
//!                                        .seconds = 30.0f, .low = 0.0f, .high = 300.0f },
//!                             "Thermometers"));
//! const std::size_t centre = m_chart.series(CanvasTexture::Color::rgb(255, 120, 0));
//! m_chart.applyLook(m_scene, board.id());
//! // each frame:
//! m_chart.add(centre, time, temperature);
//! COMPAGES_TRY(m_chart.draw());
//! \endcode
// ****************************************************************************
class Chart
{
public:

    // ------------------------------------------------------------------------
    //! \brief Make the picture and put it in the catalog of the scene.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status create(Scene& p_scene, ChartDesc const& p_desc, std::string p_name);

    // ------------------------------------------------------------------------
    //! \brief A new curve, of this colour.
    //! \return Its number, for add().
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t series(CanvasTexture::Color p_color);

    // ------------------------------------------------------------------------
    //! \brief A value of a curve at a time (times go up). Values older than
    //! the time shown are dropped.
    // ------------------------------------------------------------------------
    void add(std::size_t p_series, float p_time, float p_value);

    // ------------------------------------------------------------------------
    //! \brief Change the values at the bottom and at the top.
    // ------------------------------------------------------------------------
    void range(float p_low, float p_high);

    // ------------------------------------------------------------------------
    //! \brief Draw the background, the grid and the curves, and send the
    //! picture to the GPU.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status draw();

    // ------------------------------------------------------------------------
    //! \brief Show the picture on an entity drawn by the scene.
    // ------------------------------------------------------------------------
    void applyLook(Scene& p_scene, compages::world::EntityId p_entity);

    //! \brief The picture in the catalog.
    [[nodiscard]] TextureAssetId texture() const
    {
        return m_canvas.texture();
    }

private:

    struct Curve
    {
        CanvasTexture::Color color;
        std::deque<compages::Vector2f> points;
    };

    CanvasTexture m_canvas;
    std::vector<Curve> m_curves;
    ChartDesc m_desc{ .width = 0u, .height = 0u, .seconds = 1.0f, .low = 0.0f, .high = 1.0f };
    float m_now = 0.0f;
};

} // namespace compages::renderer
