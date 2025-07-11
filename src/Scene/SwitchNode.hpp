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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_SWITCH_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_SWITCH_HPP

#include "Scene/SceneTree.hpp"

// *****************************************************************************
//! \brief A special node scene allowing to select a single child allowed to be
//! traversed: other children are ignored. A SwitchNode3D is not renderable.
//! \note this node has been inspired by OpenInventor soSwitch
//! https://grey.colorado.edu/coin3d/classSoSwitch.html
// *****************************************************************************
class SwitchNode: public SceneObject
{
public:

    //--------------------------------------------------------------------------
    //! \brief Create a switch node with a given name.
    //!
    //! \param name the name of the node. It is used mainly for the debug.
    //--------------------------------------------------------------------------
    SwitchNode(std::string const& name) : SceneObject(name) {}

public:

    //--------------------------------------------------------------------------
    //! \brief Select the desired child.
    //!
    //! The desired child is traversable and other are no longer traversable.
    //!
    //! \param nth the number of the desired child.
    //!
    //! \note if nth is greater than the number of child. The nth is
    //! memorized but the effective index is modulo by the number of children.
    //--------------------------------------------------------------------------
    void select(size_t const nth)
    {
        if (m_designated == nth)
            return;

        m_designated = nth;
        designated2effective();
        updateEnabledChild();
    }

    //--------------------------------------------------------------------------
    //! \brief Return the effective child.
    //--------------------------------------------------------------------------
    size_t selected() const
    {
        return m_selected;
    }

    //--------------------------------------------------------------------------
    //! \brief Switch to the next child. Start to the first if the last was
    //! used.
    //--------------------------------------------------------------------------
    void next()
    {
        m_designated = (m_designated + 1_z) % children.size();
        m_selected = m_designated;
        updateEnabledChild();
    }

    //--------------------------------------------------------------------------
    //! \brief Switch to the previous child. Start to the last if the first was
    //! used.
    //--------------------------------------------------------------------------
    void previous()
    {
        if (unlikely(m_designated == 0_z))
            m_designated = children.size();
        --m_designated;
        m_selected = m_designated;
        updateEnabledChild();
    }

    //--------------------------------------------------------------------------
    //! \brief Call Node3D::debug() and show additional information.
    //--------------------------------------------------------------------------
    inline friend std::ostream& operator<<(std::ostream& os, SwitchNode& node)
    {
        return os << *static_cast<Node*>(&node) << "Switched to child "
                  << node.m_selected << ": "
                  << node.children[node.m_selected]->name() << std::endl;
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Make the desired child traversable and the others not
    //! traversable.
    //--------------------------------------------------------------------------
    void updateEnabledChild()
    {
        for (auto& it : children)
            it->enable(false);

        children[m_selected]->enable(true);
    }

    //--------------------------------------------------------------------------
    //! \brief Callback when a child has been added. Update the traversable
    //! status for all children.
    //--------------------------------------------------------------------------
    virtual void onNodeAdded() override
    {
        updateEnabledChild();
    }

    //--------------------------------------------------------------------------
    //! \brief Callback when a child has been removed. Update the traversable
    //! status for all children.
    //--------------------------------------------------------------------------
    virtual void onNodeRemoved() override
    {
        designated2effective();
        updateEnabledChild();
    }

    //--------------------------------------------------------------------------
    //! \brief Conver the designated child by the user to the real effective
    //! selected. Indeed the user may want to select a node not present or when
    //! removing a child invalidating the selected child. To avoid an out of
    //! bound error we compute the modulo of the number of children.
    //--------------------------------------------------------------------------
    inline void designated2effective()
    {
        // m_selected = std::min(m_designated, children.size() - 1_z);
        m_selected = m_designated % children.size();
    }

protected:

    //! \brief Child chosen by the user (index of the std::vector).
    size_t m_designated = 0u;
    //! \brief Chosen child converted to a valid std::vector index.
    size_t m_selected = 0u;
};

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_SWITCH_HPP
