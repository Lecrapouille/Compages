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

#ifndef BASIC_MATERIAL_HPP
#define BASIC_MATERIAL_HPP

#include "OpenGL/Textures/Textures.hpp"
#include "Scene/Material/Color.hpp"
#include "Scene/Material/Material.hpp"

class BasicMaterial: public Material
{
public:

    struct Config
    {
        enum class Fog
        {
            None,
            Linear,
            Exponential
        };
        enum class Light
        {
            None,
            Phong,
            Lambert
        };
        Config() {}

        //! \brief If true then allow to drop fragment colors if lower than a
        //! given threshold.
        bool useAlphaTest = false;
        //! \brief If true then taken into account gamma correction.
        bool useGammaInput = false;
        //! \brief If true then taken into account gamma correction.
        bool useGammaOutput = false;
        //! \brief
        bool useLightMap = false;
        Light useLight = Light::None;
        //! \brief
        bool useEnvMap = false;
        //! \brief If true then apply a texture to the object.
        bool useMap = false;
        //! \brief If true then apply a bump mapping texture to the object.
        bool useBumpMap = false;
        //! \brief If true then simulate the bright spot of a light that appears
        //! on shiny objects. Specular highlights are often more inclined to the
        //! color of the light than the color of the object.
        bool useSpecularMap = false;
        //! \brief If true then apply a color to the object.
        bool useColor = true;
        //! \brief If true then apply a linear fog. Disabled if useExpFog is set
        //! to true.
        Fog useFog = Fog::None;
        bool useSkinning = false;
        bool useBoneTexture = false;
        bool useMorphTargets = false;
        bool useShadowMap = false;
    };

public:

    BasicMaterial(GLVAO& vao,
                  BasicMaterial::Config const& conf = BasicMaterial::Config())
        : Material("BasicMaterial", vao), config(conf)
    {
    }

    inline Vector3f& diffuse()
    {
        return program.vector3f("diffuse");
    }

    inline float& opacity()
    {
        return program.scalarf("opacity");
    }

    inline Vector3f& color()
    {
        return program.vector3f("color");
    }

    inline float& alphaTest()
    {
        return program.scalarf("ALPHATEST");
    }

    GLTexture2D& texture()
    {
        return m_vao.texture2D("texture");
    }

    inline Vector4f& offsetTexture()
    {
        return program.vector4f("offsetRepeat");
    }

    inline float& fogDensity()
    {
        return program.scalarf("fogDensity");
    }

    inline float& fogNear()
    {
        return program.scalarf("fogNear");
    }

    inline float& fogFar()
    {
        return program.scalarf("fogFar");
    }

    inline Vector3f& fogColor()
    {
        return program.vector3f("fogColor");
    }

private:

    virtual void generate(GLVertexShader& vertexShader,
                          GLFragmentShader& fragmentShader) override;

    virtual void init() override;

public:

    Config config;
};

#endif
