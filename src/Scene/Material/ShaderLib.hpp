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

#ifndef OPENGLCPPWRAPPER_SHADERLIB_HPP
#define OPENGLCPPWRAPPER_SHADERLIB_HPP

//----------------------------------------------------------------------------
//! \file This code source is largely inspired by the file
//! three_cpp/three/renderers/impl/gl_shaders.ipp of the project
//! https://github.com/jdduke/three_cpp
//----------------------------------------------------------------------------

#include "Scene/Material/MeshBasicMaterial.hpp"
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define UV "uv"
#define POSITION "position"
#define NORMAL "normal"
#define COLOR "color"

namespace shaders
{
//==========================================================================
namespace name
{
static const char* uv = UV;
static const char* position = POSITION;
static const char* normal = NORMAL;
static const char* color = COLOR;
} // namespace name

//==========================================================================
namespace common
{
static const char* version()
{
#if defined(GLES)
    return "precision highp float;\n"
           "#version 330\n\n";
#elif defined(__APPLE__)
    return "#version 330\n\n";
#else
    return "#version 330\n\n";
#endif
}

static const char* constants()
{
    return "// Constants\n"
           "const float LOG2 = 1.442695;\n\n";
}

//----------------------------------------------------------------------
namespace vertex
{
static const char* params()
{
    return "// Inputs\n"
           "in vec3 " POSITION ";\n"
           "in vec3 " NORMAL ";\n\n";
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params()
{
    return "// Outputs\n"
           "out vec4 FragColor;\n\n";
}
} // namespace fragment
} // namespace common

//==========================================================================
namespace mvp
{
namespace vertex
{
static const char* params()
{
    return "// Model-View-Projection matrices\n"
           "uniform mat4 modelMatrix;\n"
           "uniform mat4 viewMatrix;\n"
           "uniform mat4 projectionMatrix;\n"
           "uniform vec3 cameraPosition;\n\n";
}
} // namespace vertex

static const char* mvp_code()
{
    return "projectionMatrix * viewMatrix * modelMatrix"
           " * vec4(" POSITION ", 1.0);\n";
}

static const char* mv_code()
{
    return "vec4 mvPosition = viewMatrix * modelMatrix * vec4(" POSITION
           ", 1.0);\n";
}

//----------------------------------------------------------------------
namespace fragment
{
static const char* params()
{
    return "// Model-View-Projection matrices\n"
           "uniform mat4 viewMatrix;\n"
           "uniform vec3 cameraPosition;\n\n";
}
} // namespace fragment
} // namespace mvp

//==========================================================================
namespace fog
{
namespace fragment
{
//------------------------------------------------------------------
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useFog == BasicMaterial::Config::Fog::Linear)
        return "// Linear Fog\n"
               "uniform vec3 fogColor;\n"
               "uniform float fogNear;\n"
               "uniform float fogFar;\n\n";

    if (config.useFog == BasicMaterial::Config::Fog::Exponential)
        return "// Exponential Fog\n"
               "uniform vec3 fogColor;\n"
               "uniform float fogDensity;\n\n";

    return "";
}

//------------------------------------------------------------------
static const char* code(BasicMaterial::Config const& config)
{
    if (config.useFog == BasicMaterial::Config::Fog::Linear)
        return "  // Linear Fog\n"
               "  float depth = gl_FragCoord.z / gl_FragCoord.w;\n"
               "  float fogFactor = smoothstep(fogNear, fogFar, depth);\n"
               "  FragColor = mix(FragColor, vec4(fogColor, FragColor.w), "
               "fogFactor);\n";

    if (config.useFog == BasicMaterial::Config::Fog::Exponential)
        return "  // Exponential Fog\n"
               "  float depth = gl_FragCoord.z / gl_FragCoord.w;\n"
               "  float fogFactor = exp2(-fogDensity * fogDensity * depth * "
               "depth * LOG2);\n"
               "  fogFactor = 1.0 - clamp(fogFactor, 0.0, 1.0);\n"
               "  FragColor = mix(FragColor, vec4(fogColor, FragColor.w), "
               "fogFactor);\n";

    return "";
}
} // namespace fragment
} // namespace fog

//==========================================================================
namespace envmap
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if ((config.useEnvMap) && (!config.useBumpMap))
        return "// Environement mapping\n"
               "out vec3 vReflect;\n"
               "uniform float refractionRatio;\n"
               "uniform bool useRefract;\n\n";

    return "";
}

static std::stringstream code(BasicMaterial::Config const& config)
{
    std::stringstream ss;

    if (config.useEnvMap)
    {
        ss << "vec4 mPosition = modelMatrix * vec4(position, 1.0);\n";
    }

    if ((config.useEnvMap) && (!config.useBumpMap))
    {
        ss << "  // Environement mapping\n"
              "  vec3 nWorld = mat3(modelMatrix[0].xyz, modelMatrix[1].xyz, "
              "modelMatrix[2].xyz) * normal;\n"
              "  if (useRefract)\n{\n"
              "    vReflect = refract(normalize(mPosition.xyz - "
              "cameraPosition), normalize(nWorld.xyz), refractionRatio);\n"
              "  }\nelse\n{\n"
              "    vReflect = reflect(normalize(mPosition.xyz - "
              "cameraPosition), normalize(nWorld.xyz));\n"
              "  }\n\n";
    }

    return ss;
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useEnvMap)
    {
        if (config.useBumpMap)
            return "// Environement mapping + bump mapping\n"
                   "uniform float reflectivity;\n"
                   "uniform samplerCube envMap;\n"
                   "uniform float flipEnvMap;\n"
                   "uniform int combine;\n"
                   "uniform bool useRefract;\n"
                   "uniform float refractionRatio;\n";

        return "// Environement mapping without bump mapping\n"
               "uniform float reflectivity;\n"
               "uniform samplerCube envMap;\n"
               "uniform float flipEnvMap;\n"
               "uniform int combine;\n"
               "varying vec3 vReflect;\n";
    }

    return "";
}

