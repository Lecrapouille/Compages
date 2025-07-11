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

#ifndef PLANE_HPP
#define PLANE_HPP

#include "Scene/Geometry/Geometry.hpp"

// *****************************************************************************
//! \brief Generate the geomatry of a plane.
// *****************************************************************************
class Plane: public Geometry
{
public:

    struct Config
    {
        Config() {}
        float width = 1.0f;
        float height = 1.0f;
        size_t widthSegments = 1_z;
        size_t heightSegments = 1_z;
    };

    virtual bool generate(GLVAO32& vao, const bool clear) override;

protected:

    //--------------------------------------------------------------------------
    //! \brief Generate the plane.
    //--------------------------------------------------------------------------
    bool generate(GLVertexBuffer<Vector3f>& vertices,
                  GLVertexBuffer<Vector3f>& normals,
                  GLVertexBuffer<Vector2f>& uv,
                  GLIndex32& index);

public:

    Config config;
};

#endif
