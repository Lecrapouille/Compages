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

#include "Scene/Geometry/Model.hpp"
#include "Loaders/3D/OBJ.hpp"
// #include "Loaders/3D/MD5mesh.hpp"
#include "Common/NonCppStd.hpp"

static std::string extension(std::string const& path)
{
    std::string::size_type pos = path.find_last_of(".");
    if (pos != std::string::npos)
    {
        std::string ext = path.substr(pos + 1, std::string::npos);

        // Ignore the ~ in the extension (ie. foo.txt~)
        if ('~' == ext.back())
            ext.pop_back();

        // Get the file extension in lower case
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

//--------------------------------------------------------------------------
static std::map<std::string, std::unique_ptr<ShapeLoader>> loaders()
{
    std::map<std::string, std::unique_ptr<ShapeLoader>> mp;

    mp["obj"] = std::make_unique<OBJFileLoader>();
    // mp["md5mesh"] = std::make_unique<MD5MeshFileLoader>();

    return mp;
}

//--------------------------------------------------------------------------
bool Model::generate(GLVAO32& vao, const bool clear)
{
    static std::map<std::string, std::unique_ptr<ShapeLoader>> s_loaders =
        loaders();

    if (config.path.size() == 0u)
    {
        std::cerr << "ERROR: Model::doGenerate: no input file given"
                  << std::endl;
        return false;
    }

    std::string ext = extension(config.path);
    auto loader = s_loaders.find(ext);
    if (loader == s_loaders.end())
    {
        std::cerr << "This file extension '" << ext << "' is not managed"
                  << std::endl;
        return false;
    }

    if (!vao.hasVBO<Vector3f>(shaders::name::position))
    {
        std::cerr << "VBO for vertices is needed" << std::endl;
        return false;
    }

    // Dummy containers.
    GLVertexBuffer<Vector3f> tmp_normals;
    GLVertexBuffer<Vector2f> tmp_uv;

    auto& positions = vao.vector3f(shaders::name::position);
    auto& normals = vao.hasVBO<Vector3f>(shaders::name::normal)
                        ? vao.vector3f(shaders::name::normal)
                        : tmp_normals;
    auto& UVs = vao.hasVBO<Vector2f>(shaders::name::uv)
                    ? vao.vector2f(shaders::name::uv)
                    : tmp_uv;

    if (clear)
    {
        positions.clear();
        normals.clear();
        UVs.clear();
        vao.index().clear();
    }

    return loader->second->load(
        config.path, positions, normals, UVs, vao.index());
}