static std::stringstream code(BasicMaterial::Config const& config)
{
    std::stringstream ss;

    if (!config.useEnvMap)
        return ss;

    if (!config.useBumpMap)
    {
        ss << "  // Environement mapping without bump mapping\n"
              "  reflectVec = vReflect;\n";
    }
    else
    {
        ss << "  // Environement mapping + bump mapping\n"
              "  vec3 reflectVec;\n"
              "  vec3 cameraToVertex = normalize(vWorldPosition - "
              "cameraPosition);\n"
              "  if (useRefract) {\n"
              "    reflectVec = refract(cameraToVertex, normal, "
              "refractionRatio);\n"
              "  } else {\n"
              "    reflectVec = reflect(cameraToVertex, normal);\n"
              "  }\n";
    }

    ss << "  #ifdef DOUBLE_SIDED\n"
          "    float flipNormal = (-1.0 + 2.0 * float( gl_FrontFacing));\n"
          "    vec4 cubeColor = textureCube(envMap, flipNormal * vec3( "
          "flipEnvMap * reflectVec.x, reflectVec.yz));\n"
          "  #else\n"
          "    vec4 cubeColor = textureCube(envMap, vec3(flipEnvMap * "
          "reflectVec.x, reflectVec.yz));\n"
          "  #endif\n";

    if (config.useGammaInput)
    {
        ss << "cubeColor.xyz *= cubeColor.xyz;\n";
    }

    ss << "  if (combine == 1) {\n"
          "    gl_FragColor.xyz = mix( gl_FragColor.xyz, cubeColor.xyz, "
          "specularStrength * reflectivity );\n"
          "  } else {\n"
          "    gl_FragColor.xyz = mix( gl_FragColor.xyz, gl_FragColor.xyz * "
          "cubeColor.xyz, specularStrength * reflectivity );\n"
          "  }\n\n";

    return ss;
}
} // namespace fragment
} // namespace envmap

//==========================================================================
namespace texture
{
namespace particles
{
//------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useMap)
        return "// Texture for particules\n"
               "uniform sampler2D map;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useMap)
        return "  // Texture for particules\n"
               "  gl_FragColor = gl_FragColor * texture2D(map, "
               "vec2(gl_PointCoord.x, 1.0 - gl_PointCoord.y));\n\n";
    return "";
}
} // namespace fragment
} // namespace particles

namespace triangles
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if ((config.useMap) || (config.useBumpMap) || (config.useSpecularMap))
        return "// Texture for meshes\n"
               "uniform vec4 offsetRepeat;\n"
               "in vec2 " UV ";\n"
               "out vec2 vUV;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if ((config.useMap) || (config.useBumpMap) || (config.useSpecularMap))
        return "  // Texture for meshes\n"
               "  vUV = " UV "* offsetRepeat.zw + offsetRepeat.xy;\n";
    return "";
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if ((config.useBumpMap) || (config.useSpecularMap))
        return "// Texture for meshes\n"
               "in vec2 vUV;\n\n";

    if (config.useMap)
        return "// Texture for meshes\n"
               "in vec2 vUV;\n"
               "uniform sampler2D map;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useMap)
    {
        if (config.useGammaInput)
            return "  // Texture for meshes + Gamma input\n"
                   "  vec4 texelColor = texture2D(texture, vUV);\n"
                   "  texelColor.xyz *= texelColor.xyz;\n"
                   "  FragColor = FragColor * texelColor;\n";
        return "  // Texture\n"
               "  FragColor = FragColor * texture2D(texture, vUV);\n";
    }
    return "";
}
} // namespace fragment
} // namespace triangles
} // namespace texture

//==========================================================================
namespace lightmap
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useLightMap)
        return "// Lightmap\n"
               "in vec2 UV2;\n"
               "out vec2 vUV2;\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useLightMap)
        return "  // Lightmap\n"
               "  vUV2 = UV2;\n\n";
    return "";
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useLightMap)
        return "// Lightmap\n"
               "in vec2 vUV2;\n"
               "uniform sampler2D lightMap;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useLightMap)
        return "  // Lightmap\n"
               "  gl_FragColor = gl_FragColor * texture2D(lightMap, vUV2);\n\n";
    return "";
}
} // namespace fragment
} // namespace lightmap

//==========================================================================
namespace bumpmap
{
//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    // Derivative maps - bump mapping unparametrized surfaces by
    // Morten Mikkelsen:
    //   http://mmikkelsen3d.blogspot.sk/2011/07/derivative-maps.html
    // Evaluate the derivative of the height w.r.t. screen-space
    // using forward differencing (listing 2)
    if (config.useBumpMap)
        return "// Bump mapping\n"
               "uniform sampler2D bumpMap;\n"
               "uniform float bumpScale;\n"
               "vec2 dHdxy_fwd()\n{\n"
               "  vec2 dSTdx = dFdx(vUv);\n"
               "  vec2 dSTdy = dFdy(vUv);\n"
               "  float Hll = bumpScale * texture2D(bumpMap, vUv).x;\n"
               "  float dBx = bumpScale * texture2D(bumpMap, vUv + dSTdx).x - "
               "Hll;\n"
               "  float dBy = bumpScale * texture2D(bumpMap, vUv + dSTdy).x - "
               "Hll;\n"
               "  return vec2(dBx, dBy);\n"
               "}\n\n"
               "vec3 perturbNormalArb(vec3 surf_pos, vec3 surf_norm vec2 "
               "dHdxy)\n{\n"
               "  vec3 vSigmaX = dFdx(surf_pos);\n"
               "  vec3 vSigmaY = dFdy(surf_pos);\n"
               "  vec3 vN = surf_norm; // normalized\n"
               "  vec3 R1 = cross(vSigmaY, vN);\n"
               "  vec3 R2 = cross(vN, vSigmaX);\n"
               "  float fDet = dot(vSigmaX, R1);\n"
               "  vec3 vGrad = sign(fDet) * (dHdxy.x * R1 + dHdxy.y * R2);\n"
               "  return normalize(abs(fDet) * surf_norm - vGrad);\n"
               "}\n\n";
    return "";
}
} // namespace fragment
} // namespace bumpmap

//==========================================================================
namespace specularmap
{
//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useSpecularMap)
        return "// Specular\n"
               "uniform sampler2D specularMap;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useSpecularMap)
        return "  // Specular map\n\n"
               "  float specularStrength;\n"
               "  vec4 texelSpecular = texture2D(specularMap, vUv);\n"
               "  specularStrength = texelSpecular.r;\n";

