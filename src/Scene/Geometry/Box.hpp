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

#ifndef BOX_HPP
#define BOX_HPP

#include "Scene/Geometry/Geometry.hpp"

// *****************************************************************************
//! \brief Generate the geometry of a axis.
// *****************************************************************************
class Box: public Geometry
{
    enum Side
    {
        X = 0,
        Y = 1,
        Z = 2
    };

public:

    struct Config
    {
        Config() {}
        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
        uint32_t widthSegments = 1u;
        uint32_t heightSegments = 1u;
        uint32_t depthSegments = 1u;
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

    void buildPlane(Side u,
                    Side v,
                    Side w,
                    float udir,
                    float vdir,
                    float width,
                    float height,
                    float depth,
                    uint32_t gridX,
                    uint32_t gridY,
                    GLVertexBuffer<Vector3f>& vertices,
                    GLVertexBuffer<Vector3f>& normals,
                    GLVertexBuffer<Vector2f>& uv,
                    GLIndex32& index,
                    uint32_t& numberOfVertices);

public:

    Config config;
};

#endif
