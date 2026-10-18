// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_ScientificAndCompute/16_NeuralNetwork.hpp"

#include "Common/Gui.hpp"

#include "Compages/GPU/Errors.hpp"

#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

namespace examples
{

namespace
{

//! \brief Examples the network learns from.
constexpr std::uint32_t SAMPLES = 300u;
//! \brief Neurons in each of the two hidden layers.
constexpr int HIDDEN = 16;
//! \brief Every weight and bias, in one array (see the offsets in COMMON).
constexpr std::uint32_t PARAMETERS =
    (HIDDEN * 2) + HIDDEN + (HIDDEN * HIDDEN) + HIDDEN + HIDDEN + 1;
//! \brief Points across the coloured plane.
constexpr std::uint32_t GRID = 100u;
//! \brief Half the side of the square that is shown and learnt.
constexpr float EXTENT = 1.1f;

// ----------------------------------------------------------------------------
// The network, shared by the three compute shaders. All the weights are in
// one array of floats; the offsets say where each layer starts.
// ----------------------------------------------------------------------------
constexpr char const* COMMON = R"(#version 450 core
layout(local_size_x = 64) in;

const int H = 16;
const int W1 = 0;            // first layer: H x 2 weights
const int B1 = W1 + H * 2;   // then H biases
const int W2 = B1 + H;       // second layer: H x H
const int B2 = W2 + H * H;
const int W3 = B2 + H;       // output: H weights
const int B3 = W3 + H;       // and one bias
const int PARAMS = B3 + 1;

layout(std430, binding = 0) buffer Weights
{
    float w[];
};

// The forward pass: what each neuron outputs, and the answer, between 0 and 1.
float forward(vec2 x, out float h1[H], out float h2[H])
{
    for (int j = 0; j < H; ++j)
    {
        h1[j] = tanh(w[B1 + j] + w[W1 + 2 * j] * x.x + w[W1 + 2 * j + 1] * x.y);
    }
    for (int j = 0; j < H; ++j)
    {
        float sum = w[B2 + j];
        for (int k = 0; k < H; ++k)
        {
            sum += w[W2 + H * j + k] * h1[k];
        }
        h2[j] = tanh(sum);
    }
    float z = w[B3];
    for (int j = 0; j < H; ++j)
    {
        z += w[W3 + j] * h2[j];
    }
    return 1.0 / (1.0 + exp(-z));
}
)";

// ----------------------------------------------------------------------------
// Pass 1: one invocation per example. Forward, then the chain rule backward:
// the gradient of the loss for this example alone, one float per weight.
// ----------------------------------------------------------------------------
constexpr char const* GRADIENT = R"(
struct Sample
{
    vec2 position;
    float label;
    float pad;
};

layout(std430, binding = 1) readonly buffer Samples
{
    Sample samples[];
};

layout(std430, binding = 2) writeonly buffer Gradients
{
    float g[];
};

layout(std430, binding = 3) writeonly buffer Losses
{
    float losses[];
};

uniform uint count;

void main()
{
    uint s = gl_GlobalInvocationID.x;
    if (s >= count)
    {
        return;
    }
    vec2 x = samples[s].position;
    float target = samples[s].label;
    float h1[H];
    float h2[H];
    float y = forward(x, h1, h2);
    float clamped = clamp(y, 1e-6, 1.0 - 1e-6);
    losses[s] = -(target * log(clamped) + (1.0 - target) * log(1.0 - clamped));

    uint base = s * uint(PARAMS);
    // Sigmoid and cross-entropy together: the error of the output is y - t.
    float d3 = y - target;
    g[base + B3] = d3;
    float d2[H];
    for (int j = 0; j < H; ++j)
    {
        g[base + W3 + j] = d3 * h2[j];
        d2[j] = d3 * w[W3 + j] * (1.0 - h2[j] * h2[j]);   // tanh' = 1 - tanh^2
        g[base + B2 + j] = d2[j];
        for (int k = 0; k < H; ++k)
        {
            g[base + W2 + H * j + k] = d2[j] * h1[k];
        }
    }
    for (int k = 0; k < H; ++k)
    {
        float d1 = 0.0;
        for (int j = 0; j < H; ++j)
        {
            d1 += d2[j] * w[W2 + H * j + k];
        }
        d1 *= 1.0 - h1[k] * h1[k];
        g[base + B1 + k] = d1;
        g[base + W1 + 2 * k] = d1 * x.x;
        g[base + W1 + 2 * k + 1] = d1 * x.y;
    }
}
)";