    return "  // Specular\n\n"
           "  float specularStrength;\n"
           "  specularStrength = 1.0;\n\n";
}
} // namespace fragment
} // namespace specularmap

//==========================================================================
namespace lightlambert
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useLight == BasicMaterial::Config::Light::Lambert)
        return "// Light Lambert\n"
               "uniform vec3 ambient;\n"
               "uniform vec3 diffuse;\n"
               "uniform vec3 emissive;\n"
               "uniform vec3 ambientLightColor;\n"

               "#if MAX_DIR_LIGHTS > 0\n"
               "uniform vec3 directionalLightColor[MAX_DIR_LIGHTS];\n"
               "uniform vec3 directionalLightDirection[MAX_DIR_LIGHTS];\n"
               "#endif\n"

               "#if MAX_POINT_LIGHTS > 0\n"
               "uniform vec3 pointLightColor[MAX_POINT_LIGHTS];\n"
               "uniform vec3 pointLightPosition[MAX_POINT_LIGHTS];\n"
               "uniform float pointLightDistance[MAX_POINT_LIGHTS];\n"
               "#endif\n"

               "#if MAX_SPOT_LIGHTS > 0\n"
               "uniform vec3 spotLightColor[MAX_SPOT_LIGHTS];\n"
               "uniform vec3 spotLightPosition[MAX_SPOT_LIGHTS];\n"
               "uniform vec3 spotLightDirection[MAX_SPOT_LIGHTS];\n"
               "uniform float spotLightDistance[MAX_SPOT_LIGHTS];\n"
               "uniform float spotLightAngle[MAX_SPOT_LIGHTS];\n"
               "uniform float spotLightExponent[MAX_SPOT_LIGHTS];\n"
               "#endif\n"

               "#ifdef WRAP_AROUND\n"
               "uniform vec3 wrapRGB;\n"
               "#endif\n";

    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useLight == BasicMaterial::Config::Light::Lambert)
        return "  // Light Lambert\n"
               "  vLightFront = vec3(0.0);\n"
               "  #ifdef DOUBLE_SIDED\n"
               "  vLightBack = vec3(0.0);\n"
               "  #endif\n\n"
               "  transformedNormal = normalize(transformedNormal);\n"
               "  #if MAX_DIR_LIGHTS > 0\n"
               "  for (int i = 0; i < MAX_DIR_LIGHTS; i ++) {\n"
               "    vec4 lDirection = viewMatrix * "
               "vec4(directionalLightDirection[i], 0.0);\n"
               "    vec3 dirVector = normalize(lDirection.xyz);\n"
               "    float dotProduct = dot(transformedNormal, dirVector);\n"
               "    vec3 directionalLightWeighting = vec3(max(dotProduct, "
               "0.0));\n"
               "    #ifdef DOUBLE_SIDED\n"
               "    vec3 directionalLightWeightingBack = vec3(max(-dotProduct, "
               "0.0));\n"
               "    #ifdef WRAP_AROUND\n"
               "    vec3 directionalLightWeightingHalfBack = vec3(max(-0.5 * "
               "dotProduct + 0.5, 0.0));\n"
               "    #endif\n"
               "  #endif\n"

               "#ifdef WRAP_AROUND\n"
               "  vec3 directionalLightWeightingHalf = vec3(max(0.5 * "
               "dotProduct + 0.5, 0.0));\n"
               "  directionalLightWeighting = mix(directionalLightWeighting, "
               "directionalLightWeightingHalf, wrapRGB);\n"
               "#ifdef DOUBLE_SIDED\n"
               "  directionalLightWeightingBack = "
               "mix(directionalLightWeightingBack, "
               "directionalLightWeightingHalfBack, wrapRGB);\n"
               "#endif\n"
               "#endif\n"
               "  vLightFront += directionalLightColor[i] * "
               "directionalLightWeighting;\n"
               "#ifdef DOUBLE_SIDED\n"
               "  vLightBack += directionalLightColor[i] * "
               "directionalLightWeightingBack;\n"
               "#endif\n"
               "}\n"
               "#endif\n"

               "#if MAX_POINT_LIGHTS > 0\n"
               "for(int i = 0; i < MAX_POINT_LIGHTS; i ++) {\n"
               "  vec4 lPosition = viewMatrix * vec4(pointLightPosition[i], "
               "1.0);\n"
               "  vec3 lVector = lPosition.xyz - mvPosition.xyz;\n"
               "  float lDistance = 1.0;\n"
               "  if (pointLightDistance[i] > 0.0)\n"
               "    lDistance = 1.0 - min((length(lVector) / "
               "pointLightDistance[i]), 1.0);\n"
               "  lVector = normalize(lVector);\n"
               "  float dotProduct = dot(transformedNormal, lVector);\n"
               "  vec3 pointLightWeighting = vec3(max(dotProduct, 0.0));\n"
               "#ifdef DOUBLE_SIDED\n"
               "  vec3 pointLightWeightingBack = vec3(max(-dotProduct, 0.0));\n"
               "#ifdef WRAP_AROUND\n"
               "  vec3 pointLightWeightingHalfBack = vec3(max(-0.5 * "
               "dotProduct + 0.5, 0.0));\n"
               "#endif\n"
               "#endif\n"
               "#ifdef WRAP_AROUND\n"
               "  vec3 pointLightWeightingHalf = vec3(max(0.5 * dotProduct + "
               "0.5, 0.0));\n"
               "  pointLightWeighting = mix(pointLightWeighting, "
               "pointLightWeightingHalf, wrapRGB);\n"
               "#ifdef DOUBLE_SIDED\n"
               "  pointLightWeightingBack = mix(pointLightWeightingBack, "
               "pointLightWeightingHalfBack, wrapRGB);\n"
               "#endif\n"
               "#endif\n"
               "  vLightFront += pointLightColor[i] * pointLightWeighting * "
               "lDistance;\n"
               "#ifdef DOUBLE_SIDED\n"
               "  vLightBack += pointLightColor[i] * pointLightWeightingBack * "
               "lDistance;\n"
               "#endif\n"
               "}\n"
               "#endif\n"

               "#if MAX_SPOT_LIGHTS > 0\n"
               "for(int i = 0; i < MAX_SPOT_LIGHTS; i ++) {\n"
               "  vec4 lPosition = viewMatrix * vec4(spotLightPosition[i], "
               "1.0);\n"
               "  vec3 lVector = lPosition.xyz - mvPosition.xyz;\n"
               "  lVector = normalize(lVector);\n"
               "  float spotEffect = dot(spotLightDirection[i], "
               "normalize(spotLightPosition[i] - mPosition.xyz));\n"
               "  if (spotEffect > spotLightAngle[i]) {\n"
               "    spotEffect = pow(spotEffect, spotLightExponent[i]);\n"
               "    float lDistance = 1.0;\n"
               "    if (spotLightDistance[i] > 0.0)\n"
               "      lDistance = 1.0 - min((length(lVector) / "
               "spotLightDistance[i]), 1.0);\n"
               "    float dotProduct = dot(transformedNormal, lVector);\n"
               "    vec3 spotLightWeighting = vec3(max(dotProduct, 0.0));\n"

               "#ifdef DOUBLE_SIDED\n"
               "    vec3 spotLightWeightingBack = vec3(max(-dotProduct, "
               "0.0));\n"
               "#ifdef WRAP_AROUND\n"
               "    vec3 spotLightWeightingHalfBack = vec3(max(-0.5 * "
               "dotProduct + 0.5, 0.0));\n"
               "#endif\n"
               "#endif\n"

               "#ifdef WRAP_AROUND\n"
               "    vec3 spotLightWeightingHalf = vec3(max(0.5 * dotProduct + "
               "0.5, 0.0));\n"
               "    spotLightWeighting = mix(spotLightWeighting, "
               "spotLightWeightingHalf, wrapRGB);\n"
               "#ifdef DOUBLE_SIDED\n"
               "    spotLightWeightingBack = mix(spotLightWeightingBack, "
               "spotLightWeightingHalfBack, wrapRGB);\n"
               "#endif\n"
               "#endif\n"

               "    vLightFront += spotLightColor[i] * spotLightWeighting * "
               "lDistance * spotEffect;\n"
               "#ifdef DOUBLE_SIDED\n"
               "    vLightBack += spotLightColor[i] * spotLightWeightingBack * "
               "lDistance * spotEffect;\n"
               "#endif\n"
               "  }\n"
               "}\n"
               "#endif\n"

               "vLightFront = vLightFront * diffuse + ambient * "
               "ambientLightColor + emissive;\n"
               "#ifdef DOUBLE_SIDED\n"
               "vLightBack = vLightBack * diffuse + ambient * "
               "ambientLightColor + emissive;\n"
               "#endif\n";

    return "";
}
} // namespace vertex
} // namespace lightlambert

