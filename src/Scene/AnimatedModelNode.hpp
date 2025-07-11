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

#ifndef OPENGLCPPWRAPPER_SCENEGRAPH_SCENE_ANIMATED_MODEL_HPP
#define OPENGLCPPWRAPPER_SCENEGRAPH_SCENE_ANIMATED_MODEL_HPP

#include "Loaders/Textures/SOIL.hpp"
#include "Math/Matrix.hpp"
#include "Math/Quaternion.hpp"
#include "OpenGL/Buffers/iVAO.hpp"
#include "Scene/Camera/CameraNode.hpp"
#include "Scene/SceneTree.hpp"
#include "nlohmann/json.hpp"
#include <string>
#include <unordered_map>
#include <vector>

// Based on https://github.com/emargollo/OpenGL-Animator
// also inspired from ThinMatrix's tutorial playlist
// https://www.youtube.com/watch?v=f3Cr8Yx3GGA&list=PLRIWtICgwaX2tKWCxdeB7Wv_rTET9JtWW

class AnimatedModel;

class Animation
{
public:

    struct JointAnim // TODO keyFrames
    {
        std::string m_name;
        std::map<double, Vector3f> m_positionKeys;
        std::map<double, Quatf> m_rotationKeys;
        std::map<double, Vector3f> m_scallingKeys;
    };

public:

    Animation(nlohmann::json const& jsonObj)
    {
        m_name = jsonObj["name"].get<std::string>();
        m_duration = jsonObj["duration"].get<double>();

        for (nlohmann::json const& bone : jsonObj["bones"])
        {
            JointAnim jAnim;
            jAnim.m_name = bone["name"].get<std::string>();

            for (nlohmann::json const& posKey : bone["pos_keys"])
            {
                Vector3f pos(posKey["x"], posKey["y"], posKey["z"]);
                double time = posKey["time"];
                jAnim.m_positionKeys.insert(std::make_pair(time, pos));
            }
            for (nlohmann::json const& rotKey : bone["rot_keys"])
            {
                Quatf rot(maths::toDegree(rotKey["w"].get<float>()),
                          maths::toDegree(rotKey["x"].get<float>()),
                          maths::toDegree(rotKey["y"].get<float>()),
                          maths::toDegree(rotKey["z"].get<float>()));
                double time = rotKey["time"];
                jAnim.m_rotationKeys.insert(std::make_pair(time, rot));
            }
            for (nlohmann::json const& scaleKey : bone["scale_keys"])
            {
                Vector3f scale(scaleKey["x"], scaleKey["y"], scaleKey["z"]);
                double time = scaleKey["time"];
                jAnim.m_scallingKeys.insert(std::make_pair(time, scale));
            }

            m_jointAnims.push_back(jAnim);
        }
    }

    inline std::string const& getName() const
    {
        return m_name;
    }
    inline double getDuration() const
    {
        return m_duration;
    }
    inline std::vector<JointAnim> const& getJointAnims() const
    {
        return m_jointAnims;
    }

private:

    std::string m_name;
    double m_duration = 0.0; // TODO chrono::seconds
    std::vector<Animation::JointAnim> m_jointAnims;
};

class AnimatedMesh
{
public:

    struct BoneData
    {
        Matrix44f offsetMatrix;
        Matrix44f finalTransformation;
    };

    AnimatedMesh(nlohmann::json const& jsonObj,
                 std::string const& texture_path);

    bool Draw(Matrix44f const& modelMatrix)
    {
        std::cout << "Model: " << modelMatrix << std::endl;
        m_prog.matrix44f("modelMatrix") = modelMatrix;
        return m_vao.draw();
    }

    void cameraUpdated(Camera& camera)
    {
        m_prog.matrix44f("viewMatrix") = camera.view();
        m_prog.matrix44f("projectionMatrix") = camera.projection();

        std::cout << "Camera view: " << camera.view() << std::endl;
        std::cout << "Camera projection: " << camera.projection() << std::endl;
    }

    std::vector<Matrix44f> GetBoneArray()
    {
        std::vector<Matrix44f> boneArray;

        boneArray.assign(m_boneDataMap.size(), Matrix44f());
        for (auto i : m_boneDataMap)
        {
            boneArray[i.first] = i.second.finalTransformation;
        }

        return boneArray;
    }

