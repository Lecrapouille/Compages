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

class Dummy final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "00a_Dummy";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override
    {
        return compages::success();
    }
    void draw(compages::world::ViewFrame const& p_frame) override;
};

} // namespace examples
