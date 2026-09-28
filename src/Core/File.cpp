//=====================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "Compages/Core/File.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

//------------------------------------------------------------------------------
bool File::readAllFile(std::string const& p_filename, std::string& p_buffer)
{
    std::ifstream infile(p_filename, std::ifstream::in);
    if (!infile.is_open())
    {
        std::cerr << "Failed open file '" << p_filename
                  << "': " << std::strerror(errno) << '\n';
        return false;
    }

    infile.seekg(0, std::ios::end);
    std::streampos const pos = infile.tellg();
    if (pos <= 0)
    {
        p_buffer.clear();
        return pos == 0;
    }

    p_buffer.resize(static_cast<std::size_t>(pos));
    infile.seekg(0, std::ios::beg);
    infile.read(p_buffer.data(), static_cast<std::streamsize>(p_buffer.size()));
    if (!infile.good())
    {
        std::cerr << "Failed reading the whole file '" << p_filename
                  << "': " << std::strerror(errno) << '\n';
        p_buffer.clear();
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
std::string File::generateTempFileName(std::string const& p_root_path,
                                       std::string const& p_extension)
{
    try
    {
        std::chrono::zoned_time const local{
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::seconds>(
                std::chrono::system_clock::now())
        };

        std::string path = p_root_path;
        path += std::format("{:%Y-%m-%d/}", local);
        path += std::format("{:%H}", local);
        path += 'h';
        path += std::format("-{:%M}", local);
        path += 'm';
        path += std::format("-{:%S}", local);
        path += 's';
        path += p_extension;
        return path;
    }
    catch (std::format_error const&)
    {
        return p_root_path + p_extension;
    }
    catch (std::runtime_error const&)
    {
        return p_root_path + p_extension;
    }
}

//------------------------------------------------------------------------------
bool File::mkdir(std::string_view const& p_path, mode_t p_mode)
{
    struct stat st{};
    auto iter = p_path.begin();
    auto const end = p_path.end();

    while (iter != end)
    {
        auto const new_iter = std::find(iter, end, '/');
        std::string const new_path =
            '/' + std::string(p_path.begin(), new_iter);

        if (stat(new_path.c_str(), &st) != 0)
        {
            if ((::mkdir(new_path.c_str(), p_mode) != 0) && (errno != EEXIST))
            {
                std::cerr << "cannot create folder [" << new_path
                          << "]: " << std::strerror(errno) << '\n';
                return false;
            }
        }
        else if ((st.st_mode & S_IFDIR) == 0)
        {
            errno = ENOTDIR;
            std::cerr << "path [" << new_path << "] is not a directory\n";
            return false;
        }

        iter = new_iter;
        if (new_iter != end)
        {
            ++iter;
        }
    }
    return true;
}
