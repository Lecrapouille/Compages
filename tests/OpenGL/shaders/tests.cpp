#include "../../Common/File.hpp"
#include "../../Common/Path.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <regex>
#include <string>

class GLShader
{
public:

    std::string m_code;
    std::string m_error;
    std::string m_file_name;
    Path path;

    GLShader() : path(".:include") {}

    bool read(std::string const& path, std::string& code)
    {
        std::string msg;
        std::ifstream infile;
        m_file_name = path;

        // Open shader file
        errno = 0;
        infile.open(path);
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
                    msg = "Failed reading file '" + path + "'. Reason was '" +
                          ::strerror(errno) + "'";
                    std::cerr << msg << std::endl;
                    // concatError(msg);
                    return false;
                }
            }
            else
            {
                msg = "Failed reading file '" + path +
                      "'. Reason was 'cannot get the file size'";
                std::cerr << msg << std::endl;
                // concatError(msg);
                return false;
            }
        }
        else
        {
            msg = "Failed opening file '" + path + "'. Reason was '" +
                  ::strerror(errno) + "'";
            std::cerr << msg << std::endl;
            // concatError(msg);
            return false;
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    bool qq()
    {
        std::regex regex(R"(^\s*#\s*include\s+([\w\/.]+))");
        std::cmatch matches;

        std::istringstream code(m_code);
        std::string new_code;
        std::string line;
        std::list<std::string> opened_files;

        bool changed;
        do
        {
            new_code = "";
            changed = false;
            // std::cout << "New code " << m_file_name << ": [" << code.str() <<
            // "]" << std::endl;
            while (std::getline(code, line))
            {
                // std::cout << "Line: [" << line << "]" << std::endl;
                std::regex_search(line.c_str(), matches, regex);
                if (matches.empty())
                {
                    new_code += line;
                    new_code += '\n';
                    // std::cout << "ADD LINE: [" << new_code << "]" <<
                    // std::endl;
                }
                else
                {
                    // Found a file to include, read its whole content and
                    // append it to the code
                    std::string full_path = path.expand(matches.str(1));
                    m_file_name = File::fileName(full_path);
                    if (opened_files.end() == std::find(opened_files.begin(),
                                                        opened_files.end(),
                                                        m_file_name))
                    {
                        // std::cout << "Add to opened files: " << m_file_name
                        // << std::endl;
                        opened_files.push_back(m_file_name);
                        if (!read(full_path, new_code))
                            return false;
                        // std::cout << "Replaced by new code: [" << new_code <<
                        // "]" << std::endl;

                        // By security add the character on the same line after
                        // "include foo"
                        std::string eol = line.substr(matches.position(0) +
                                                      matches.str(0).size());
                        if (!eol.empty())
                        {
                            new_code += '\n';
                            // std::cout << "EOL:" << eol << std::endl;
                            new_code += eol;
                        }
                        changed = true;
                    }
                    else
                    {
                        // std::cout << full_path << " already included" <<
                        // std::endl; std::cout << "Code: [" << new_code << "]"
                        // << std::endl; std::cout << "Line: [" << line << "]"
                        // << std::endl;
                    }
                }
            }

            // Reparse the shader code to keep searching potential token
            // 'include' added by the included file.
            if (changed)
            {
                // std::cout << "changed" << std::endl;
                code.clear();
                code.str(new_code);
            }
        } while (changed);

        m_code = new_code;
        return true;
    }
};

// g++ -W -Wall --std=c++11 tests.cpp -I../.. ../../Common/Path.cpp
// ../../Common/File.cpp -o prog
int main()
{
    GLShader s;

    if (!s.read("shader.vs", s.m_code))
    {
        std::cout << "pas cool: '" << s.m_code << "'" << std::endl;
        return 1;
    }
    s.qq();
    std::cout << "cool: '" << s.m_code << "'" << std::endl;

    return 0;
}
