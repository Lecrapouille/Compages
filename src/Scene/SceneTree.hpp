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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_SCENE_TREE_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_SCENE_TREE_HPP

#include "Math/Transformable.hpp"
#include "Scene/GameObject.hpp"
#include "Scene/Tree.hpp"

class Camera;

// *****************************************************************************
//! \brief Class container for holding a 3D scene.
//!
//! A scene is a spatial representation of a graphical scene using a tree
//! structure. Instead of storing 3D models (entities) in an array, where
//! their positions are related to the world origin, making more difficult to
//! move these entites inside the world, a scene classifies entities inside a
//! tree structure where parent-children relations between nodes allow to
//! define relative transformations of their positions given by a transform
//! matrix (translation, rotation and scaling). This way, the animation of
//! entities are easier because modifying the matrix of one node will
//! automatically impact the position of the descendant nodes. Computed
//! matrices are usually passed to the GLSL shaders through a 4x4 uniform
//! matrix with the name of "model matrix".
//!
//! For more information you can read this document "Scene Graphs" at
//!   https://research.ncl.ac.uk/game/mastersdegree/graphicsforgames/
// *****************************************************************************
class SceneTree
{
public:

    //--------------------------------------------------------------------------
    //! \brief Release all node attached to this scene.
    //--------------------------------------------------------------------------
    ~SceneTree();

    //--------------------------------------------------------------------------
    //! \brief Extend the Tree node class by adding data (transform matrices
    //! ...) while keep knowing the parent node and child nodes.
    //--------------------------------------------------------------------------
    class Node: public GameObject, public Tree<Node>
    {
        friend class SceneTree;

    public:

        //----------------------------------------------------------------------
        //! \brief Print on the console internal states of a node.
        //! \note This is not a recursive method, call debug() instead.
        //----------------------------------------------------------------------
        inline friend std::ostream& operator<<(std::ostream& os, Node& node)
        {
            os << "Node id " << node.id() << ": " << node.name() << std::endl
               << " transform: " << node.transform.matrix() << std::endl
               << " enabled: " << node.enabled() << std::endl
               << " has " << node.children.size() << " children:" << std::endl
               << " ";
            for (auto const& it : node.children)
                os << "  " << it->name();
            if (node.children.size() != 0u)
                os << std::endl;
            return os;
        }

        //----------------------------------------------------------------------
        //! \brief Create a Node with a name. Names are supposed to be unique
        //! over stored nodes. For performance reason unicity of names will not
        //! be checked. The transform matrix maked the node be placed at the
        //! origin of the world.
        //----------------------------------------------------------------------
        Node(std::string const& name /* TODO: components*/) : GameObject(name)
        {
        }

        //----------------------------------------------------------------------
        //! \brief Create a Node with a name and a tag. Name are supposed to be
        //! unique over stored nodes. For performance reason unicity of names
        //! will not be checked. Tags allows to search group of nodes. The
        //! transform matrix maked the node be placed at the origin of the
        //! world.
        //----------------------------------------------------------------------
        Node(std::string const& name, std::string const& tag_)
            : GameObject(name, tag_)
        {
        }

        //----------------------------------------------------------------------
        //! \brief Return the const reference of the world transformation
        //! matrix. This matrix allows to place the object in the world.
        //----------------------------------------------------------------------
        inline Matrix44f const& worldTransform() const
        {
            return m_world_transform;
        }

    public:

        //! \brief Relative transformation to parent node. This allows to give
        //! to this instance a relative movement to its parent.
        Transformable3D transform;

    private:

        //! \brief The matrix transform to apply on the child. Transformations
        //! are relative to the parent node.
        Matrix44f m_world_transform{ matrix::Identity };
    };

    //--------------------------------------------------------------------------
    //! \brief Find the first node by its name or by hierarchy.
    //!
    //! \param[in] name: the name of the node.
    //!   - If \c name starts by '/' then do a hierarchy search from the
    //!     root. Each node shall be seperated by the '/' char. Complexity is
    //!     O(n) where n the number of '/' chars.
    //!   - If \c name does not start by '/' then find and return the first node
    //!     matching the given node. Complexity is O(n) where n the number of
    //!     nodes.
    //!
    //! \return the Node address if found or nullptr if not found.
    //--------------------------------------------------------------------------
    Node* get(std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Find several elements from their tag.
    //! \param[in] tag the tag filtering nodes.
    //! \param[out] found the container holding found nodes.
    //! \param[in] clear if true \c found id clear first.
    //! \return the number of elements found.
    //--------------------------------------------------------------------------
    size_t getByTag(std::string const& tag,
                    std::vector<Node*> found,
                    bool clear = true);

    //--------------------------------------------------------------------------
    //! \brief Traverse the scene and print information on each node to the
    //! console.
    //--------------------------------------------------------------------------
    void debug();

    //--------------------------------------------------------------------------
    //! \brief Traverse the scene and call onSetup() on each node.
    //--------------------------------------------------------------------------
    void setup();

    //--------------------------------------------------------------------------
    //! \brief Traverse the scene and call onUpdate() on each node to rebuild
    //! tansformation matrices.
    //! \param[in] dt: delta time from the previous frame.
    //--------------------------------------------------------------------------
    void update(float const dt);

    //--------------------------------------------------------------------------
    //! \brief Traverse the scene and call the draw() method.
    //--------------------------------------------------------------------------
    void draw(Camera& camera);
    void draw();

    //--------------------------------------------------------------------------
    //! \brief Traverse the scene and call onDisable() on each node. Then all
    //! nodes are destroyed. The root node is set to nullptr.
    //--------------------------------------------------------------------------
    void release();

private:

    void applyViewPort(Camera& camera);
    Node* findChild(Node* node, std::string const& path);

public:

    // Root of the scene
    Node::Ptr root = nullptr;

    // TODO
    // OverrideMaterial
    // active camera
    // fog
};

//------------------------------------------------------------------------------
//! \brief Alias name
//------------------------------------------------------------------------------
using SceneObject = SceneTree::Node;

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_SCENE_TREE_HPP
