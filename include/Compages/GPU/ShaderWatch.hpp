// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/ShaderSource.hpp"

#include <filesystem>
#include <functional>
#include <vector>

namespace compages::gpu
{

// ****************************************************************************
//! \brief Hot reload: watch the files of shaders (the main files and every
//! file they include) and load them again when one is saved.
//!
//! What "load again" means is yours: a Program alone, a Program and the
//! Pipelines made from it (a new program has a new handle), a Material made
//! again. A load that fails keeps what was there (Program::load() does), so
//! a typo while editing does not break the picture: poll() returns the log.
//!
//! \code
//! auto vertex = compages::gpu::ShaderSource::fromFile("shaders/water.vert");
//! auto fragment = compages::gpu::ShaderSource::fromFile("shaders/water.frag");
//! auto load = [this](ShaderSource const& p_vertex, ShaderSource const& p_fragment) -> Status
//! {
//!     COMPAGES_TRY(m_program.load(p_vertex, p_fragment));
//!     COMPAGES_TRY_ASSIGN(m_pipeline, Pipeline::create<Vertex>(m_program, m_state));
//!     return success();
//! };
//! COMPAGES_TRY(m_watch.watch(vertex, fragment, load));   // loads once now
//! // each frame:
//! if (Status reloaded = m_watch.poll(); !reloaded) { std::cerr << reloaded.error(); }
//! \endcode
// ****************************************************************************
class ShaderWatch
{
public:

    //! \brief Loads the two stages: called by watch(), then by poll() when a
    //! file changed.
    using Load = std::function<Status(ShaderSource const&, ShaderSource const&)>;

    // ------------------------------------------------------------------------
    //! \brief Load the stages now, and remember their files.
    //! \return Why they could not be resolved or loaded.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status watch(ShaderSource p_vertex, ShaderSource p_fragment, Load p_load);

    // ------------------------------------------------------------------------
    //! \brief Load again what changed on the disk since the last time.
    //! \return The first error of a load (what was there is kept); success
    //! when nothing changed or everything loaded.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status poll();

    //! \brief How many loads poll() made, for a counter on the screen.
    [[nodiscard]] std::size_t reloads() const
    {
        return m_reloads;
    }

private:

    struct Watched
    {
        ShaderSource vertex;
        ShaderSource fragment;
        Load load;
        std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>> files;
    };

    //! \brief The files of both stages, with their time of change.
    [[nodiscard]] static Status filesOf(Watched& p_watched);

    std::vector<Watched> m_watched;
    std::size_t m_reloads = 0u;
};

} // namespace compages::gpu
