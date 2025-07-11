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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_SHAPE_NODE_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_SHAPE_NODE_HPP

#include "OpenGL/Buffers/iVAO.hpp"
#include "Scene/Camera/CameraNode.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "Scene/Material/Material.hpp"
#include "Scene/Material/ShaderLib.hpp"
#include "Scene/SceneTree.hpp"

// *****************************************************************************
//! \brief Base class for Shape<Geometry,Material> since sometines we need to
//! get the ancestor (tree traversal) and with template class this quite boring.
// *****************************************************************************
class BaseShape
{
public:

    virtual ~BaseShape() = default;
    virtual GLIndex32& index() = 0;
    virtual GLVertexBuffer<Vector3f>& vertices() = 0;
    virtual GLVertexBuffer<Vector3f>& normals() = 0;
    virtual GLVertexBuffer<Vector2f>& uv() = 0;
    virtual Matrix44f& modelMatrix() = 0;
    virtual Matrix44f& viewMatrix() = 0;
    virtual Matrix44f& projectionMatrix() = 0;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
template <class Geometry, class Material>
class Shape: public BaseShape, public SceneObject
{
public:

    using Ptr = std::unique_ptr<Shape<Geometry, Material>>;

    Shape(std::string const& name, Mode const mode = Mode::TRIANGLES)
        : SceneObject(name),
          m_vao("VAO_" + name),
          material(m_vao), // will use the material default config
          m_drawMode(mode)
    {
    }

    bool compile()
    {
        if (!material.compile())
        {
            std::cerr << "Shape " << name() << ": Failed creating its material"
                      << std::endl;

            return false;
        }

        if (!material.program.bind(m_vao))
        {
            std::cerr << "Shape " << name() << ": Failed binding its VAO "
                      << std::endl;
            return false;
        }

        if (!geometry.generate(m_vao, true))
        {
            std::cerr << "Shape " << name() << ": Failed creating its geometry"
                      << std::endl;
            return false;
        }

        return true;
    }

    virtual void onCameraUpdated(Camera& camera) override
    {
        // TODO if camera.updated()
        viewMatrix() = camera.view();
        projectionMatrix() = camera.projection();
    }

    virtual bool
    onDraw(Matrix44f const& model_matrix = Matrix44f(matrix::Identity)) override
    {
        modelMatrix() = model_matrix;
        return m_vao.draw(m_drawMode);
    }

    virtual GLIndex32& index() override
    {
        return m_vao.index();
    }

    virtual GLVertexBuffer<Vector3f>& vertices() override
    {
        return m_vao.vector3f(shaders::name::position);
    }

    virtual GLVertexBuffer<Vector3f>& normals() override
    {
        return m_vao.vector3f(shaders::name::normal);
    }

    virtual GLVertexBuffer<Vector2f>& uv() override
    {
        return m_vao.vector2f(shaders::name::uv);
    }

    //! \bref Vertex coordinates of our model starts in local space as local
    //! coordinates (relative to its local origin). This matrix make them
    //! process to world coordinates which are coordinates in respect of a
    //! larger world. These coordinates are relative to some global origin of
    //! the world, together with many other objects also placed relative to this
    //! world's origin.
    //!
    //! V_clip = M_proj * M_view * M_model * V_local
    virtual Matrix44f& modelMatrix() override
    {
        return material.program.matrix44f("modelMatrix");
    }

    //! \bref After modelMatrix() this matrix make process coordinates of our
    //! model to view coordinates in such a way that each coordinate is as seen
    //! from the camera or viewer's point of view.
    //!
    //! V_clip = M_proj * M_view * M_model * V_local
    virtual Matrix44f& viewMatrix() override
    {
        return material.program.matrix44f("viewMatrix");
    }

    //! \bref After viewMatrix() the coordinates are in view space we want to
    //! project them to clip coordinates. Clip coordinates are processed to the
    //! -1.0 and 1.0 range and determine which vertices will end up on the
    //! screen.
    //!
    //! V_clip = M_proj * M_view * M_model * V_local
    virtual Matrix44f& projectionMatrix() override
    {
        return material.program.matrix44f("projectionMatrix");
    }

protected:

    GLVAO32 m_vao;

public:

    Material material;
    Geometry geometry;

private:

    Mode const m_drawMode;
};

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_SHAPE_NODE_HPP