    bool LoadTexture(std::string const& filename)
    {
        // Get the texture from its sampler name.
        GLTexture2D& texture = m_vao.texture2D("diffuse");

        // Configurate the texture
        texture.interpolation(GLTexture::Minification::LINEAR,
                              GLTexture::Magnification::LINEAR);
        texture.wrap(GLTexture::Wrap::MIRRORED_REPEAT);

        // Read the png file and load data on the texture (CPU side). Dirty data
        // will be automaticaly transfered to the GPU. Note that the loader is
        // by default SOIL but you can pass it your own load as 2nd parameter.
        return texture.load<SOIL>(filename);
    }

public:

    std::unordered_map<std::string, unsigned int> m_boneIdMap;
    std::unordered_map<unsigned int, AnimatedMesh::BoneData> m_boneDataMap;

private:

    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;
    GLProgram m_prog;
    GLVAO32 m_vao;
};

// FIXME faire type Joint = SceneObject car ce n'est rien d'autre que
// qu'un scene graph (mais pour les animataions et surement en mode privee
// car on ne souhaite pas attacher un noeud de scene graph sur un joint).
class Joint
{
public:

    inline Matrix44f const& getTransform() const
    {
        return m_jointTransform;
    }
    inline void setTransform(Matrix44f const& transform)
    {
        m_jointTransform = transform;
    }
    inline void setName(std::string const& name)
    {
        m_name = name;
    }
    inline std::string const& getName() const
    {
        return m_name;
    }
    std::vector<std::unique_ptr<Joint>> const& getChildren() const
    {
        return m_children;
    }
    void addChild(std::unique_ptr<Joint> j)
    {
        m_children.push_back(std::move(j));
    }

    Joint* Find(std::string const& name)
    {
        if (m_name == name)
            return this;

        for (auto const& joint : m_children)
        {
            Joint* j = joint->Find(name);
            if (j != nullptr)
                return j;
        }
        return nullptr;
    }

private:

    Matrix44f m_jointTransform;
    std::vector<std::unique_ptr<Joint>> m_children;
    std::string m_name;
};

class Animator
{
public:

    Animator(AnimatedModel* model) : m_model(model) {}

    void startAnimation(const std::string& animationName)
    {
        for (auto const& animation : m_animations)
        {
            if (animation->getName() == animationName)
            {
                m_animationTime = 0;
                m_currentAnimation = animation.get();
            }
        }
    }

    void addAnimation(nlohmann::json const& jsonAnimation)
    {
        m_animations.push_back(std::make_unique<Animation>(jsonAnimation));
    }

    void Update(double elapsedTime);

private:

    std::unordered_map<std::string, Matrix44f> CalculateJointTransforms()
    {
        std::unordered_map<std::string, Matrix44f> jointTransforms;
        for (auto jointKeys : m_currentAnimation->getJointAnims())
        {
            // Using upper bound as it will never return the first value of a
            // map.
            auto posNext =
                jointKeys.m_positionKeys.upper_bound(m_animationTime);
            auto rotNext =
                jointKeys.m_rotationKeys.upper_bound(m_animationTime);
            auto scaleNext =
                jointKeys.m_scallingKeys.upper_bound(m_animationTime);

            if (posNext == jointKeys.m_positionKeys.end())
            {
                posNext--;
            }
            if (rotNext == jointKeys.m_rotationKeys.end())
            {
                rotNext--;
            }
            if (scaleNext == jointKeys.m_scallingKeys.end())
            {
                scaleNext--;
            }

            Vector3f pos = GetInterpolatedPos(*(std::prev(posNext)), *posNext);
            Quatf rot = GetInterpolatedRot(*(std::prev(rotNext)), *rotNext);
            Vector3f scale =
                GetInterpolatedScale(*(std::prev(scaleNext)), *scaleNext);

            Matrix44f transform = matrix::translationMatrix(pos) *
                                  rot.toMatrix() * matrix::scalingMatrix(scale);
            jointTransforms.insert(std::make_pair(jointKeys.m_name, transform));
        }
        return jointTransforms;
    }

