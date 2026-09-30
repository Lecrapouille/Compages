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

#include "Common/ExampleManifest.hpp"

#include <iostream>

// ****************************************************************************
//! \file
//! \brief Every example, in one program.
//!
//! Run it with no argument to start at the first one, or with the name of an
//! example to start there:
//! \code
//! ./build/Compages-examples 04_DepthAndTransforms
//! ./build/Compages-examples --check
//! \endcode
//!
//! With --check, every example is built, drawn for a few frames and closed, and
//! the program fails if any of them could not run or left anything behind on
//! the device. That is a leak test as much as a smoke test, and it is the
//! reason all of the examples are one program rather than seventeen.
//!
//! The examples are meant to be read in order. Each one adds a single idea to
//! the one before it, and says in its own comments which idea that is and why
//! it is there.
// ****************************************************************************

int main(int argc, char* argv[])
{
    examples::Gallery gallery;
    examples::registerManifest(gallery);

    examples::Gallery::Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--check")
        {
            // Enough frames that a first draw doing more than the following
            // ones, such as the first upload of a vertex array, is included.
            options.frames = 10u;
            options.visit_all = true;
        }
        else if (argument == "--cycle")
        {
            // Five seconds each, going round for ever. What to leave running on
            // a second screen while working on the library.
            options.frames = 300u;
        }
        else if (argument == "--no-overlay")
        {
            options.overlay = false;
        }
        else if ((argument == "--shots") && ((i + 1) < argc))
        {
            options.screenshots = argv[++i];
        }
        else if ((argument == "--shots-with-panels") && ((i + 1) < argc))
        {
            options.screenshots = argv[++i];
            options.screenshots_with_panels = true;
        }
        else if ((argument == "--help") || (argument == "-h"))
        {
            std::cout
                << "Usage: " << argv[0] << " [options] [name of an example]\n"
                << "  --check       draw every example for a few frames, then "
                   "report anything\n"
                << "                that failed or was left on the device, and "
                   "exit\n"
                << "  --cycle       move on to the next example every few "
                   "seconds\n"
                << "  --no-overlay  start with the panels hidden (F1 shows "
                   "them)\n"
                << "  --shots DIR   write one picture per example into DIR, "
                   "which "
                   "needs --check\n"
                << "                or --cycle to say when to take them\n"
                << "  --shots-with-panels DIR  same, the whole window with its "
                   "panels\n"
                << "\nIn the window: PgUp/PgDn change example, F1 panels, F2 "
                   "wireframe,\nF5 restart, F6 pause, F7 one frame, F12 "
                   "screenshot, Esc quit."
                << std::endl;
            return EXIT_SUCCESS;
        }
        else
        {
            options.start = argument;
        }
    }

    compages::Status ran = gallery.run(options);
    if (!ran)
    {
        std::cerr << ran.error() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