// ----------------------------------------------------------------------------
// Pass 2: one invocation per weight. The mean of its gradients over every
// example, then an Adam step. One more invocation averages the losses.
// ----------------------------------------------------------------------------
constexpr char const* UPDATE = R"(
layout(std430, binding = 2) readonly buffer Gradients
{
    float g[];
};

layout(std430, binding = 3) readonly buffer Losses
{
    float losses[];
};

layout(std430, binding = 4) buffer Moments
{
    vec2 moments[];   // Adam: mean of the gradient, mean of its square
};

layout(std430, binding = 5) writeonly buffer Stats
{
    float stats[];
};

uniform uint count;
uniform float rate;
uniform int step;

void main()
{
    uint k = gl_GlobalInvocationID.x;
    if (k == uint(PARAMS))
    {
        float sum = 0.0;
        for (uint s = 0u; s < count; ++s)
        {
            sum += losses[s];
        }
        stats[0] = sum / float(count);
        return;
    }
    if (k > uint(PARAMS))
    {
        return;
    }
    float gradient = 0.0;
    for (uint s = 0u; s < count; ++s)
    {
        gradient += g[s * uint(PARAMS) + k];
    }
    gradient /= float(count);

    vec2 m = moments[k];
    m.x = 0.9 * m.x + 0.1 * gradient;
    m.y = 0.999 * m.y + 0.001 * gradient * gradient;
    moments[k] = m;
    float mean = m.x / (1.0 - pow(0.9, float(step)));
    float square = m.y / (1.0 - pow(0.999, float(step)));
    w[k] -= rate * mean / (sqrt(square) + 1e-8);
}
)";

// ----------------------------------------------------------------------------
// Pass 3: one invocation per point, the forward pass only.
// ----------------------------------------------------------------------------
constexpr char const* EVALUATE = R"(
struct Point
{
    vec2 position;
    float value;
    float pad;
};

layout(std430, binding = 6) buffer Points
{
    Point points[];
};

uniform uint count;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }
    float h1[H];
    float h2[H];
    points[i].value = forward(points[i].position, h1, h2);
}
)";

constexpr char const* PLANE_VERTEX = R"(#version 450 core
in vec2 position;
in float value;
uniform float aspect;
uniform float scale;
uniform float pixels;
out float vValue;

void main()
{
    vValue = value;
    gl_PointSize = pixels;
    gl_Position = vec4(position.x * scale / aspect, position.y * scale, 0.0, 1.0);
}
)";

constexpr char const* PLANE_FRAGMENT = R"(#version 450 core
in float vValue;
out vec4 oColor;

void main()
{
    // Blue for class 0, orange for class 1, dark where the network hesitates.
    vec3 zero = vec3(0.15, 0.35, 0.75);
    vec3 one = vec3(0.85, 0.45, 0.12);
    float sure = abs(vValue - 0.5) * 2.0;
    oColor = vec4(mix(zero, one, vValue) * (0.25 + 0.45 * sure), 1.0);
}
)";

constexpr char const* DOT_VERTEX = R"(#version 450 core
in vec2 position;
in float label;
uniform float aspect;
uniform float scale;
out float vLabel;

void main()
{
    vLabel = label;
    gl_PointSize = 8.0;
    gl_Position = vec4(position.x * scale / aspect, position.y * scale, 0.0, 1.0);
}
)";

constexpr char const* DOT_FRAGMENT = R"(#version 450 core
in float vLabel;
out vec4 oColor;

void main()
{
    float r = length(gl_PointCoord - vec2(0.5));
    if (r > 0.5)
    {
        discard;
    }
    vec3 color = (vLabel > 0.5) ? vec3(1.0, 0.65, 0.3) : vec3(0.4, 0.65, 1.0);
    oColor = vec4((r > 0.38) ? vec3(1.0) : color, 1.0);
}
)";

std::string source(char const* p_pass)
{
    return std::string(COMMON) + p_pass;
}