//==========================================================================
namespace lightphong
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useLight == BasicMaterial::Config::Light::Phong)
        return "// Light Phong (per pixel)\n"
               "#if MAX_POINT_LIGHTS > 0\n"
               "  uniform vec3 pointLightPosition[MAX_POINT_LIGHTS];\n"
               "  uniform float pointLightDistance[MAX_POINT_LIGHTS];\n"
               "  varying vec4 vPointLight[MAX_POINT_LIGHTS];\n"
               "#endif\n"

               "#if MAX_SPOT_LIGHTS > 0\n"
               "  uniform vec3 spotLightPosition[MAX_SPOT_LIGHTS];\n"
               "  uniform float spotLightDistance[MAX_SPOT_LIGHTS];\n"
               "  varying vec4 vSpotLight[MAX_SPOT_LIGHTS];\n"
               "#endif\n"

               "#if MAX_SPOT_LIGHTS > 0\n" // || defined(USE_BUMPMAP)\n"
               "  varying vec3 vWorldPosition;\n"
               "#endif\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useLight == BasicMaterial::Config::Light::Phong)
        return "// Light Phong (per pixel)\n"
               "  #if MAX_POINT_LIGHTS > 0\n"
               "    for (int i = 0; i < MAX_POINT_LIGHTS; i ++) {\n"
               "      vec4 lPosition = viewMatrix * "
               "vec4(pointLightPosition[i], 1.0);\n"
               "      vec3 lVector = lPosition.xyz - mvPosition.xyz;\n"
               "      float lDistance = 1.0;\n"
               "      if (pointLightDistance[i] > 0.0)\n"
               "        lDistance = 1.0 - min((length(lVector) / "
               "pointLightDistance[i]), 1.0);\n"
               "      vPointLight[i] = vec4(lVector, lDistance);\n"
               "    }\n"
               "  #endif\n"
               "  #if MAX_SPOT_LIGHTS > 0\n"
               "    for (int i = 0; i < MAX_SPOT_LIGHTS; i ++) {\n"
               "      vec4 lPosition = viewMatrix * vec4(spotLightPosition[i], "
               "1.0);\n"
               "      vec3 lVector = lPosition.xyz - mvPosition.xyz;\n"
               "      float lDistance = 1.0;\n"
               "      if (spotLightDistance[i] > 0.0)\n"
               "        lDistance = 1.0 - min((length(lVector) / "
               "spotLightDistance[i]), 1.0);\n"
               "      vSpotLight[i] = vec4(lVector, lDistance);\n"
               "    }\n"
               "  #endif\n"
               "  #if MAX_SPOT_LIGHTS > 0\n" // || defined(USE_BUMPMAP)\n"
               "    vWorldPosition = mPosition.xyz;\n"
               "  #endif\n";
    return "";
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useLight == BasicMaterial::Config::Light::Phong)
        return "// Light Phong (per pixel)\n"
               "uniform vec3 ambientLightColor;\n"
               "#if MAX_DIR_LIGHTS > 0\n"
               "  uniform vec3 directionalLightColor[MAX_DIR_LIGHTS];\n"
               "  uniform vec3 directionalLightDirection[MAX_DIR_LIGHTS];\n"
               "#endif\n"
               "#if MAX_POINT_LIGHTS > 0\n"
               "  uniform vec3 pointLightColor[MAX_POINT_LIGHTS];\n"
               "  uniform vec3 pointLightPosition[MAX_POINT_LIGHTS];\n"
               "  uniform float pointLightDistance[MAX_POINT_LIGHTS];\n"
               "#endif\n"
               "#if MAX_SPOT_LIGHTS > 0\n"
               "  uniform vec3 spotLightColor[MAX_SPOT_LIGHTS];\n"
               "  uniform vec3 spotLightPosition[MAX_SPOT_LIGHTS];\n"
               "  uniform vec3 spotLightDirection[MAX_SPOT_LIGHTS];\n"
               "  uniform float spotLightAngle[MAX_SPOT_LIGHTS];\n"
               "  uniform float spotLightExponent[MAX_SPOT_LIGHTS];\n"
               "  uniform float spotLightDistance[MAX_SPOT_LIGHTS];\n"
               "#endif\n"
               "#if MAX_SPOT_LIGHTS > 0\n" // || defined(USE_BUMPMAP)\n"
               "  varying vec3 vWorldPosition;\n"
               "#endif\n"
               "#ifdef WRAP_AROUND\n"
               "  uniform vec3 wrapRGB;\n"
               "#endif\n"
               "varying vec3 vViewPosition;\n"
               "varying vec3 vNormal;\n";

    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useLight == BasicMaterial::Config::Light::Phong)
        return "  // Light Phong (per pixel)"
               "  vec3 normal = normalize(vNormal);\n"
               "  vec3 viewPosition = normalize(vViewPosition);\n"
               "  #ifdef USE_BUMPMAP\n"
               "    normal = perturbNormalArb(-vViewPosition normal "
               "dHdxy_fwd());\n"
               "  #endif\n"
               "  #if MAX_POINT_LIGHTS > 0\n"
               "    vec3 pointDiffuse  = vec3(0.0);\n"
               "    vec3 pointSpecular = vec3(0.0);\n"
               "    for (int i = 0; i < MAX_POINT_LIGHTS; i ++) {\n"
               "      vec4 lPosition = viewMatrix * "
               "vec4(pointLightPosition[i], 1.0);\n"
               "      vec3 lVector = lPosition.xyz + vViewPosition.xyz;\n"
               "      float lDistance = 1.0;\n"
               "      if (pointLightDistance[i] > 0.0)\n"
               "        lDistance = 1.0 - min((length(lVector) / "
               "pointLightDistance[i]), 1.0);\n"
               "      lVector = normalize(lVector);\n"
               "      // Diffuse\n"
               "float dotProduct = dot(normal, lVector);\n"

               "#ifdef WRAP_AROUND;\n"

               "float pointDiffuseWeightFull = max(dotProduct, 0.0);\n"
               "float pointDiffuseWeightHalf = max(0.5 * dotProduct + 0.5, "
               "0.0);\n"

               "vec3 pointDiffuseWeight = mix(vec3 (pointDiffuseWeightFull), "
               "vec3(pointDiffuseWeightHalf), wrapRGB);\n"

               "#else;\n"

               "float pointDiffuseWeight = max(dotProduct, 0.0);\n"

               "#endif;\n"

               "pointDiffuse  += diffuse * pointLightColor[i] * "
               "pointDiffuseWeight * lDistance;\n"

               // specular

               "vec3 pointHalfVector = normalize(lVector + viewPosition);\n"
               "float pointDotNormalHalf = max(dot(normal, pointHalfVector), "
               "0.0);\n"
               "float pointSpecularWeight = specularStrength * "
               "max(pow(pointDotNormalHalf, shininess), 0.0);\n"

               "#ifdef PHYSICALLY_BASED_SHADING;\n"

               // 2.0 => 2.0001 is hack to work around ANGLE bug

               "float specularNormalization = (shininess + 2.0001) / 8.0;\n"

               "vec3 schlick = specular + vec3(1.0 - specular) * pow(1.0 - "
               "dot(lVector, pointHalfVector), 5.0);\n"
               "pointSpecular += schlick * pointLightColor[i] * "
               "pointSpecularWeight * pointDiffuseWeight * lDistance * "
               "specularNormalization;\n"

               "#else;\n"

               "pointSpecular += specular * pointLightColor[i] * "
               "pointSpecularWeight * pointDiffuseWeight * lDistance;\n"

               "#endif;\n"

               "}\n"

               "#endif\n"

               "#if MAX_SPOT_LIGHTS > 0\n"

               "vec3 spotDiffuse  = vec3(0.0);\n"
               "vec3 spotSpecular = vec3(0.0);\n"

               "for (int i = 0; i < MAX_SPOT_LIGHTS; i ++) {\n"

               "#ifdef PHONG_PER_PIXEL\n"

               "vec4 lPosition = viewMatrix * vec4(spotLightPosition[i], "
               "1.0);\n"
               "vec3 lVector = lPosition.xyz + vViewPosition.xyz;\n"

               "float lDistance = 1.0;\n"
               "if (spotLightDistance[i] > 0.0)\n"
               "lDistance = 1.0 - min((length(lVector) / "
               "spotLightDistance[i]), 1.0);\n"

               "lVector = normalize(lVector);\n"

               "#else\n"

               "vec3 lVector = normalize(vSpotLight[i].xyz);\n"
               "float lDistance = vSpotLight[i].w;\n"

               "#endif\n"

               "float spotEffect = dot(spotLightDirection[i], "
               "normalize(spotLightPosition[i] - vWorldPosition));\n"

               "if (spotEffect > spotLightAngle[i]) {\n"

               "spotEffect = pow(spotEffect, spotLightExponent[i]);\n"

               // diffuse

               "float dotProduct = dot(normal, lVector);\n"

               "#ifdef WRAP_AROUND\n"

               "float spotDiffuseWeightFull = max(dotProduct 0.0);\n"
               "float spotDiffuseWeightHalf = max(0.5 * dotProduct + 0.5 "
               "0.0);\n"

               "vec3 spotDiffuseWeight = mix(vec3 (spotDiffuseWeightFull), "
               "vec3(spotDiffuseWeightHalf), wrapRGB);\n"

               "#else\n"

               "float spotDiffuseWeight = max(dotProduct, 0.0);\n"

               "#endif\n"

               "spotDiffuse += diffuse * spotLightColor[i] * spotDiffuseWeight "
               "* lDistance * spotEffect;\n"

               // specular

               "vec3 spotHalfVector = normalize(lVector + viewPosition);\n"
               "float spotDotNormalHalf = max(dot(normal, spotHalfVector), "
               "0.0);\n"
               "float spotSpecularWeight = specularStrength * "
               "max(pow(spotDotNormalHalf, shininess), 0.0);\n"

               "#ifdef PHYSICALLY_BASED_SHADING\n"

               // 2.0 => 2.0001 is hack to work around ANGLE bug

               "float specularNormalization = (shininess + 2.0001) / 8.0;\n"

               "vec3 schlick = specular + vec3(1.0 - specular) * pow(1.0 - "
               "dot(lVector, spotHalfVector), 5.0);\n"
               "spotSpecular += schlick * spotLightColor[i] * "
               "spotSpecularWeight * spotDiffuseWeight * lDistance * "
               "specularNormalization * spotEffect;\n"

               "#else\n"

               "spotSpecular += specular * spotLightColor[i] * "
               "spotSpecularWeight * spotDiffuseWeight * lDistance * "
               "spotEffect;\n"

               "#endif\n"

               "}\n"

               "}\n"

               "#endif\n"

               "#if MAX_DIR_LIGHTS > 0\n"

               "vec3 dirDiffuse  = vec3(0.0);\n"
               "vec3 dirSpecular = vec3(0.0);\n"

               "for(int i = 0; i < MAX_DIR_LIGHTS; i ++) {\n"

               "vec4 lDirection = viewMatrix * "
               "vec4(directionalLightDirection[i], 0.0);\n"
               "vec3 dirVector = normalize(lDirection.xyz);\n"

               // diffuse

               "float dotProduct = dot(normal, dirVector);\n"

               "#ifdef WRAP_AROUND\n"

               "float dirDiffuseWeightFull = max(dotProduct, 0.0);\n"
               "float dirDiffuseWeightHalf = max(0.5 * dotProduct + 0.5, "
               "0.0);\n"

               "vec3 dirDiffuseWeight = mix(vec3(dirDiffuseWeightFull), "
               "vec3(dirDiffuseWeightHalf), wrapRGB);\n"

               "#else\n"

               "float dirDiffuseWeight = max(dotProduct, 0.0);\n"

               "#endif\n"

               "dirDiffuse  += diffuse * directionalLightColor[i] * "
               "dirDiffuseWeight;\n"

               // specular

               "vec3 dirHalfVector = normalize(dirVector + viewPosition);\n"
               "float dirDotNormalHalf = max(dot(normal, dirHalfVector), "
               "0.0);\n"
               "float dirSpecularWeight = specularStrength * "
               "max(pow(dirDotNormalHalf, shininess), 0.0);\n"

               "#ifdef PHYSICALLY_BASED_SHADING\n"

               /*
               // fresnel term from skin shader
               "const float F0 = 0.128;\n"

               "float base = 1.0 - dot(viewPosition, dirHalfVector);\n"
               "float exponential = pow(base, 5.0);\n"

               "float fresnel = exponential + F0 * (1.0 - exponential);\n"
               */

               /*
               // fresnel term from fresnel shader
               "const float mFresnelBias = 0.08;\n"
               "const float mFresnelScale = 0.3;\n"
               "const float mFresnelPower = 5.0;\n"

               "float fresnel = mFresnelBias + mFresnelScale * pow(1.0 +
               dot(normalize(-viewPosition) normal) mFresnelPower);\n"
               */

               // 2.0 => 2.0001 is hack to work around ANGLE bug

               "float specularNormalization = (shininess + 2.0001) / 8.0;\n"

               //"dirSpecular += specular * directionalLightColor[i] *
               //dirSpecularWeight * dirDiffuseWeight * specularNormalization *
               //fresnel;\n"

               "vec3 schlick = specular + vec3(1.0 - specular) * pow(1.0 - "
               "dot(dirVector, dirHalfVector), 5.0);\n"
               "dirSpecular += schlick * directionalLightColor[i] * "
               "dirSpecularWeight * dirDiffuseWeight * specularNormalization;\n"

               "#else\n"

               "dirSpecular += specular * directionalLightColor[i] * "
               "dirSpecularWeight * dirDiffuseWeight;\n"

               "#endif\n"

               "}\n"

               "#endif\n"

               "vec3 totalDiffuse = vec3(0.0);\n"
               "vec3 totalSpecular = vec3(0.0);\n"

               "#if MAX_DIR_LIGHTS > 0\n"
               "totalDiffuse += dirDiffuse;\n"
               "totalSpecular += dirSpecular;\n"
               "#endif\n"

               "#if MAX_POINT_LIGHTS > 0\n"
               "totalDiffuse += pointDiffuse;\n"
               "totalSpecular += pointSpecular;\n"
               "#endif\n"

               "#if MAX_SPOT_LIGHTS > 0\n"
               "totalDiffuse += spotDiffuse;\n"
               "totalSpecular += spotSpecular;\n"
               "#endif\n"

               "#ifdef METAL\n"
               "gl_FragColor.xyz = gl_FragColor.xyz * (emissive + totalDiffuse "
               "+ ambientLightColor * ambient + totalSpecular);\n"
               "#else\n"
               "gl_FragColor.xyz = gl_FragColor.xyz * (emissive + totalDiffuse "
               "+ ambientLightColor * ambient) + totalSpecular;\n"
               "#endif\n";

    return "";
}
} // namespace fragment
} // namespace lightphong

