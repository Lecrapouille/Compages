// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
