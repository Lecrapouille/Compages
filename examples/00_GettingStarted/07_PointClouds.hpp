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

struct PointVertex
{
    Vector3f position;
};

// ****************************************************************************
//! \brief A dense sphere of points: a struct with one field, and the Points
//! primitive in the render state of the drawable.
// ****************************************************************************
class PointSphere: public Example
{
public:

    [[nodiscard]] std::string name() const override { return "07_PointClouds"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    //! \brief One PointVertex per point, drawn as points rather than triangles.
    compages::gpu::Drawable m_points;
};

} // namespace examples