//==========================================================================
namespace colors // Per vertex color
{
//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useColor)
        return "// Vertex color\n"
               "in vec3 vColor;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useColor)
        return "  // Vertex color\n"
               "  FragColor = FragColor * vec4(vColor, opacity);\n";
    return "";
}
} // namespace fragment

//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useColor)
        return "// Vertex color\n"
               "uniform vec3 color;\n"
               "out vec3 vColor;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useColor)
    {
        if (config.useGammaInput)
            return "  // Vertex color + Gamma input\n"
                   "  vColor = color * color;\n";
        return "  //Vertex color\n"
               "  vColor = color;\n";
    }
    return "";
}
} // namespace vertex
} // namespace colors

//==========================================================================
namespace skinning
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useSkinning)
    {
        if (config.useBoneTexture)
            return "// Skinning + use bone texture\n"
                   "uniform sampler2D boneTexture;\n"
                   "mat4 getBoneMatrix(const in float i)\n{\n"
                   "  float j = i * 4.0;\n"
                   "  float x = mod(j N_BONE_PIXEL_X);\n"
                   "  float y = floor(j / N_BONE_PIXEL_X);\n\n"
                   "  const float dx = 1.0 / N_BONE_PIXEL_X;\n"
                   "  const float dy = 1.0 / N_BONE_PIXEL_Y;\n"
                   "  y = dy * (y + 0.5);\n\n"
                   "  vec4 v1 = texture2D(boneTexture, vec2(dx * (x + 0.5), "
                   "y));\n"
                   "  vec4 v2 = texture2D(boneTexture, vec2(dx * (x + 1.5), "
                   "y));\n"
                   "  vec4 v3 = texture2D(boneTexture, vec2(dx * (x + 2.5), "
                   "y));\n"
                   "  vec4 v4 = texture2D(boneTexture, vec2(dx * (x + 3.5), "
                   "y));\n\n"
                   "  mat4 bone = mat4(v1, v2, v3, v4);\n"
                   "  return bone;\n"
                   "}\n";

        return "// Skinning without using bone texture\n"
               "uniform mat4 boneGlobalMatrices[MAX_BONES];\n"
               "mat4 getBoneMatrix(const in float i)\n{\n"
               "  mat4 bone = boneGlobalMatrices[int(i)];\n"
               "  return bone;\n"
               "}\n";
    }

    return "";
}

