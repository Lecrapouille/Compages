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
//
// This file is a derived work of https://github.com/glumpy/glumpy
//
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
//=====================================================================

#include "OpenGL/Shaders/Shader.hpp"
#include "Common/File.hpp"
#include <iostream>
#include <list>
#include <regex>

//--------------------------------------------------------------------------
GLShader::GLShader(std::string const& name, const GLenum target)
    : GLObject(name, target), m_file_name(name)
{
}

//--------------------------------------------------------------------------
GLShader::~GLShader()
{
    release();
}

//--------------------------------------------------------------------------
bool GLShader::read(std::string const& file)
{
    throw_if_already_compiled();
    m_code.clear();
    m_file_name = file;
    std::cout << "read: " << path.expand(file) << std::endl;
    return read(path.expand(file), m_code);
}

//--------------------------------------------------------------------------
bool GLShader::read(std::string const& file, std::string& code)
{
    std::string msg;
    std::ifstream infile;

    // Open shader file
    errno = 0;
    infile.open(file);
    if (!infile.fail())
    {
        // Get the size of the shader
        infile.seekg(0, std::ios::end);
        std::streampos pos = infile.tellg();
        infile.seekg(0, std::ios::beg);

        if (pos > 0)
        {
            // Increase the buffer size
            size_t p = code.size();
            code.resize(p + static_cast<size_t>(pos));

            // Read the shader content
            if (!infile.read(&code[p], static_cast<std::streamsize>(pos)))
            {
                msg = "Failed reading file '" + file + "'. Reason was '" +
                      ::strerror(errno) + "'";
                concatError(msg);
                return false;
            }
        }
        else
        {
            msg = "Failed reading file '" + file +
                  "'. Reason was 'cannot get the file size'";
            concatError(msg);
            return false;
        }
    }
    else
    {
        msg = "Failed opening file '" + file + "'. Reason was '" +
              ::strerror(errno) + "'";
        concatError(msg);
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------
bool GLShader::solveIncludes()
{
    std::regex regex(R"(^\s*#\s*include\s+([\w\/.]+))");
    std::cmatch matches;

    std::istringstream code(m_code);
    std::string new_code;
    std::string line;
    std::list<std::string> opened_files;
    bool changed;

    opened_files.push_back(File::fileName(path.expand(m_file_name)));
    do
    {
        new_code = "";
        changed = false;
        while (std::getline(code, line))
        {
            std::regex_search(line.c_str(), matches, regex);
            if (matches.empty())
            {
                new_code += line;
                new_code += '\n';
            }
            else
            {
                // Found a file to include, read its whole content and append it
                // to the code
                std::string full_path = path.expand(matches.str(1));
                m_file_name = File::fileName(full_path);
                if (opened_files.end() != std::find(opened_files.begin(),
                                                    opened_files.end(),
                                                    m_file_name))
                {
                    std::cout << full_path << " already included" << std::endl;
                    continue;
                }
                opened_files.push_back(m_file_name);
                if (!read(full_path, new_code))
                {
                    return false;
                }
                // By security add the character on the same line
                // after "include foo" for example "include foo uniform bar;"
                std::string eol = line.substr(size_t(matches.position(0)) +
                                              matches.str(0).size());
                if (!eol.empty())
                {
                    new_code += '\n';
                    new_code += eol;
                }
                changed = true;
            }
        }

        // Reparse the shader code to keep searching potential token 'include'
        // added by the included file.
        if (changed)
        {
            code.clear();
            code.str(new_code);
        }
    } while (changed);

    m_code = new_code;
    return true;
}

//--------------------------------------------------------------------------
std::string GLShader::strerror()
{
    std::string tmp(m_error);
    m_error.clear();
    return tmp;
}

//--------------------------------------------------------------------------
bool GLShader::compile()
{
    if (compiled())
        return true;

    begin();
    return (m_error.size() == 0u);
}

//--------------------------------------------------------------------------
bool GLShader::onCreate()
{
    m_handle = glCheck(glCreateShader(m_target));
    return false;
}

//--------------------------------------------------------------------------
void GLShader::onActivate() {}

//--------------------------------------------------------------------------
bool GLShader::onSetup()
{
    bool compiled = false;

    std::cout << "Compiling shader " << name() << " ..." << std::endl;
    if (loaded())
    {
        solveIncludes();
        char const* source = m_code.c_str();
        GLint length = static_cast<GLint>(m_code.size());
        glCheck(glShaderSource(m_handle, 1, &source, &length));
        glCheck(glCompileShader(m_handle));
        compiled = checkCompilationStatus(m_handle);
    }
    else
    {
        std::string msg = "Failed compiling shader " + name() +
                          ". Reason was 'no shader code was loaded'";
        concatError(msg);
    }

    return !compiled;
}

//--------------------------------------------------------------------------
bool GLShader::onUpdate()
{
    return false;
}

//--------------------------------------------------------------------------
void GLShader::onDeactivate() {}

//--------------------------------------------------------------------------
void GLShader::onRelease()
{
    glCheck(glDeleteShader(m_handle));
    m_code.clear();
    m_error.clear();
}

//--------------------------------------------------------------------------
bool GLShader::checkCompilationStatus(GLuint obj)
{
    GLint status;

    glCheck(glGetShaderiv(obj, GL_COMPILE_STATUS, &status));
    if (GL_FALSE == status)
    {
        GLint length;
        glCheck(glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length));
        std::vector<char> log(static_cast<size_t>(length + 1));
        glCheck(glGetShaderInfoLog(obj, length, &length, &log[0U]));

        std::string msg =
            "Failed compiling shader " + name() + ". Reason was: ";
        concatError(msg);
        concatError(&log[0U]);
    }
    else
    {
        m_error.clear();
    }
    return status;
}

//--------------------------------------------------------------------------
void GLShader::throw_if_not_loaded()
{
    if (m_code.empty())
    {
        throw GL::Exception("No code attached to the shader");
    }
}

//--------------------------------------------------------------------------
void GLShader::throw_if_already_compiled()
{
    if (compiled())
    {
        throw GL::Exception("Shader already compiled");
    }
}

//--------------------------------------------------------------------------
void GLShader::concatError(std::string const& msg)
{
    std::cerr << msg << std::endl;
    if (!m_error.empty())
    {
        m_error += '\n';
    }
    m_error += msg;
}
