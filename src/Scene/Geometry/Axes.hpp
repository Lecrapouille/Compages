//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef AXES_HPP
#define AXES_HPP

#include "Scene/Geometry/Geometry.hpp"

// *****************************************************************************
//! \brief Generate the geometry of a axis.
// *****************************************************************************
class Axes: public Geometry
{
public:

    struct Config
    {
        Config() {}
        float size = 1.0f;
    };

    //--------------------------------------------------------------------------
    //! \brief Generate the plane.
    //--------------------------------------------------------------------------
    virtual bool generate(GLVAO32& vao, const bool clear) override;

public:

    Config config;
};

#endif