    Vector3f GetInterpolatedPos(std::pair<double, Vector3f> start,
                                std::pair<double, Vector3f> end)
    {
        double delta = end.first - start.first;
        double factor = (m_animationTime - start.first) / delta;
        Vector3f deltaPos = end.second - start.second;
        Vector3f pos = start.second + float(factor) * deltaPos;
        return pos;
    }

    Quatf GetInterpolatedRot(std::pair<double, Quatf> start,
                             std::pair<double, Quatf> end)
    {
        double delta = end.first - start.first;
        double factor = (m_animationTime - start.first) / delta;
        Quatf rot = Slerp(start.second, end.second, float(factor));
        rot.normalize();
        return rot;
    }

    Vector3f GetInterpolatedScale(std::pair<double, Vector3f> start,
                                  std::pair<double, Vector3f> end)
    {
        double delta = end.first - start.first;
        double factor = (m_animationTime - start.first) / delta;
        Vector3f deltaScale = end.second - start.second;
        Vector3f scale = start.second + float(factor) * deltaScale;
        return scale;
    }

    Quatf Slerp(Quatf q0, Quatf q1, float t)
    {
        // Only unit quaternions are valid rotations.
        // Normalize to avoid undefined behavior.
        q0.normalize();
        q1.normalize();

        // Compute the cosine of the angle between the two vectors.
        float d = dot(q0, q1); // dot product

        // If the dot product is negative, the quaternions
        // have opposite handed-ness and slerp won't take
        // the shorter path. Fix by reversing one quaternion.
        if (d < 0.0f)
        {
            q1 = -q1;
            d = -d;
        }

        maths::clamp(
            d, -1.0f, 1.0f); // Robustness: Stay within domain of acos()
        float theta_0 = std::acos(d); // theta_0 = angle between input vectors
        float theta = theta_0 * t;    // theta = angle between q0 and result

        float s0 =
            std::cos(theta) -
            d * std::sin(theta) /
                std::sin(theta_0); // == sin(theta_0 - theta) / sin(theta_0)
        float s1 = std::sin(theta) / std::sin(theta_0);

        return (s0 * q0) + (s1 * q1);
    }

private:

    AnimatedModel* m_model;
    std::vector<std::unique_ptr<Animation>> m_animations;
    Animation* m_currentAnimation = nullptr;
    double m_animationTime;
};

class AnimatedModel: public SceneObject
{
public:

    using Ptr = std::unique_ptr<AnimatedModel>;

    AnimatedModel(std::string const& filename) : SceneObject(filename)
    {
        m_animator = std::make_unique<Animator>(this);
        m_rootJoint = std::make_unique<Joint>();
        if (!LoadModel(filename))
        {
            std::cout << "Failed loading model" << std::endl;
        }
    }

    bool LoadModel(const std::string& filename)
    {
        nlohmann::json model;
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << "Failed loading animated model. File '" << filename
                      << "' does not exist" << std::endl;
            return false;
        }
        file >> model;
        file.close();

        if (model.find("texture") != model.end())
        {
            m_path_texture =
                "/home/qq/MyGitHub/OpenGLCppWrapper/examples/external/assets/";
            m_path_texture += model["texture"].get<std::string>();
        }
        for (nlohmann::json const& jsonMesh : model["meshes"])
        {
            m_meshes.push_back(
                std::make_unique<AnimatedMesh>(jsonMesh, m_path_texture));
        }
        for (nlohmann::json const& jsonAnimation : model["animations"])
        {
            m_animator->addAnimation(jsonAnimation);
        }