static const char* basecode(BasicMaterial::Config const& config)
{
    if (config.useSkinning)
        return "  // Skinning\n"
               "  mat4 boneMatX = getBoneMatrix(skinIndex.x);\n"
               "  mat4 boneMatY = getBoneMatrix(skinIndex.y);\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useSkinning)
        return "// Skinning\n"
               "  vec4 skinned  = boneMatX * skinVertexA * skinWeight.x;\n"
               "  skinned      += boneMatY * skinVertexB * skinWeight.y;\n"
               "  gl_Position   = projectionMatrix * modelViewMatrix * "
               "skinned;\n";
    return "";
}
} // namespace vertex
} // namespace skinning

//==========================================================================
namespace morphing // TODO
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& /*config*/)
{
    return "// Morphing TODO";
}

static const char* code(BasicMaterial::Config const& /*config*/)
{
    return "// Morphing TODO";
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& /*config*/)
{
    return "// Morphing TODO";
}

static const char* code(BasicMaterial::Config const& /*config*/)
{
    return "// Morphing TODO";
}
} // namespace fragment
} // namespace morphing

//==========================================================================
namespace shadowmap
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useShadowMap)
        return "// Shadow map\n"
               "varying vec4 vShadowCoord[MAX_SHADOWS];\n"
               "uniform mat4 shadowMatrix[MAX_SHADOWS];\n\n";
    return "";
}

