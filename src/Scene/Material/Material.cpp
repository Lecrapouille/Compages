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

#include "Scene/Material/Material.hpp"

//------------------------------------------------------------------------------
bool Material::compile()
{
    if (program.compiled())
        return true;

    // Generate shaders
    m_vert_shader.clear();
    m_frag_shader.clear();
    generate(m_vert_shader, m_frag_shader);

    // Debug
    // std::cout << m_vert_shader << std::endl;
    // std::cout << m_frag_shader << std::endl;

    // Compile generated shaders
    if (!program.compile(m_vert_shader, m_frag_shader))
    {
        std::cerr << "Failed compiling Material " << name() << ". Reason was '"
                  << program.strerror() << "'" << std::endl;
        return false;
    }

    // Init variables of generated shaders
    init();

    return true;
}
