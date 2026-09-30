//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "Common/BarChart.hpp"
#include "Common/Example.hpp"

#include "Compages/GPU/Buffer.hpp"

#include <string>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Two copies of the same numbers: one on the CPU, one on the GPU.
//!
//! The graphics card has a memory of its own. A compages::gpu::Buffer keeps a
//! copy of its elements on the CPU, used like a std::vector, and remembers
//! which ones were changed since they were last sent:
//! \code
//! m_values.assign({ 3, 5, 8, 13, 21 });   // on the CPU only
//! m_values[2u] = 42;                      // still on the CPU only
//! COMPAGES_TRY(m_values.upload());         // now on the GPU too
//! \endcode
//! The bars are what the GPU holds, read back from it. The buttons of the
//! "Try it" panel change the CPU copy: the bars turn orange where the two
//! copies disagree, and catch up when upload() is pressed. A drawable does that
//! upload itself at every draw, which is why no later example calls it.
// ****************************************************************************
class CpuGpuSync final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "00b_CpuGpuSync";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief Read what the GPU holds into m_on_gpu, for the bars.
    void readBack();

    compages::gpu::Buffer<int> m_values;
    //! \brief What the GPU held when last asked.
    std::vector<int> m_on_gpu;
    //! \brief Which element the next "+5" changes.
    std::size_t m_next = 0u;
    //! \brief What the last button did, in words.
    std::string m_said;
    BarChart m_chart;
};

} // namespace examples
