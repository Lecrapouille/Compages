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
//! \brief A long fragment shader tuned by a handful of uniforms.
//!
//! The uniforms that never change are set once at set up and keep their
//! value; only the time is set per frame.
// ****************************************************************************
class ComplexShader: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06b_ComplexShader";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::gpu::Drawable m_quad;
};

} // namespace examples
