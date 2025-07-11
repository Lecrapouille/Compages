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

#include "File.hpp"
#include <fstream>
#include <stdio.h>

//------------------------------------------------------------------
//! \param filename the file path to read.
//! \param the buffer receiving the content of the file.
//! \return true if success, else false.
//------------------------------------------------------------------
bool File::readAllFile(std::string const& filename, std::string& buffer)
{
    std::ifstream infile;
    bool res = false;

    // Read the shader code from a file
    infile.open(filename, std::ifstream::in);
    if (infile.is_open())
    {
        infile.seekg(0, std::ios::end);
        std::streampos pos = infile.tellg();
        if (pos > 0)
        {
            buffer.resize(static_cast<size_t>(pos));
            infile.seekg(0, std::ios::beg);
            infile.read(&buffer[0U],
                        static_cast<std::streamsize>(buffer.size()));
            res = infile.good();
            if (!res)
            {
                printf("Failed reading the whole file '%s'. Reason is '%s'\n",
                       filename.c_str(),
                       strerror(errno));
            }
        }
        infile.close();
    }
    else
    {
        printf("Failed open file '%s'. Reason is '%s'\n",
               filename.c_str(),
               strerror(errno));
    }

    return res;
}
