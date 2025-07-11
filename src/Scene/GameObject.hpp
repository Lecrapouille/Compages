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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_GAME_OBJECT_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_GAME_OBJECT_HPP

#include "Common/ClassCounter.hpp"
#include "Components/Components.hpp"
#include "Scene/Behavior.hpp"

//------------------------------------------------------------------------------
//! \brief Base and abstract GameObject class specialized for game.
//------------------------------------------------------------------------------
class GameObject: private UniqueID<GameObject>,
                  private InstanceCounter<GameObject>,
                  public Behavior
{
public:

    //--------------------------------------------------------------------------
    //! \brief Set the name of the GameObject and generate an unique identifier.
    //--------------------------------------------------------------------------
    GameObject(std::string const& name /* TODO: components*/)
        : m_id(UniqueID<GameObject>::getID()), m_name(name)
    {
    }

    GameObject(std::string const& name, std::string const& tag_)
        : m_id(UniqueID<GameObject>::getID()), m_name(name)
    {
        tag = tag_;
    }

    //--------------------------------------------------------------------------
    //! \brief Generate an unique identifier and set the default name
    //--------------------------------------------------------------------------
    GameObject()
        : m_id(UniqueID<GameObject>::getID()),
          m_name("GameObject" + std::to_string(m_id))
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Return the object name. Should be unique.
    //--------------------------------------------------------------------------
    std::string const& name() const
    {
        return m_name;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    //--------------------------------------------------------------------------
    Key id() const
    {
        return m_id;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the number of created Objects.
    //--------------------------------------------------------------------------
    static Key count()
    {
        return InstanceCounter<GameObject>::count();
    }

    //--------------------------------------------------------------------------
    //! \brief Enable (true) or disable (false) a node.
    //! If disabled, the node is no longer drawn, updated ...
    //--------------------------------------------------------------------------
    void enable(bool const v = true)
    {
        m_enabled = v;
        v ? onEnable() : onDisable();
    }

    //--------------------------------------------------------------------------
    //! \brief Return if the node is enabled (true) or disabled (false).
    //--------------------------------------------------------------------------
    bool enabled() const
    {
        return m_enabled;
    }

public:

    //! \brief Extend the class by adding new behaviors.
    Components components;
    //! \brief A tag is a reference word which you can assign to one or more
    //! Objects.
    std::string tag;

private:

    //! \brief Unique identifier. Used for searching a node from the root node
    //! of the scene.
    Key m_id;
    //! \brief Name of the object. Used for searching a node from the root node
    //! of the scene.
    std::string m_name;
    //! \brief If disabled, the node is no longer drawn, updated ...
    bool m_enabled = true;
};

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_GAME_OBJECT_HPP
