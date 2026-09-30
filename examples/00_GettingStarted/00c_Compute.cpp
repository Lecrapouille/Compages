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

#include "00_GettingStarted/00c_Compute.hpp"

#include "Common/Gui.hpp"

namespace examples
{

//! \brief How many numbers, and the value they wrap around at.
constexpr std::size_t COUNT = 12u;
constexpr int TOP = 60;

//! \brief Run once per number, all at the same time. Each copy finds its
//! number from its own index and moves it by that index plus one, so the bars
//! on the right climb faster.
constexpr std::string_view STEP_SOURCE = R"(#version 450 core
layout(local_size_x = 16) in;

layout(std430, binding = 0) buffer Values
{
    int values[];
};

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= uint(values.length())) { return; }
    values[i] = (values[i] + int(i) + 1) % 60;
}
)";

//------------------------------------------------------------------------------
std::string IntroCompute::description() const
{
    return "A compute shader draws nothing: it is a function the GPU runs once "
           "per number, all at the same time. Each copy knows its index and "
           "moves "
           "its number up by that index plus one.\n\nThe bars show what the "
           "GPU "
           "holds. They are orange while the CPU copy is out of date: press "
           "download() in the Try it panel to bring the results back.";
}

//------------------------------------------------------------------------------
compages::Status IntroCompute::setUp()
{
    COMPAGES_TRY(m_chart.setUp());
    COMPAGES_TRY(m_step.load(STEP_SOURCE));

    // Storage, so that a compute shader may write the numbers the CPU gave.
    m_on_gpu = std::vector<int>(COUNT, 0);
    COMPAGES_TRY_ASSIGN(m_values,
                        compages::gpu::Buffer<int>::from(
                            m_on_gpu,
                            { .kind = compages::gpu::BufferKind::Storage,
                              .usage = compages::gpu::BufferUsage::Storage }));
    COMPAGES_TRY(m_values.upload());
    return compages::success();
}

//------------------------------------------------------------------------------
void IntroCompute::step()
{
    // Sends what changed on the CPU, runs one copy of the shader per number,
    // and waits until what it wrote can be read.
    if (!compages::gpu::dispatch(m_step, m_values))
    {
        return;
    }
    m_stale = true;

    // Reading back makes the CPU wait for the GPU: fine for a dozen numbers
    // in a lesson. A real program draws them where they are, see 12.
    compages::Result<std::vector<int>> read = m_values.read();
    if (read)
    {
        m_on_gpu = std::move(read.value());
    }
}

//------------------------------------------------------------------------------
void IntroCompute::draw(compages::world::ViewFrame const& p_frame)
{
    // Four steps a second when running, so that each one can be followed.
    m_since_step += p_frame.elapsed;
    if (m_running && (m_since_step >= 0.25f))
    {
        m_since_step = 0.0f;
        step();
    }
    m_chart.draw(m_on_gpu, TOP, 0u, m_stale ? m_on_gpu.size() : 0u);
}

//------------------------------------------------------------------------------
void IntroCompute::controls()
{
    ImGui::Checkbox("Run the shader four times a second", &m_running);
    if (ImGui::Button("dispatch() once"))
    {
        step();
    }
    ImGui::SameLine();
    // Replaces the CPU copy with what the GPU holds.
    if (ImGui::Button("download()") && m_values.download())
    {
        m_stale = false;
    }

    ImGui::Text("GPU:");
    for (int value : m_on_gpu)
    {
        ImGui::SameLine();
        ImGui::Text("%2d", value);
    }
    ImGui::Text("CPU:");
    for (int value : m_values.elements())
    {
        ImGui::SameLine();
        ImGui::TextColored(m_stale ? ImVec4(1.0f, 0.62f, 0.25f, 1.0f)
                                   : ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                           "%2d",
                           value);
    }
    ImGui::TextDisabled(m_stale ? "The CPU copy is out of date."
                                : "Both copies agree.");
}

} // namespace examples
