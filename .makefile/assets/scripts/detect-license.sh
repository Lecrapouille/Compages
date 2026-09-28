#!/bin/bash -e
##==================================================================================
## GPL v3 License
##
## Copyright (c) 2019-2025 Quentin Quadrat <lecrapouille@gmail.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <https://www.gnu.org/licenses/>.
##==================================================================================

### Detect the license type from a LICENSE file by parsing text patterns.
### This script attempts to identify the license by searching for keywords
### and returns an SPDX short identifier.
###
### Arguments:
### $1: path to the project root directory

PROJECT_ROOT="$1"
LICENSE_FILE="$PROJECT_ROOT/LICENSE"

# Check if LICENSE file exists
if [ ! -f "$LICENSE_FILE" ]; then
    echo "UNKNOWN"
    exit 1
fi

# Read first 200 lines to analyze license
LICENSE_CONTENT=$(head -n 200 "$LICENSE_FILE" 2>/dev/null)

# GPL v3
if echo "$LICENSE_CONTENT" | grep -qi "GNU GENERAL PUBLIC LICENSE" && \
   echo "$LICENSE_CONTENT" | grep -qi "Version 3"; then
    echo "GPL-3.0"
    exit 0
fi

# GPL v2
if echo "$LICENSE_CONTENT" | grep -qi "GNU GENERAL PUBLIC LICENSE" && \
   echo "$LICENSE_CONTENT" | grep -qi "Version 2"; then
    echo "GPL-2.0"
    exit 0
fi

# LGPL v3
if echo "$LICENSE_CONTENT" | grep -qi "GNU LESSER GENERAL PUBLIC LICENSE" && \
   echo "$LICENSE_CONTENT" | grep -qi "Version 3"; then
    echo "LGPL-3.0"
    exit 0
fi

# LGPL v2.1
if echo "$LICENSE_CONTENT" | grep -qi "GNU LESSER GENERAL PUBLIC LICENSE" && \
   echo "$LICENSE_CONTENT" | grep -qi "Version 2.1"; then
    echo "LGPL-2.1"
    exit 0
fi

# MIT License
if echo "$LICENSE_CONTENT" | grep -qi "MIT License" && \
   echo "$LICENSE_CONTENT" | grep -qi "Permission is hereby granted, free of charge"; then
    echo "MIT"
    exit 0
fi

# Apache License 2.0
if echo "$LICENSE_CONTENT" | grep -qi "Apache License" && \
   echo "$LICENSE_CONTENT" | grep -qi "Version 2.0"; then
    echo "Apache-2.0"
    exit 0
fi

# BSD 3-Clause
if echo "$LICENSE_CONTENT" | grep -qiE "BSD" && \
   echo "$LICENSE_CONTENT" | grep -qi "3-Clause"; then
    echo "BSD-3-Clause"
    exit 0
fi

# BSD 2-Clause
if echo "$LICENSE_CONTENT" | grep -qiE "BSD" && \
   echo "$LICENSE_CONTENT" | grep -qi "2-Clause"; then
    echo "BSD-2-Clause"
    exit 0
fi

# If we get here, we couldn't detect the license
echo "UNKNOWN"
exit 1