        LoadJointHierarchy(model["rootnode"]);
        InitJointHierarchy(*m_rootJoint, Matrix44f(matrix::Type::Identity));
        return true;
    }

    void LoadJointHierarchy(nlohmann::json root)
    {
        std::cout << "  LoadJointHierarchy: " << root["name"] << std::endl;

        Vector3f pos(root["pos"]["x"], root["pos"]["y"], root["pos"]["z"]);
        Quatf rot(root["rot"]["w"],
                  root["rot"]["x"],
                  root["rot"]["y"],
                  root["rot"]["z"]);
        Vector3f scale(
            root["scale"]["x"], root["scale"]["y"], root["scale"]["z"]);

        // GLM order: Matrix44f modelMatrix = matrix::translationMatrix(pos) *
        // rot.toMatrix() * matrix::scalingMatrix(scale);
        Matrix44f modelMatrix = matrix::scalingMatrix(scale) * rot.toMatrix() *
                                matrix::translationMatrix(pos);
        m_inverseModelMatrix = matrix::inverse(modelMatrix);

        std::cout << "Scaling: "
                  << matrix::scalingMatrix(Vector3f(1.0f, 2.0f, 3.0f))
                  << std::endl;
        std::cout << "Trans: "
                  << matrix::translationMatrix(Vector3f(1.0f, 2.0f, 3.0f))
                  << std::endl;
        std::cout << "Rot: " << rot.toMatrix() << std::endl;
        std::cout << "Model: " << modelMatrix << std::endl;
        std::cout << "Model^-1: " << m_inverseModelMatrix << std::endl;

        m_rootJoint->setTransform(modelMatrix);
        m_rootJoint->setName(root["name"]);
        for (nlohmann::json const& child : root["children"])
        {
            m_rootJoint->addChild(LoadJoint(child));
        }
    }

    void InitJointHierarchy(Joint const& root, Matrix44f const& parentTransform)
    {
        Matrix44f globalTransform = parentTransform * root.getTransform();

        // Insert configure bones final transforms for each mesh
        UpdateMeshBone(root.getName(), globalTransform);

        for (auto const& j : root.getChildren())
        {
            InitJointHierarchy(*j, globalTransform);
        }
    }

    void UpdateMeshBone(std::string const& jointName,
                        Matrix44f const& globalTransform)
    {
        for (auto& mesh : m_meshes)
        {
            if (mesh->m_boneIdMap.find(jointName) == mesh->m_boneIdMap.end())
                continue;
            unsigned int id = mesh->m_boneIdMap[jointName];
            mesh->m_boneDataMap[id].finalTransformation =
                m_inverseModelMatrix * globalTransform *
                mesh->m_boneDataMap[id].offsetMatrix;
        }
    }

    virtual void onUpdate(float const dt) override
    {
        m_animator->Update(dt /*elapsedTime*/);
    }

    // void Draw(Transform position, Camera camera)
    virtual bool onDraw(Matrix44f const& modelMatrix) override
    {
        bool res = true;

        for (auto const& it : m_meshes)
        {
            res &= it->Draw(modelMatrix);
        }

        return res;
    }

    virtual void onCameraUpdated(Camera& camera) override
    {
        for (auto const& it : m_meshes)
        {
            it->cameraUpdated(camera);
        }
    }

    Animator& getAnimator()
    {
        return *m_animator;
    }
    Joint& getRootJoint()
    {
        return *m_rootJoint;
    }

    Joint* FindJoint(std::string const& jointName)
    {
        return m_rootJoint->Find(jointName);
    }

private:

    std::unique_ptr<Joint> LoadJoint(nlohmann::json joint)
    {
        std::unique_ptr<Joint> j = std::make_unique<Joint>();

        Vector3f pos(joint["pos"]["x"], joint["pos"]["y"], joint["pos"]["z"]);
        Quatf rot(joint["rot"]["w"],
                  joint["rot"]["x"],
                  joint["rot"]["y"],
                  joint["rot"]["z"]);
        Vector3f scale(
            joint["scale"]["x"], joint["scale"]["y"], joint["scale"]["z"]);

        // GLM: Matrix44f jointMatrix = matrix::translationMatrix(pos) *
        // rot.toMatrix() * matrix::scalingMatrix(scale);
        Matrix44f jointMatrix = matrix::scalingMatrix(scale) * rot.toMatrix() *
                                matrix::translationMatrix(pos);

        j->setTransform(jointMatrix);
        j->setName(joint["name"]);

        for (nlohmann::json child : joint["children"])
        {
            j->addChild(LoadJoint(child));
        }

        return j;
    }

private:

    Matrix44f m_inverseModelMatrix;

    std::vector<std::unique_ptr<AnimatedMesh>> m_meshes;
    std::unique_ptr<Joint> m_rootJoint;
    std::unique_ptr<Animator> m_animator;
    std::string m_path_texture;
};

#endif