static std::stringstream code(BasicMaterial::Config const& config)
{
    std::stringstream ss;

    if (!config.useShadowMap)
        return ss;

    ss << "  // Shadow map\n\n"
       << "  vec4 transformedPosition;\n";

    if (config.useMorphTargets)
    {
        ss << "  transformedPosition = modelMatrix * vec4(morphed, 1.0);\n";
    }
    else
    {
        if (config.useSkinning)
        {
            ss << "  transformedPosition = modelMatrix * skinned;\n";
        }
        else
        {
            ss << "  transformedPosition = modelMatrix * vec4(position, "
                  "1.0);\n";
        }
    }

    ss << "  for (int i = 0; i < MAX_SHADOWS; ++i) {\n"
       << "    vShadowCoord[i] = shadowMatrix[i] * transformedPosition;\n"
       << "  }\n";

    return ss;
}
} // namespace vertex

//----------------------------------------------------------------------
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useAlphaTest)
        return "// Shadow map\n"
               "uniform sampler2D shadowMap[MAX_SHADOWS];\n"
               "uniform vec2 shadowMapSize[MAX_SHADOWS];\n"
               "uniform float shadowDarkness[MAX_SHADOWS];\n"
               "uniform float shadowBias[MAX_SHADOWS];\n"
               "varying vec4 vShadowCoord[MAX_SHADOWS];\n"
               "float unpackDepth(const in vec4 rgba_depth) {\n"
               "  const vec4 bit_shift = vec4(1.0 / (256.0 * 256.0 * 256.0),\n"
               "                              1.0 / (256.0 * 256.0),\n"
               "                              1.0 / 256.0, 1.0);\n"
               "  float depth = dot(rgba_depth, bit_shift);\n"
               "  return depth;\n"
               "}\n";
    return "";
}

static const char* code(BasicMaterial::Config const& /*config*/)
{
    return // TODO
        "  // TODO Shadow map\n\n";
}
} // namespace fragment
} // namespace shadowmap

//==========================================================================
namespace alphatest
{
namespace fragment
{
static const char* params(BasicMaterial::Config const& config)
{
    if (config.useAlphaTest)
        return "// Alpha Test\n"
               "uniform float ALPHATEST;\n\n";
    return "";
}

static const char* code(BasicMaterial::Config const& config)
{
    if (config.useAlphaTest)
        return "  // Alpha Test\n"
               "  if (FragColor.a < ALPHATEST) discard;\n";
    return "";
}
} // namespace fragment
} // namespace alphatest

//==========================================================================
namespace gamma
{
namespace fragment
{
static const char* code(BasicMaterial::Config const& config)
{
    if (config.useGammaOutput)
        return "  // Linear to Gamma\n"
               "  FragColor.xyz = sqrt(FragColor.xyz);\n\n";
    return "";
}
} // namespace fragment
} // namespace gamma

