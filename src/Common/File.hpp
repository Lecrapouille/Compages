//=====================================================================
// SimTaDyn: A GIS in a spreadsheet.
// Copyright 2018 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of SimTaDyn.
//
// SimTaDyn is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SimTaDyn.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef FILE_HPP_
#define FILE_HPP_

#include <algorithm>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

// **************************************************************
//! \brief
// **************************************************************
class File
{
public:

    enum FileType
    {
        DoesNotExist,
        Directory,
        Document,
        UnknownType
    };

    //------------------------------------------------------------------
    //! \brief Check if a file exits.
    //!
    //! Beware of race condition ! Do not use this function for opening
    //! a file but use instead the open() function and check the error
    //! code. Indeed, the file may be deleted between this function and
    //! the open() function because theses two functions will not be
    //! atomic.
    //------------------------------------------------------------------
    inline static bool exist(std::string const& path)
    {
        struct stat buffer;
        return 0 == stat(path.c_str(), &buffer);
    }

    //------------------------------------------------------------------
    //! \brief Return the type of file
    //------------------------------------------------------------------
    inline static FileType type(std::string const& path)
    {
        struct stat buffer;
        if (0 == stat(path.c_str(), &buffer))
        {
            if (buffer.st_mode & S_IFDIR)
            {
                return Directory;
            }
            else if (buffer.st_mode & S_IFREG)
            {
                return Document;
            }
            return UnknownType;
        }
        return DoesNotExist;
    }

    //------------------------------------------------------------------
    //! \brief Return if a directory or a file is readable.
    //------------------------------------------------------------------
    inline static bool isReadable(std::string const& path)
    {
        return 0 == access(path.c_str(), R_OK);
    }

    //------------------------------------------------------------------
    //! \brief Return if a directory or a file is writable.
    //------------------------------------------------------------------
    inline static bool isWritable(std::string const& path)
    {
        return 0 == access(path.c_str(), W_OK);
    }

    //------------------------------------------------------------------
    //! \brief Read the whole file and store its content as string.
    //------------------------------------------------------------------
    static bool readAllFile(std::string const& filename, std::string& buffer);

    //------------------------------------------------------------------
    //! \brief give the file name with its extension from a given path
    //------------------------------------------------------------------
    inline static std::string fileName(std::string const& path)
    {
        std::string::size_type pos = path.find_last_of("\\/");
        if (pos != std::string::npos)
            return path.substr(pos + 1, std::string::npos);
        return path;
    }

    //------------------------------------------------------------------
    //! \brief give the file name without its extension from a given path
    //------------------------------------------------------------------
    inline static std::string baseName(std::string const& path)
    {
        std::string filename = fileName(path);
        return filename.substr(0, filename.find_last_of("."));
    }

    //------------------------------------------------------------------
    //! \brief give the file extension
    //------------------------------------------------------------------
    inline static std::string extension(std::string const& path)
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

    //------------------------------------------------------------------
    //! \brief
    //! \note the path is considered as referencing a file not a directory
    //! execpt if a '/' is set Example dirName("/tmp/") will return
    //! dirName("/tmp/") but dirName("/tmp") will return dirName("/")
    //------------------------------------------------------------------
    inline static std::string dirName(std::string const& path)
    {
        std::string::size_type pos = path.find_last_of("\\/");
        if (pos == path.length() - 1)
        {
            return path;
        }
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
        return "";
    }

    //------------------------------------------------------------------
    //! \brief Generate the name of a temporary file or directory.
    //! The name is made with the current date. There is no garanty that
    //! directory does not already exists, therefore you have to check it
    //! before calling mkdir().
    //------------------------------------------------------------------
    static std::string generateTempFileName(std::string const& root_path,
                                            std::string const& extension = "")
    {
        char buffer_time[32];
        std::string path(root_path);
        time_t current_time = time(nullptr);

        strftime(buffer_time,
                 sizeof(buffer_time),
                 "%Y-%m-%d/",
                 localtime(&current_time));
        path += buffer_time;

        strftime(buffer_time,
                 sizeof(buffer_time),
                 "%Hh-%Mm-%Ss",
                 localtime(&current_time));
        path += buffer_time;

        // Optionaly: file extension
        path += extension;

        return path;
    }

    //------------------------------------------------------------------
    //! \brief Create a path of directory.
    //! \param last element of path shall not be considerate as a file
    //! but as a directory.
    //------------------------------------------------------------------
    static bool mkdir(std::string const& path,
                      mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO)
    {
        struct stat st;
        std::string::const_iterator begin = path.begin();
        std::string::const_iterator end = path.end();
        std::string::const_iterator iter = begin;

        while (iter != end)
        {
            std::string::const_iterator new_iter = std::find(iter, end, '/');
            std::string new_path = '/' + std::string(begin, new_iter);

            if (stat(new_path.c_str(), &st) != 0)
            {
                if ((::mkdir(new_path.c_str(), mode) != 0) && (errno != EEXIST))
                {
                    std::cout << "cannot create folder [" << new_path
                              << "] : " << strerror(errno) << std::endl;
                    return false;
                }
            }
            else if (!(st.st_mode & S_IFDIR))
            {
                errno = ENOTDIR;
                std::cout << "path [" << new_path << "] not a dir "
                          << std::endl;
                return false;
            }
            else
            {
                // std::cout << "path [" << new_path << "] already exists " <<
                // std::endl;
            }

            iter = new_iter;
            if (new_iter != path.end())
            {
                ++iter;
            }
        }
        return true;
    }
};

#endif /* FILE_HPP_ */
