// Original code source
// https://stackoverflow.com/questions/44105058/how-does-unitys-getcomponent-work
// Quote: "I'm wondering how the GetComponent() method that Unity implements
// works."

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_COMPONENTS_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_COMPONENTS_HPP

#include "Common/NonCppStd.hpp"
#include <algorithm> // find_if
#include <stdexcept> // std::out_of_range
#include <vector>

// *****************************************************************************
//! \brief Base class for Component
// *****************************************************************************
class Component
{
public:

    // -------------------------------------------------------------------------
    virtual ~Component() = default;

    // -------------------------------------------------------------------------
    //! \brief Emulate RTTI
    // -------------------------------------------------------------------------
    virtual bool isClassType(const std::size_t classType) const
    {
        return classType == Type;
    }

public:

    static constexpr std::size_t Type = "Component"_hash;
};

// *****************************************************************************
//! \brief Container for Components
// *****************************************************************************
class Components
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create and store a new component.
    //!
    //! \tparam ComponentType the type of the component (shall derive from \c
    //! Component).
    //! \param params list of param for creating the component.
    //! \return the reference of the newly created component.
    // -------------------------------------------------------------------------
    template <class ComponentType, typename... Args>
    ComponentType& addComponent(Args&&... params)
    {
        m_components.emplace_back(
            std::make_unique<ComponentType>(std::forward<Args>(params)...));
        return *static_cast<ComponentType*>(m_components.back().get());
    }

    // -------------------------------------------------------------------------
    //! \brief Find and return the first component of given type (including if
    //! ancestor are of the given type).
    //!
    //! \throw std::out_of_range if no component has been found.
    //! \return the reference of the component.
    // -------------------------------------------------------------------------
    template <class ComponentType>
    ComponentType& getComponent()
    {
        for (auto&& component : m_components)
        {
            if (component->isClassType(ComponentType::Type))
                return *static_cast<ComponentType*>(component.get());
        }

        // return *std::unique_ptr<ComponentType>(nullptr);
        throw std::out_of_range("No component found");
    }

    // -------------------------------------------------------------------------
    //! \brief Remove the component of given type (including if ancestor are of
    //! the given type).
    //! \return true if one component has been found, else return false.
    // -------------------------------------------------------------------------
    template <class ComponentType>
    bool removeComponent()
    {
        if (m_components.empty())
            return false;

        auto index = std::find_if(
            m_components.begin(),
            m_components.end(),
            [](std::unique_ptr<Component>& component)
            { return component->isClassType(ComponentType::Type); });

        bool success = index != m_components.end();
        if (success)
            m_components.erase(index);

        return success;
    }

    // -------------------------------------------------------------------------
    //! \brief Return the list of components of given type (including if
    //! ancestor are of the given type).
    //! \return the std::vector of components (may be empty).
    // -------------------------------------------------------------------------
    template <class ComponentType>
    std::vector<ComponentType*> getComponents()
    {
        std::vector<ComponentType*> componentsOfType;

        for (auto&& component : m_components)
        {
            if (component->isClassType(ComponentType::Type))
                componentsOfType.emplace_back(
                    static_cast<ComponentType*>(component.get()));
        }

        return componentsOfType;
    }

    // -------------------------------------------------------------------------
    //! \brief Remove all components of given type (including if ancestor are
    //! of the given type).
    //!
    //! \note complexity is O(n^2) where n is the number of elements. Indeed the
    //! container is a liast so he search takes O(n) and removing inside a list
    //! is also O(n).
    //!
    //! \return the number of components removed.
    // -------------------------------------------------------------------------
    template <class ComponentType>
    size_t removeComponents()
    {
        bool success = false;
        size_t numRemoved = 0u;

        if (m_components.empty())
            return numRemoved;

        do
        {
            auto index = std::find_if(
                m_components.begin(),
                m_components.end(),
                [](std::unique_ptr<Component>& component)
                { return component->isClassType(ComponentType::Type); });

            success = (index != m_components.end());
            if (success)
            {
                m_components.erase(index);
                ++numRemoved;
            }
        } while (success);

        return numRemoved;
    }

    // -------------------------------------------------------------------------
    //! \brief Erase all components inside the container.
    // -------------------------------------------------------------------------
    void clear()
    {
        m_components.clear();
    }

    // -------------------------------------------------------------------------
    //! \brief Return the number of components hold by this container.
    // -------------------------------------------------------------------------
    size_t countComponents() const
    {
        return m_components.size();
    }

private:

    //! \brief Container of components
    std::vector<std::unique_ptr<Component>> m_components;
};

#endif // OPENGLCPPWRAPPER_SCENEGRAPH_COMPONENTS_HPP
