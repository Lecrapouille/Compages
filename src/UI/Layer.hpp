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

#ifndef OPENGLCPPWRAPPER_UI_LAYER_HPP
#define OPENGLCPPWRAPPER_UI_LAYER_HPP

#include "UI/Window.hpp"
#include <string>

// ***************************************************************************
//! \brief A layer allows to render
// ***************************************************************************
class Layer
{
    friend GLWindow;

public:

    //--------------------------------------------------------------------------
    //! \brief Default constructor passing the parent window that will manage
    //! this instance of layer.
    //! \param[in] name: the layer name.
    //--------------------------------------------------------------------------
    Layer(GLWindow& window, std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Needed because of virtual methods.
    //--------------------------------------------------------------------------
    virtual ~Layer() = default;

    //--------------------------------------------------------------------------
    //! \brief Return the layer name.
    //--------------------------------------------------------------------------
    inline std::string const& name() const
    {
        return m_name;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the parent window (the window owning this instance of
    //! layer).
    //--------------------------------------------------------------------------
    template <class W>
    inline W& owner()
    {
        return reinterpret_cast<W&>(m_window);
    }

private:

    //! \note Do not create/delete layers from m_window from this method since
    //! m_window is iterating on layers.
    virtual bool onSetup() = 0;
    //! \note Do not create/delete layers from m_window from this method since
    //! m_window is iterating on layers.
    virtual bool onPaint() = 0;
    //! \note Do not delete layers from m_window from this method since m_window
    //! is iterating on layers.
    virtual bool onRelease() = 0;

protected:

    GLWindow& m_window; // FIXME template ? mais pas de stockage depuis GLWindow

private:

    std::string m_name;
};

#endif // OPENGLCPPWRAPPER_UI_LAYER_HPP
