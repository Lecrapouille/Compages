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

#include "Scene/AnimatedModelNode.hpp"
#include <iostream>

AnimatedMesh::AnimatedMesh(nlohmann::json const& jsonObj,
                           std::string const& texture_path)
    : m_prog("prog"), m_vao("vao")
{
    // FIXME: create a shader in ShaderLib.hpp and load it
    m_vertex_shader.read("/home/qq/MyGitHub/OpenGLCppWrapper/src/Scene/"
                         "Material/animationShader.glvs");
    m_fragment_shader.read("/home/qq/MyGitHub/OpenGLCppWrapper/src/Scene/"
                           "Material/animationShader.glfs");
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return;
    }
    m_prog.bind(m_vao);

    if (!LoadTexture(texture_path))
    {
        std::cout << "Failed loading texture" << std::endl;
    }

    // References to VBOs
    auto& positions = m_vao.vector3f("position");
    auto& texCoords = m_vao.vector2f("texCoord");
    // auto& normals = m_vao.vector3f("normal"); // FIXME removed by GLSL
    // compiler
    auto& jointIds = m_vao.vector4u("jointIds");
    auto& jointWeights = m_vao.vector4f("jointWeights");

    // Load indices array into vector
    auto& indices = m_vao.index();
    indices = jsonObj["indices"].get<std::vector<unsigned int>>();

    // load each vertex data into each vector
    for (nlohmann::json const& vertex : jsonObj["vertices"])
    {
        Vector3f pos(
            vertex["pos"]["x"], vertex["pos"]["y"], vertex["pos"]["z"]);
        Vector2f texcoord(vertex["texcoord"]["x"], vertex["texcoord"]["y"]);
        Vector3f normal(vertex["normal"]["x"],
                        vertex["normal"]["y"],
                        vertex["normal"]["z"]);
        Vector4u jointId(0);
        Vector4f jointWeight(0);

        for (size_t i = 0; i < vertex["joints"].size(); ++i)
        {
            nlohmann::json const& joint = vertex["joints"][i];
            switch (i)
            {
                case 0:
                    jointId.x = joint["id"];
                    jointWeight.x = joint["weight"];
                    break;
                case 1:
                    jointId.y = joint["id"];
                    jointWeight.y = joint["weight"];
                    break;
                case 2:
                    jointId.z = joint["id"];
                    jointWeight.z = joint["weight"];
                    break;
                case 3:
                    jointId.w = joint["id"];
                    jointWeight.w = joint["weight"];
                    break;
                default:
                    std::cerr << "Too many joints, this shouldn't happen!";
            }
        }

        positions.append(pos);
        texCoords.append(texcoord);
        // FIXME normals.append(normal);
        jointIds.append(jointId);
        jointWeights.append(jointWeight);
    }

    // load each bone data into two unordered maps to be used when needed to
    // load
    for (nlohmann::json const& bone : jsonObj["bones"])
    {
        BoneData data;

        Vector3f pos(bone["pos"]["x"], bone["pos"]["y"], bone["pos"]["z"]);
        Quatf rot(bone["rot"]["w"],
                  bone["rot"]["x"],
                  bone["rot"]["y"],
                  bone["rot"]["z"]);
        Vector3f scale(
            bone["scale"]["x"], bone["scale"]["y"], bone["scale"]["z"]);

        // GLM order Matrix44f offset = matrix::translationMatrix(pos) *
        // rot.toMatrix() * matrix::scalingMatrix(scale);
        Matrix44f offset = matrix::scalingMatrix(scale) * rot.toMatrix() *
                           matrix::translationMatrix(pos);

        unsigned int id = bone["id"].get<unsigned int>();
        std::string name = bone["name"].get<std::string>();

        data.offsetMatrix = offset;

        m_boneIdMap.insert(std::make_pair(name, id));
        m_boneDataMap.insert(std::make_pair(id, data));
    }
}

void Animator::Update(double elapsedTime)
{
    if (m_currentAnimation == nullptr)
    {
        return;
    }
    m_animationTime += elapsedTime;
    if (m_animationTime > m_currentAnimation->getDuration())
    {
        m_animationTime -= m_currentAnimation->getDuration();
    }

    std::unordered_map<std::string, Matrix44f> jointMap =
        CalculateJointTransforms();

    for (auto jointTransform : jointMap)
    {
        Joint* joint = m_model->FindJoint(jointTransform.first);
        if (joint != nullptr)
        {
            joint->setTransform(jointTransform.second);
        }
    }
    m_model->InitJointHierarchy(m_model->getRootJoint(),
                                Matrix44f(matrix::Type::Identity));
}
