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

#ifndef SHAPE_LOADER_HPP
#define SHAPE_LOADER_HPP

#include <OpenGLCppWrapper/OpenGLCppWrapper.hpp>

class ShapeLoader
{
public:

    virtual ~ShapeLoader() = default;
    virtual bool load(std::string const& fileName,
                      GLVertexBuffer<Vector3f>& vertices,
                      GLVertexBuffer<Vector3f>& normals,
                      GLVertexBuffer<Vector2f>& uv,
                      GLIndex32& indices) = 0;

    inline std::string const& error() const
    {
        return m_error;
    }

protected:

    std::string m_error;
};

#endif