//==========================================================================
namespace base
{
//----------------------------------------------------------------------
namespace vertex
{
static const char* code(BasicMaterial::Config const& config)
{
    if ((!config.useMorphTargets) && (!config.useSkinning))
        return "  // Final position\n"
               "  gl_Position = projectionMatrix * mvPosition;\n";
    return "";
}
} // namespace vertex
} // namespace base

//==========================================================================
namespace materials
{
//----------------------------------------------------------------------
namespace depth
{
static void code(GLVertexShader& vertexShader)
{
    vertexShader << "// shaders::materials::depth::vertex\n"
                 << shaders::common::version()
                 << shaders::common::vertex::params()
                 << shaders::mvp::vertex::params() << "void main()\n{\n"
                 << "  // Final position\n"
                 << "  gl_Position = " << shaders::mvp::mvp_code() << "}\n";
}

static void code(GLFragmentShader& fragmentShader)
{
    fragmentShader << "// shaders::materials::depth::fragment\n"
                   << shaders::common::version()
                   << shaders::common::fragment::params()
                   << "uniform float near;\n"
                   << "uniform float far;\n"
                   << "uniform float opacity;\n"
                   << "void main()\n{\n"
                   << "  float depth = gl_FragCoord.z / gl_FragCoord.w;\n"
                   << "  float color = 1.0 - smoothstep(near, far, depth);\n"
                   << "  FragColor = vec4(vec3(color), opacity);\n"
                   << "}\n";
}
} // namespace depth

//----------------------------------------------------------------------
namespace normals
{
static void code(GLVertexShader& vertexShader)
{
    vertexShader << "// shaders::materials::normals::vertex\n"
                 << shaders::common::version() << shaders::mvp::vertex::params()
                 << shaders::common::vertex::params()
                 << "uniform mat3 normalMatrix; // = "
                    "mat3(transpose(inverse(modelMatrix * viewMatrix)));\n"
                 << "out vec3 vNormal;\n"
                 << "void main()\n{\n"
                 << "  vNormal = normalMatrix * " NORMAL ";\n"
                 << "  // Final position\n"
                 << "  gl_Position = " << shaders::mvp::mvp_code() << "}\n";
}

static void code(GLFragmentShader& fragmentShader)
{
    fragmentShader
        << "// shaders::materials::normals::fragment\n"
        << shaders::common::version() << shaders::common::fragment::params()
        << "uniform float opacity;\n"
        << "in vec3 vNormal;\n"
        << "void main()\n{\n"
        << "  FragColor = vec4(0.5 * normalize(vNormal) + 0.5, opacity);\n"
        << "}\n";
}
} // namespace normals

//----------------------------------------------------------------------
namespace basic
{
//************
namespace line
{
static void code(GLVertexShader& vertexShader)
{
    vertexShader << "// shaders::materials::basic::line::vertex\n"
                 << shaders::common::version()
                 << shaders::common::vertex::params()
                 << shaders::mvp::vertex::params() << "in vec4 colors;\n"
                 << "out vec4 vColors;\n"
                 << "void main()\n{\n"
                 << "  vColors = colors;\n"
                 << "  // Final position\n"
                 << "  gl_Position = " << shaders::mvp::mvp_code() << "}\n";
}

static void code(GLFragmentShader& fragmentShader)
{
    fragmentShader << "// shaders::materials::basic::line::fragment\n"
                   << shaders::common::version()
                   << shaders::common::fragment::params()
                   << "in vec4 vColors;\n"
                   << "void main()\n{\n"
                   << "  FragColor = vColors;\n"
                   << "}\n";
}
} // namespace line

//************
namespace mesh
{
static void code(GLVertexShader& vertexShader,
                 BasicMaterial::Config const& config)
{
    vertexShader << "// shaders::materials::basic::mesh::vertex\n"
                 << shaders::common::version()
                 << shaders::common::vertex::params()
                 << shaders::mvp::vertex::params()
                 << shaders::lightmap::vertex::params(config)
                 << shaders::envmap::vertex::params(config)
                 << shaders::colors::vertex::params(config)
                 << shaders::skinning::vertex::params(config)
                 // << shaders::morphtarget::vertex::params(config)
                 << shaders::shadowmap::vertex::params(config)
                 << "void main()\n{\n"
                 << shaders::mvp::mv_code()
                 << shaders::lightmap::vertex::code(config)
                 << shaders::envmap::vertex::code(config).str()
                 << shaders::colors::vertex::code(config)
                 << shaders::skinning::vertex::basecode(config)
                 << shaders::skinning::vertex::code(config)
                 // << shaders::morphtarget::vertex::code(config)
                 << shaders::base::vertex::code(config)
                 << shaders::shadowmap::vertex::code(config).str() << "}\n";
}

static void code(GLFragmentShader& fragmentShader,
                 BasicMaterial::Config const& config)
{
    fragmentShader << "// shaders::materials::basic::mesh::fragment\n"
                   << shaders::common::version()
                   << shaders::common::fragment::params()
                   << shaders::common::constants()
                   << shaders::colors::fragment::params(config)
                   << shaders::texture::triangles::fragment::params(config)
                   << shaders::lightmap::fragment::params(config)
                   << shaders::envmap::fragment::params(config)
                   << shaders::fog::fragment::params(config)
                   << shaders::shadowmap::fragment::params(config)
                   << shaders::specularmap::fragment::params(config)
                   << shaders::alphatest::fragment::params(config)
                   << "// Diffuse color\n"
                   << "uniform vec3 diffuse;\n"
                   << "uniform float opacity;\n\n"
                   << "void main()\n{\n"
                   << "  FragColor = vec4(diffuse, opacity);\n"
                   << shaders::texture::triangles::fragment::code(config)
                   << shaders::alphatest::fragment::code(config)
                   << shaders::specularmap::fragment::code(config)
                   << shaders::lightmap::fragment::code(config)
                   << shaders::colors::fragment::code(config)
                   << shaders::envmap::fragment::code(config).str()
                   << shaders::shadowmap::fragment::code(config)
                   << shaders::gamma::fragment::code(config)
                   << shaders::fog::fragment::code(config) << "}\n";
}
} // namespace mesh
} // namespace basic
} // namespace materials
} // namespace shaders

#endif // OPENGLCPPWRAPPER_SHADERLIB_HPP
