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

#include "Loaders/3D/OBJ.hpp"
#include <cstring>
#include <fstream>
#include <sstream>

// TODO manage o, g and s
// https://web.cse.ohio-state.edu/~shen.94/581/Site/Lab3_files/Labhelp_Obj_parser.htm

//------------------------------------------------------------------------------
static bool readObjFile(std::stringstream& ss, const std::string& fileName)
{
    std::ifstream objFile;
    objFile.open(fileName);
    if (!objFile)
    {
        return false;
    }

    ss << objFile.rdbuf();
    objFile.close();
    return true;
}

//------------------------------------------------------------------------------
bool OBJFileLoader::load(std::string const& fileName,
                         GLVertexBuffer<Vector3f>& vertices,
                         GLVertexBuffer<Vector3f>& normals,
                         GLVertexBuffer<Vector2f>& uv,
                         GLIndex32& indices)
{
    std::stringstream objData;

    if (!readObjFile(objData, fileName))
    {
        m_error = "Failed loading 3D file '" + fileName + "'. Reason was: '" +
                  std::strerror(errno) + "'";
        std::cerr << m_error << std::endl;
        return false;
    }

    std::vector<Vector3f> tmp_vertices;
    std::vector<Vector3f> tmp_normals;
    std::vector<Vector2f> tmp_uv;
    std::vector<std::string> faces;
    std::string line;
    float x, y, z;
    size_t lines = 0u;

    while (std::getline(objData, line))
    {
        lines += 1u;
        std::istringstream ss(line);
        std::string token;

        ss >> token;
        if (token == "v") // Vertex
        {
            ss >> x >> y >> z;
            tmp_vertices.push_back(Vector3f(x, y, z));
        }
        else if (token == "vt") // Texture
        {
            ss >> x >> y;
            tmp_uv.push_back(Vector2f(x, y));
        }
        else if (token == "vn") // Normals
        {
            ss >> x >> y >> z;
            tmp_normals.push_back(Vector3f(x, y, z));
        }
        else if (token == "f") // Faces
        {
            for (int v = 0; v < 3; v++)
            {
                ss >> token;
                faces.push_back(token);
            }
        }
    }

    if (tmp_vertices.size() == 0u)
        return false;

    vertices.reserve(tmp_vertices.size());
    uv.reserve(tmp_uv.size());
    normals.reserve(tmp_normals.size());

    GLIndex32::Type count = 0u;
    for (auto& f : faces)
    {
        std::istringstream ss(f);
        std::string str;
        size_t type = 0u;

        while (getline(ss, str, '/'))
        {
            ++type;

            if (str.size() == 0u)
                continue;

            try
            {
                uint32_t index = uint32_t(std::stoul(str)) - 1u;
                if (type == 1u)
                {
                    vertices.append(tmp_vertices.at(index));
                }
                else if (type == 2u)
                {
                    uv.append(tmp_uv.at(index));
                }
                else if (type == 3u)
                {
                    normals.append(tmp_normals.at(index));
                }
            }
            catch (std::exception const&)
            {
                m_error = "Failed loading 3D file '" + fileName +
                          "'. Reason was: '" + "Erroneous face index at line " +
                          std::to_string(lines) + "'";
                std::cerr << m_error << std::endl;
                return false;
            }
        }

        indices.append(count++);
    }

    return true;
}
