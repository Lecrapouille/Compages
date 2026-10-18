// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A small neural network learns on the GPU; the CPU gives the inputs
//! and reads the outputs.
//!
//! A multilayer perceptron, 2 -> 16 -> 16 -> 1 (tanh, tanh, sigmoid), learns
//! to tell two classes of points apart. The CPU makes the examples and the
//! first weights, and puts them in storage buffers. Three compute shaders do
//! the rest:
//! - gradients: one invocation per example, forward then backward pass;
//! - update: one invocation per weight, the mean of the gradients and an
//!   Adam step;
//! - evaluate: one invocation per point, the forward pass only. It colours
//!   the plane, and answers the questions of the CPU (read back).
// ****************************************************************************
class NeuralNetwork final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "16_NeuralNetwork";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

    //! \brief One example: a point of the plane and its class (0 or 1).
    //! Same layout as the GLSL struct (std430: 16 bytes).
    struct Sample
    {
        compages::core::Vector2f position;
        float label;
        float pad;
    };

    //! \brief A point where the network is asked for its answer.
    struct Point
    {
        compages::core::Vector2f position;
        float value;
        float pad;
    };

private:

    [[nodiscard]] compages::Status reset();
    void train(int p_iterations);
    //! \brief The answers of the network at these points, computed on the GPU
    //! and read back.
    [[nodiscard]] std::vector<float>
    ask(std::vector<compages::core::Vector2f> const& p_points);
    void printToConsole();

    compages::gpu::Buffer<float> m_weights;
    compages::gpu::Buffer<compages::core::Vector2f> m_moments;
    compages::gpu::Buffer<Sample> m_samples;
    compages::gpu::Buffer<float> m_gradients;
    compages::gpu::Buffer<float> m_losses;
    compages::gpu::Buffer<float> m_stats;
    compages::gpu::Buffer<Point> m_field;
    compages::gpu::Buffer<Point> m_questions;

    compages::gpu::ComputeProgram m_gradient_pass;
    compages::gpu::ComputeProgram m_update_pass;
    compages::gpu::ComputeProgram m_evaluate_pass;
    compages::gpu::Drawable m_plane;
    compages::gpu::Drawable m_dots;

    std::mt19937 m_random{ 16u };
    int m_dataset = 2; // 0 circle, 1 xor, 2 spirals
    bool m_dirty = false;
    bool m_running = true;
    int m_iterations_per_frame = 20;
    float m_rate = 0.01f;
    std::int32_t m_step = 0;
    float m_loss = 0.0f;
    int m_frames = 0;
    compages::core::Vector2f m_question{ 0.3f, -0.2f };
    float m_answer = 0.5f;
};

} // namespace examples
