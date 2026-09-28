// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief One triangle, from a shader and its attributes given by name.
//!
//! A compages::gpu::Drawable is a shader with its data. The shader declares what a
//! vertex is, so the values are given attribute by attribute:
//! \code
//! m_triangle["position"] = { {-0.8f, -0.6f}, {0.8f, -0.6f}, {0.0f, 0.8f} };
//! \endcode
//! and stored interleaved, one whole vertex after the other, which is what the
//! GPU reads fastest. No buffer, layout or pipeline to write: the drawable
//! derives them from the shader and sends the vertices at the first draw.
//!
//! 01c_InterleavedTriangle draws the same triangle from a C++ struct.
// ****************************************************************************
class Triangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "01b_Triangle";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::gpu::Drawable m_triangle;
};

} // namespace examples