//! \brief The examples of one dataset, made on the CPU.
std::vector<NeuralNetwork::Sample> makeSamples(int p_dataset,
                                               std::mt19937& p_random)
{
    std::uniform_real_distribution<float> plane(-1.0f, 1.0f);
    std::normal_distribution<float> noise(0.0f, 0.03f);
    std::vector<NeuralNetwork::Sample> samples(SAMPLES);
    for (std::uint32_t i = 0u; i < SAMPLES; ++i)
    {
        NeuralNetwork::Sample& sample = samples[i];
        sample.pad = 0.0f;
        if (p_dataset == 2)
        {
            // Two spirals, one turning inside the other.
            const float t = float(i / 2u) / float(SAMPLES / 2u);
            const float angle = (t * 3.0f * std::numbers::pi_v<float>)+(
                (i % 2u == 0u) ? 0.0f : std::numbers::pi_v<float>);
            const float radius = 0.1f + (0.85f * t);
            sample.position = compages::core::Vector2f(
                (radius * std::cos(angle)) + noise(p_random),
                (radius * std::sin(angle)) + noise(p_random));
            sample.label = float(i % 2u);
            continue;
        }
        sample.position =
            compages::core::Vector2f(plane(p_random), plane(p_random));
        const compages::core::Vector2f& p = sample.position;
        sample.label = (p_dataset == 0)
                           ? (((p.x * p.x) + (p.y * p.y) < 0.36f) ? 1.0f : 0.0f)
                           : (((p.x * p.y) > 0.0f) ? 1.0f : 0.0f);
    }
    return samples;
}

} // namespace

std::string NeuralNetwork::description() const
{
    return "A neural network (2 -> 16 -> 16 -> 1) learns to separate two "
           "classes of points. The CPU makes the examples and reads the "
           "answers; the GPU does the forward pass, the backward pass and the "
           "update of the weights in three compute shaders. The colour is the "
           "answer of the network at each point of the plane.";
}

compages::Status NeuralNetwork::setUp()
{
    COMPAGES_TRY(m_gradient_pass.load(source(GRADIENT)));
    COMPAGES_TRY(m_update_pass.load(source(UPDATE)));
    COMPAGES_TRY(m_evaluate_pass.load(source(EVALUATE)));

    // Storage buffers the GPU fills itself: no data from the CPU.
    constexpr auto storage = compages::gpu::BufferKind::Storage;
    constexpr auto usage = compages::gpu::BufferUsage::Storage;
    COMPAGES_TRY_ASSIGN(
        m_gradients,
        compages::gpu::Buffer<float>::create(
            std::size_t{ SAMPLES } * PARAMETERS, storage, usage));
    COMPAGES_TRY_ASSIGN(
        m_losses,
        compages::gpu::Buffer<float>::create(SAMPLES, storage, usage));
    COMPAGES_TRY_ASSIGN(
        m_stats, compages::gpu::Buffer<float>::create(1u, storage, usage));

    // The plane: one point per cell, the network writes its value.
    std::vector<Point> grid;
    grid.reserve(std::size_t{ GRID } * GRID);
    for (std::uint32_t y = 0u; y < GRID; ++y)
    {
        for (std::uint32_t x = 0u; x < GRID; ++x)
        {
            const float u = ((float(x) + 0.5f) / float(GRID) * 2.0f) - 1.0f;
            const float v = ((float(y) + 0.5f) / float(GRID) * 2.0f) - 1.0f;
            grid.push_back(Point{
                compages::core::Vector2f(u * EXTENT, v * EXTENT), 0.5f, 0.0f });
        }
    }
    COMPAGES_TRY_ASSIGN(m_field,
                        compages::gpu::Buffer<Point>::from(
                            grid, { .kind = storage, .usage = usage }));
    COMPAGES_TRY(m_field.upload());

    COMPAGES_TRY(m_plane.load(PLANE_VERTEX, PLANE_FRAGMENT));
    m_plane.vertices(m_field);
    m_plane.primitive(compages::gpu::Primitive::Points);
    COMPAGES_TRY(m_dots.load(DOT_VERTEX, DOT_FRAGMENT));
    m_dots.primitive(compages::gpu::Primitive::Points);

    COMPAGES_TRY(reset());
    return m_plane.prepare();
}

