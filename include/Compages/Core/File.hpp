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

#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace compages::core
{
// ****************************************************************************
//! \brief Static helpers for paths, filesystem checks, and file I/O.
// ****************************************************************************
class File
{
public:

    File() = delete;
    File(File const&) = delete;
    File& operator=(File const&) = delete;

    enum class Type
    {
        DoesNotExist,
        Directory,
        Document,
        UnknownType
    };

    // ------------------------------------------------------------------
    //! \brief Check if a file exists.
    //!
    //! Do not use this to gate \c open(): the path may disappear before
    //! open runs (TOCTOU). Prefer opening and handling the error.
    // ------------------------------------------------------------------
    [[nodiscard]] static bool exist(std::string const& p_path)
    {
        struct stat buffer{};
        return stat(p_path.c_str(), &buffer) == 0;
    }

    // ------------------------------------------------------------------
    //! \brief Return the type of file at \c p_path.
    // ------------------------------------------------------------------
    [[nodiscard]] static Type type(std::string const& p_path)
    {
        struct stat buffer{};
        if (stat(p_path.c_str(), &buffer) != 0)
        {
            return Type::DoesNotExist;
        }
        if ((buffer.st_mode & S_IFDIR) != 0)
        {
            return Type::Directory;
        }
        if ((buffer.st_mode & S_IFREG) != 0)
        {
            return Type::Document;
        }
        return Type::UnknownType;
    }

    // ------------------------------------------------------------------
    //! \brief Whether a directory or file is readable.
    // ------------------------------------------------------------------
    [[nodiscard]] static bool isReadable(std::string const& p_path)
    {
        return access(p_path.c_str(), R_OK) == 0;
    }

    // ------------------------------------------------------------------
    //! \brief Whether a directory or file is writable.
    // ------------------------------------------------------------------
    [[nodiscard]] static bool isWritable(std::string const& p_path)
    {
        return access(p_path.c_str(), W_OK) == 0;
    }

    //! \brief Read the whole file into \c p_buffer.
    [[nodiscard]] static bool readAllFile(std::string const& p_filename,
                                          std::string& p_buffer);

    // ------------------------------------------------------------------
    //! \brief File name with extension from a path.
    // ------------------------------------------------------------------
    [[nodiscard]] static std::string fileName(std::string const& p_path)
    {
        std::string::size_type const pos = p_path.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            return p_path.substr(pos + 1);
        }
        return p_path;
    }

    // ------------------------------------------------------------------
    //! \brief File name without extension from a path.
    // ------------------------------------------------------------------
    [[nodiscard]] static std::string baseName(std::string const& p_path)
    {
        std::string const filename = fileName(p_path);
        std::string::size_type const dot = filename.find_last_of('.');
        if (dot != std::string::npos)
        {
            return filename.substr(0, dot);
        }
        return filename;
    }

    // ------------------------------------------------------------------
    //! \brief Lower-case extension without leading dot.
    // ------------------------------------------------------------------
    [[nodiscard]] static std::string extension(std::string_view const& p_path)
    {
        std::string::size_type const pos = p_path.find_last_of('.');
        if (pos == std::string::npos)
        {
            return {};
        }

        std::string ext(p_path.substr(pos + 1));
        if (!ext.empty() && ext.back() == '~')
        {
            ext.pop_back();
        }

        for (char& ch : ext)
        {
            ch =
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return ext;
    }

    // ------------------------------------------------------------------
    //! \brief Directory part of a path (includes trailing slash when present).
    //!
    //! \c dirName("/tmp/") returns \c "/tmp/"; \c dirName("/tmp") returns
    //! \c "/".
    // ------------------------------------------------------------------
    [[nodiscard]] static std::string dirName(std::string const& p_path)
    {
        if (p_path.empty())
        {
            return {};
        }

        std::string::size_type const pos = p_path.find_last_of("\\/");
        if (pos == std::string::npos)
        {
            return {};
        }
        if (pos == p_path.length() - 1)
        {
            return p_path;
        }
        return p_path.substr(0, pos + 1);
    }

    // ------------------------------------------------------------------
    //! \brief Path under \c p_root_path from the current local time.
    //!
    //! Not guaranteed unique; check before \c mkdir().
    // ------------------------------------------------------------------
    [[nodiscard]] static std::string
    generateTempFileName(std::string const& p_root_path,
                         std::string const& p_extension = {});

    // ------------------------------------------------------------------
    //! \brief Create missing directories along \c p_path.
    //!
    //! The last segment is treated as a directory name, not a file.
    // ------------------------------------------------------------------
    [[nodiscard]] static bool mkdir(std::string_view const& p_path,
                                    mode_t p_mode = S_IRWXU | S_IRWXG |
                                                    S_IRWXO);
};

} // namespace compages::core

