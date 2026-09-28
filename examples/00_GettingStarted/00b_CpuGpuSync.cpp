// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/00b_CpuGpuSync.hpp"

#include "Common/Gui.hpp"

namespace examples
{

//! \brief How tall the tallest bar may be.
constexpr int TOP = 60;

std::string CpuGpuSync::description() const
{
    return "The graphics card has its own memory. A compages::gpu::Buffer keeps a copy of "
           "its numbers on the CPU, used like a std::vector, and sends only what "
           "changed when upload() is called.\n\nThe bars show what the GPU holds. "
           "Press the buttons of the Try it panel: the CPU copy changes at once, the orange bars "
           "are those the GPU does not know about yet, and upload() sends them.";
}

void CpuGpuSync::readBack()
{
    // Reading back makes the CPU wait for the GPU: fine for five numbers in a
    // lesson, what a real frame avoids.
    compages::gpu::Result<std::vector<int>> read = m_values.read();
    if (read)
    {
        m_on_gpu = std::move(read.value());
    }
}

compages::gpu::Status CpuGpuSync::setUp()
{
    COMPAGES_TRY(m_chart.setUp());

    // On the CPU only: nothing is reserved on the device yet.
    m_values.assign({ 3, 5, 8, 13, 21 });

    // The one synchronisation point: device memory is reserved, then what
    // changed is sent.
    COMPAGES_TRY(m_values.upload());
    readBack();
    m_said = "assign() then upload(): both copies hold the same five numbers.";
    return compages::gpu::success();
}

void CpuGpuSync::draw(Frame const&)
{
    // What the GPU holds, with the elements upload() would send in orange.
    compages::gpu::DirtyRange const& pending = m_values.pending();
    m_chart.draw(m_on_gpu, TOP, pending.begin(), pending.empty() ? 0u : pending.count());
}

void CpuGpuSync::controls()
{
    // Changing an element: only the CPU copy moves, and the range of what the
    // next upload() sends grows to cover it.
    if (ImGui::Button("values[i] += 5"))
    {
        m_next = m_next % m_values.count();
        m_values[m_next] = (m_values[m_next] + 5) % TOP;
        m_said = "values[" + std::to_string(m_next) + "] changed on the CPU only.";
        m_next = m_next + 1u;
    }
    ImGui::SameLine();
    // Appending: the GPU memory has no room for it yet; upload() makes some.
    if (ImGui::Button("emplace_back(7)") && (m_values.count() < 12u))
    {
        m_values.emplace_back(7);
        m_said = "One more number, on the CPU only.";
    }
    ImGui::SameLine();
    if (ImGui::Button("upload()"))
    {
        const std::size_t sent = m_values.pending().empty() ? 0u : m_values.pending().count();
        if (m_values.upload())
        {
            readBack();
            m_said = "upload() sent " + std::to_string(sent) + " number(s) to the GPU.";
        }
    }

    // Both copies, side by side.
    ImGui::TextWrapped("%s", m_said.c_str());
    ImGui::Text("CPU:");
    for (int value : m_values.elements())
    {
        ImGui::SameLine();
        ImGui::Text("%3d", value);
    }
    ImGui::Text("GPU:");
    for (int value : m_on_gpu)
    {
        ImGui::SameLine();
        ImGui::Text("%3d", value);
    }
    ImGui::TextDisabled("GPU room: %zu numbers", m_values.deviceCapacity());
}

} // namespace examples