compages::Status NeuralNetwork::reset()
{
    constexpr auto storage = compages::gpu::BufferKind::Storage;
    constexpr auto usage = compages::gpu::BufferUsage::Storage;

    // The inputs, from the CPU.
    COMPAGES_TRY_ASSIGN(m_samples,
                        compages::gpu::Buffer<Sample>::from(
                            makeSamples(m_dataset, m_random),
                            { .kind = storage, .usage = usage }));
    COMPAGES_TRY(m_samples.upload());
    m_dots.vertices(m_samples);

    // Small random weights (Xavier): each neuron starts a little different.
    std::vector<float> weights(PARAMETERS, 0.0f);
    std::normal_distribution<float> gauss(0.0f, 1.0f);
    for (int j = 0; j < HIDDEN; ++j)
    {
        weights[std::size_t(2 * j)] =
            gauss(m_random) * std::sqrt(1.0f / 2.0f) * 2.0f;
        weights[std::size_t((2 * j) + 1)] =
            gauss(m_random) * std::sqrt(1.0f / 2.0f) * 2.0f;
        for (int k = 0; k < HIDDEN; ++k)
        {
            weights[std::size_t((3 * HIDDEN) + (HIDDEN * j) + k)] =
                gauss(m_random) * std::sqrt(1.0f / float(HIDDEN));
        }
        weights[std::size_t((4 * HIDDEN) + (HIDDEN * HIDDEN) + j)] =
            gauss(m_random) * std::sqrt(1.0f / float(HIDDEN));
    }
    COMPAGES_TRY_ASSIGN(m_weights,
                        compages::gpu::Buffer<float>::from(
                            weights, { .kind = storage, .usage = usage }));
    COMPAGES_TRY(m_weights.upload());
    const std::vector<compages::core::Vector2f> zero(
        PARAMETERS, compages::core::Vector2f(0.0f, 0.0f));
    COMPAGES_TRY_ASSIGN(m_moments,
                        compages::gpu::Buffer<compages::core::Vector2f>::from(
                            zero, { .kind = storage, .usage = usage }));
    COMPAGES_TRY(m_moments.upload());

    m_step = 0;
    m_loss = 0.0f;
    return compages::success();
}

void NeuralNetwork::train(int p_iterations)
{
    m_gradient_pass.set("count", SAMPLES);
    m_update_pass.set("count", SAMPLES);
    m_update_pass.set("rate", m_rate);
    if (!compages::gpu::check(m_gradient_pass.bind("Weights", m_weights)) ||
        !compages::gpu::check(m_gradient_pass.bind("Samples", m_samples)) ||
        !compages::gpu::check(m_gradient_pass.bind("Gradients", m_gradients)) ||
        !compages::gpu::check(m_gradient_pass.bind("Losses", m_losses)) ||
        !compages::gpu::check(m_update_pass.bind("Weights", m_weights)) ||
        !compages::gpu::check(m_update_pass.bind("Gradients", m_gradients)) ||
        !compages::gpu::check(m_update_pass.bind("Losses", m_losses)) ||
        !compages::gpu::check(m_update_pass.bind("Moments", m_moments)) ||
        !compages::gpu::check(m_update_pass.bind("Stats", m_stats)))
    {
        return;
    }
    for (int i = 0; i < p_iterations; ++i)
    {
        // The update reads every gradient: they must all be written first.
        if (!compages::gpu::check(m_gradient_pass.dispatchItems(SAMPLES)))
        {
            return;
        }
        compages::gpu::barrier(compages::gpu::Barrier::Storage);
        m_update_pass.set("step", ++m_step);
        if (!compages::gpu::check(m_update_pass.dispatchItems(PARAMETERS + 1u)))
        {
            return;
        }
        // And the next gradients read the new weights.
        compages::gpu::barrier(compages::gpu::Barrier::Storage);
    }
}

std::vector<float>
NeuralNetwork::ask(std::vector<compages::core::Vector2f> const& p_points)
{
    // CPU -> GPU: the questions; GPU: the forward pass; GPU -> CPU: the
    // answers.
    std::vector<Point> questions;
    questions.reserve(p_points.size());
    for (compages::core::Vector2f const& point : p_points)
    {
        questions.push_back(Point{ point, 0.0f, 0.0f });
    }
    auto created = compages::gpu::Buffer<Point>::from(
        questions,
        { .kind = compages::gpu::BufferKind::Storage,
          .usage = compages::gpu::BufferUsage::Storage });
    if (!created)
    {
        compages::gpu::reportError(created.error());
        return {};
    }
    m_questions = created.take();
    m_evaluate_pass.set("count", std::uint32_t(questions.size()));
    if (!compages::gpu::check(m_questions.upload()) ||
        !compages::gpu::check(m_evaluate_pass.bind("Weights", m_weights)) ||
        !compages::gpu::check(m_evaluate_pass.bind("Points", m_questions)) ||
        !compages::gpu::check(
            m_evaluate_pass.dispatchItems(std::uint32_t(questions.size()))))
    {
        return {};
    }
    compages::gpu::barrier(compages::gpu::Barrier::Storage);
    compages::Result<std::vector<Point>> answers = m_questions.read();
    if (!answers)
    {
        compages::gpu::reportError(answers.error());
        return {};
    }
    std::vector<float> values;
    values.reserve(answers.value().size());
    for (Point const& answer : answers.value())
    {
        values.push_back(answer.value);
    }
    return values;
}

