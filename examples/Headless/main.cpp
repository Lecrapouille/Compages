// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
//! \file
//! \brief Draw a scene into a PNG file on a machine without a screen.
//!
//! Compages never makes the OpenGL context: the program does, then calls
//! gpu::init(). The gallery makes it with a GLFW window. This program makes it
//! with compages::HeadlessContext (library Compages-Headless): EGL and no
//! surface at all, which works on a server, in a container or in a CI job,
//! with a GPU or with Mesa llvmpipe.
//!
//! Usage: Compages-headless [out.png] [width] [height]
// ****************************************************************************

#include "Compages/Compages.hpp"

#include "Compages/Headless/HeadlessContext.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace compages::si::literals;

namespace
{

// ----------------------------------------------------------------------------
//! \brief Three shapes, a camera and a sun, built, updated once, written.
// ----------------------------------------------------------------------------
compages::Status renderScene(std::string const& p_path,
                             std::uint32_t p_width,
                             std::uint32_t p_height)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.04f, 0.05f, 0.08f);
    scene.camera().position(0.0f, 2.0f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
    scene.box("Cube", compages::renderer::color(0.9f, 0.2f, 0.1f))
        .position(-1.2f, 0.0f, 0.0f)
        .rotation(0.6_rad, { 0.0f, 1.0f, 0.0f });
    scene.sphere("Ball", compages::renderer::color(0.2f, 0.7f, 0.3f));
    scene.cone("Cone", compages::renderer::color(0.2f, 0.4f, 0.9f))
        .position(1.2f, 0.0f, 0.0f);
    COMPAGES_TRY(scene.prepare());

    // One update computes the world matrices; then one picture.
    scene.update(compages::world::ViewFrame{});
    return scene.renderToFile(p_path, p_width, p_height);
}

} // namespace

int main(int argc, char* argv[])
{
    const std::string path = (argc > 1) ? argv[1] : "headless.png";
    const auto width = std::uint32_t((argc > 2) ? std::atoi(argv[2]) : 800);
    const auto height = std::uint32_t((argc > 3) ? std::atoi(argv[3]) : 600);

    auto context = compages::HeadlessContext::open();
    compages::Status status = context ? compages::success()
                                      : compages::failure(context.error());
    if (status)
    {
        status = compages::gpu::init(&compages::HeadlessContext::load);
    }
    if (status)
    {
        status = renderScene(path, width, height);
        compages::gpu::shutdown();
    }
    if (!status)
    {
        std::cerr << status.error() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "wrote " << path << std::endl;
    return EXIT_SUCCESS;
}
