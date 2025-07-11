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

#ifndef GENERAL_TREE_HPP
#define GENERAL_TREE_HPP

#include <iostream>
#include <memory>
#include <vector>

//------------------------------------------------------------------------------
//! \brief General Tree class. This class is made to allow derived class to
//! access to parent and child nodes. This is easier to create scene graph and
//! contrary to std container data has to be mixed with internal states.
//! If this is not desired you can see other implementations:
//! -
//! https://github.com/reconndev/General-Tree/blob/master/GeneralTree/GeneralTree.hpp
//! - https://gist.github.com/phoemur/6dd18d608438373185f6a2457662c1c2
//! - https://github.com/taskflow/taskflow/blob/master/taskflow/core/graph.hpp
//! \tparam T derived class deriving from Tree.
//------------------------------------------------------------------------------
template <class T>
class Tree
{
public:

    //--------------------------------------------------------------------------
    //! \brief Use as T::Ptr
    //--------------------------------------------------------------------------
    using Ptr = std::unique_ptr<T>;

    //--------------------------------------------------------------------------
    //! \brief Create a new T as smart pointer. Use insert() to add it in the
    //! tree.
    //--------------------------------------------------------------------------
    template <class X, typename... ArgsT>
    static std::unique_ptr<X> create(ArgsT&&... args)
    {
        return std::make_unique<X>(std::forward<ArgsT>(args)...);
    }

public:

    Tree<T>() = default;
    Tree<T>(const Tree<T>&) = delete;
    Tree<T>& operator=(const Tree<T>&) = delete;

    //--------------------------------------------------------------------------
    //! \brief Release this node and its child nodes.
    //--------------------------------------------------------------------------
    virtual ~Tree<T>()
    {
        // std::cout << "Bye " << static_cast<T*>(this)->name() << std::endl;
        //  Avoid using implicit recursive deletion due to usage of smart
        //  pointer
        clear(); // FIXME ?
    }

    //--------------------------------------------------------------------------
    //! \brief Create and store a new tree node.
    //! \param[in] args parameters to the tree node constructor.
    //! \return the reference to the newly created tree node.
    //--------------------------------------------------------------------------
    template <class X, typename... ArgsT>
    X& attach(ArgsT&&... args)
    {
        children.push_back(create<X>(std::forward<ArgsT>(args)...));
        children.back()->parent = static_cast<T*>(this);
        onNodeAdded();
        return *static_cast<X*>(children.back().get());
    }

    //--------------------------------------------------------------------------
    //! \brief Insert a new node. The element is moved so no longer be used.
    //! \return the index of the object.
    //--------------------------------------------------------------------------
    size_t attach(Ptr node)
    {
        node->parent = static_cast<T*>(this);
        children.push_back(std::move(node));
        onNodeAdded();
        return children.size() - 1u;
    }

    //--------------------------------------------------------------------------
    //! \brief Insert a new node. The element is moved so no longer be used.
    //! \return the index of the object.
    //--------------------------------------------------------------------------
    size_t detach(Ptr node)
    {
        auto it = std::find_if(children.begin(),
                               children.end(),
                               [node](Ptr i) { return i.get() == node.get(); });

        if (it != children.end())
        {
            children.erase(it);
            onNodeRemoved();
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Getter to the nth child. Do not throw an exception if the index
    //! is odd. For throw method, use instead child(i).
    //! \return the reference to the nth child.
    //--------------------------------------------------------------------------
    T& operator[](size_t const i)
    {
        return *children[i];
    }

    T const& operator[](size_t const i) const
    {
        return *children[i];
    }

    //--------------------------------------------------------------------------
    //! \brief Return the root of the tree.
    //! \note this algorithm is O(n) where n is the depth of the node.
    //! \return the reference of the tree root.
    //--------------------------------------------------------------------------
    T& root()
    {
        T* n = static_cast<T*>(this);
        while (n->parent != nullptr)
            n = n->parent;
        return *n;
    }

    //--------------------------------------------------------------------------
    //! \brief Clear the tree in a not recursive way to avoid stack overflow.
    //! \return the number of deleted nodes.
    //--------------------------------------------------------------------------
    size_t clear()
    {
        size_t removed = 0u;
        Tree* current = this;

        while ((current != this) || (!this->children.empty()))
        {
            if (!current->children.empty())
            {
                current = current->children.back().get();
            }
            else
            {
                current = current->parent;
                current->children.pop_back();
                ++removed;
            }
        }
        ++removed;
        return removed;
    }

    //--------------------------------------------------------------------------
    //! \brief Traverse the tree and call a function on each node
    //! \note Recursive method.
    //! \param[in] functor lambda function to call on each node.
    //! \param[in] args extra parameters of the lambda function.
    //--------------------------------------------------------------------------
    template <typename Functor, typename... ArgsT>
    void traverse(Functor functor, ArgsT&&... args)
    {
        functor(static_cast<T*>(this), std::forward<ArgsT>(args)...);

        for (auto& child : children)
        {
            child->traverse(functor, std::forward<ArgsT>(args)...);
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Const Traverse the tree and call a function on each node
    //! \note Recursive method.
    //! \param[in] functor lambda function to call on each node.
    //! \param[in] args extra parameters of the lambda function.
    //--------------------------------------------------------------------------
    template <typename Functor, typename... ArgsT>
    void traverse(Functor functor, ArgsT&&... args) const
    {
        functor(static_cast<const T*>(this), std::forward<ArgsT>(args)...);

        for (auto& child : children)
        {
            child->traverse(functor, std::forward<ArgsT>(args)...);
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Return the number of elements sotred in the tree.
    //! \note time is O(n) where n is the number of elements.
    //--------------------------------------------------------------------------
    inline std::size_t size() const
    {
        size_t count = 0u;
        Tree<T>::traverse([](T const* node, size_t& c)
                          { c += node->children.size(); },
                          count);
        return count + 1u;
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if the tree is empty.
    //--------------------------------------------------------------------------
    inline bool empty() const
    {
        return size() == 0u;
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Callback when a node has been added. By default do nothing but
    //! derived class can override this method.
    //!
    //! \param node newly inserted node.
    //--------------------------------------------------------------------------
    virtual void onNodeAdded() {}

    //--------------------------------------------------------------------------
    //! \brief Callback when a node has been removed. By default do nothing but
    //! derived class can override this method.
    //--------------------------------------------------------------------------
    virtual void onNodeRemoved() {}

public:

    //! \brief Access to the parent node. Needed for scene graph to get parent
    //! transform matrix.
    T* parent = nullptr;
    //! \brief Child nodes.
    std::vector<std::unique_ptr<T>> children;
};

#endif