void NeuralNetwork::draw(compages::world::ViewFrame const& p_frame)
{
    if (m_dirty)
    {
        m_dirty = false;
        if (!compages::gpu::check(reset()))
        {
            return;
        }
    }
    if (m_running)
    {
        train(m_iterations_per_frame);
    }

    // The loss and the answer to the question of the panel, a few times a
    // second: reading back makes the CPU wait for the GPU.
    if ((m_frames++ % 10) == 0)
    {
        compages::Result<std::vector<float>> stats = m_stats.read();
        if (stats && !stats.value().empty())
        {
            m_loss = stats.value()[0];
        }
        std::vector<float> answer = ask({ m_question });
        if (!answer.empty())
        {
            m_answer = answer[0];
        }
    }

    // Colour the plane with the answers of the network.
    m_evaluate_pass.set("count", GRID * GRID);
    if (!compages::gpu::check(m_evaluate_pass.bind("Weights", m_weights)) ||
        !compages::gpu::check(m_evaluate_pass.bind("Points", m_field)) ||
        !compages::gpu::check(m_evaluate_pass.dispatchItems(GRID * GRID)))
    {
        return;
    }
    compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);

    const float aspect = compages::core::aspect(p_frame);
    const float scale = 0.95f / EXTENT / std::max(1.0f, 1.0f / aspect);
    const float side =
        float(std::min(p_frame.width, p_frame.height)) * std::min(1.0f, aspect);
    compages::gpu::clear({ 0.02f, 0.02f, 0.04f });
    m_plane["aspect"] = aspect;
    m_plane["scale"] = scale;
    m_plane["pixels"] = std::ceil(side * 0.95f / float(GRID)) + 1.0f;
    m_plane.draw();
    m_dots["aspect"] = aspect;
    m_dots["scale"] = scale;
    m_dots.vertices(m_samples);
    m_dots.draw();
}

void NeuralNetwork::printToConsole()
{
    // Every example, and what the network says about it.
    std::vector<compages::core::Vector2f> inputs;
    compages::Result<std::vector<Sample>> samples = m_samples.read();
    if (!samples)
    {
        return;
    }
    for (Sample const& sample : samples.value())
    {
        inputs.push_back(sample.position);
    }
    const std::vector<float> outputs = ask(inputs);
    std::size_t right = 0u;
    std::cout << "[16_NeuralNetwork] x y label -> output\n";
    for (std::size_t i = 0u; i < outputs.size(); ++i)
    {
        Sample const& sample = samples.value()[i];
        right += ((outputs[i] > 0.5f) == (sample.label > 0.5f)) ? 1u : 0u;
        std::cout << sample.position.x << ' ' << sample.position.y << ' '
                  << sample.label << " -> " << outputs[i] << '\n';
    }
    std::cout << "[16_NeuralNetwork] " << right << " / " << outputs.size()
              << " right, loss " << m_loss << ", after " << m_step << " steps"
              << std::endl;
}

void NeuralNetwork::controls()
{
    static char const* const datasets[] = { "Circle", "XOR", "Two spirals" };
    if (ImGui::Combo("Examples", &m_dataset, datasets, 3))
    {
        m_dirty = true;
    }
    ImGui::Checkbox("Learn", &m_running);
    ImGui::SameLine();
    m_dirty = ImGui::Button("Start again") || m_dirty;
    ImGui::SliderInt("Steps per frame", &m_iterations_per_frame, 1, 100);
    ImGui::SliderFloat("Learning rate", &m_rate, 0.001f, 0.05f, "%.3f");
    ImGui::Text("Steps: %d   loss: %.4f", m_step, double(m_loss));

    ImGui::SeparatorText("Ask the network");
    ImGui::SliderFloat2("Point", &m_question.x, -EXTENT, EXTENT, "%.2f");
    ImGui::Text(
        "Answer: %.3f (class %d)", double(m_answer), (m_answer > 0.5f) ? 1 : 0);
    if (ImGui::Button("Print every example and its answer to the console"))
    {
        printToConsole();
    }
}

} // namespace examples
