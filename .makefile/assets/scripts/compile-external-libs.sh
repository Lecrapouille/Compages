#!/bin/bash -e
##==================================================================================
## MIT License
##
## Copyright (c) 2019-2025 Quentin Quadrat <lecrapouille@gmail.com>
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##==================================================================================

### This script is called by (cd .. && make compile-external-libs). It will
### compile third-party libraries cloned previously with make download-external-libs.
###
### To avoid polluting the system, these libraries are not installed on your
### operating system (no sudo make install is called). As a consequence, you have to tell
### your project ../Makefile where to find their files.
###
### The last important point to avoid pollution: it is better to compile third-party
### libraries as static libraries rather than shared libraries to avoid telling your
### system where to find them when you'll start your application.
###
### Input of this script:
### $1: target (your application name)
### $2: script to include defining what to compile.
### $3: architecture (i.e. Linux, Darwin, Windows, Emscripten)
### $4: CC (C compiler)
### $5: CXX (C++ compiler)

function fatal
{
    echo -e "\033[35mFailure! $1\033[00m"
    exit 1
}

if [ "$#" -ne 5 ]; then
    fatal "Illegal number of parameters"
fi

PROJECT_NAME=$1
SCRIPT=$2
OS=$3

### Compilation flags if not using Emscripten
if [ "$OS" != "Emscripten" ]; then
    CC="$4"
    CXX="$5"
    export CXX=$CXX
    export CC=$CC
fi

### Number of CPU cores
NPROC=
if [ "$OS" == "Darwin" ]; then
    NPROC=`sysctl -n hw.logicalcpu`
else
    NPROC=`nproc`
fi

function print-compile
{
    THIRD_PARTIES_NAME=$1
    echo -e "\033[35m*** Compiling:\033[00m \033[36m$PROJECT_NAME\033[00m <= \033[33m$THIRD_PARTIES_NAME\033[00m"
    if [ ! -e $1 ];
    then
        echo "Failed compiling external/$PROJECT_NAME: directory does not exist"
    fi
}

function call-configure
{
    if [ "$OS" == "Emscripten" ]; then
        emconfigure ./configure $*
    else
        ./configure $*
    fi
}

function call-cmake
{
    if [ "$OS" == "Emscripten" ]; then
        emcmake cmake $*
    else
        cmake $*
    fi
}

function call-make
{
    if [ "$OS" == "Emscripten" ]; then
        VERBOSE=1 emmake make -j$NPROC $*
    else
        VERBOSE=1 make -j$NPROC $*
    fi
}

source $SCRIPT
