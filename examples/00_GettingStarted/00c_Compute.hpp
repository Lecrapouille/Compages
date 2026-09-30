// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/BarChart.hpp"
#include "Common/Example.hpp"

#include "Compages/GPU/Compute.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief The GPU used as a calculator: a compute shader changes numbers.
//!
//! A compute shader draws nothing. It is a function the GPU runs many times
//! at once, each copy knowing its own index, gl_GlobalInvocationID.x. Here each
//! copy moves one number of a buffer up by its index plus one:
//! \code
//! COMPAGES_TRY(m_step.load(STEP_SOURCE));
//! COMPAGES_TRY(compages::gpu::dispatch(m_step, m_values));   // one copy per
//! number COMPAGES_TRY(m_values.download());               // the results, on
//! the CPU
//! \endcode
//! The bars are what the GPU holds. The CPU copy of the buffer does not follow
//! on its own: it is out of date, the bars orange, until download() is called.
// ****************************************************************************
class IntroCompute final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "00c_Compute";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief Run the shader once over every number, and read the bars back.
    void step();

    compages::gpu::ComputeProgram m_step;
    compages::gpu::Buffer<int> m_values;
    //! \brief What the GPU held after the last step, for the bars.
    std::vector<int> m_on_gpu;
    //! \brief The CPU copy is older than what the GPU holds.
    bool m_stale = false;
    //! \brief Step on its own, a few times a second.
    bool m_running = true;
    float m_since_step = 0.0f;
    BarChart m_chart;
};

} // namespace examples
