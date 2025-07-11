# Compilation/Installation

OpenGLCppWrapper is mainly made for Linux but it also works for OS X (while I
can hardly test on it because I have to borrow a MacBook).  OpenGLCppWrapper is
yet working for Windows because I do not have windows to develop with.

Note: In this doc the `-j8` with the `make` command is the number of your CPU
cores. Please adapt this number to your case.

## Prerequisite

You need to install the following libs on your operating system: glfw glew.
- Ubuntu, Debian: `sudo apt-get update && apt-get install libglew-dev libglfw3-dev libbz2-dev`
- Mac OS X: `brew install glfw glew`

## Compilation of the API

To download the project, its external libraries and compile the API with its examples:

```sh
git clone --recurse-submodules https://github.com/Lecrapouille/OpenGLCppWrapper.git --depth=1
cd OpenGLCppWrapper
make download-external-libs
make compile-external-libs
make -j8
```

The `--recurse-submodules` is important to get my Makefile routines for compiling the project.

If you are a developper `make download-external-libs` and `make compile-external-libs`
has to be called once or when you want to upgrade the libraries: they follow the
master branch and they remove the previously downloaded third-parts.

The `make download-external-libs` command plays the same role than a recursive git clone because
I hate git submodules: it always make things painful and `repo`
with its manifests is a bit overkill, so I prefer Makefile rules or script
shell and this gave me good result even with continuous integration.

After `make` a `build/` folder shall have been created containing the compiled
and runnable files. Two libraries (one static the second dynamic) shall also be
present. You can use them for your project. You can type `sudo make install` to
install them on your system.

If, after that, you want to modify code source, just do `make`. You can type
`make help` for displaying rules.

## Compilation of examples

To compile API examples:
```sh
cd examples
make -j8
```

A `build/` folder shall have been created containing the compiled and runnable
files. Run examples:
```sh
./build/OpenGLExamples
```

Will display the list of possible examples. Type `./build/OpenGLExamples 0` for
example for running the 1st example.

## Installation

Multiple versions of this library can coexist thanks to their versioning number.
After the compilation, just type:

```sh
cd OpenGLCppWrapper
sudo make install
```

This will install:
* in `/usr/lib`: the static and shared libraries libOpenGLCppWrapper.
* in `/usr/include/openglcppwrapper-<version>`: all headers files (hpp).
* in `/usr/lib/pkgconfig`: a pkg confile file for linking this API with your future projects.
* in `/usr/share/OpenGLCppWrapper/<version>/`: documentation, examples and other files.

If you do not like the default location, Pass to Makefile options `DESTDIR`,
`PREFIX` and `BINDIR` or directly edit the file `.makefile/Makefile.header`
(note: touching a makefile will force to recompile the whole project).

Check the presence of libraries in your system:
```sh
cd /usr/lib
ls -la libOpenGLCppWrapper*
```
Or better:
```
echo `pkg-config `openglcppwrapper --libs``
```

## Developpers

### How to use OpenGLCppWrapper in your project?

Add the include in your cpp file: `# include <OpenGLCppWrapper/OpenGLCppWrapper.hpp>`

In debug mode, you can use the following macros:
* `-DCHECK_OPENGL` for checking bad parameters passed to OpenGL routines.
* `-DENABLE_DEBUG` for activating console logs for debugging.

Compile using the pkg-config tool to add compilation flags to GCC or clang:
* `CFLAGS=`pkg-config openglcppwrapper --cflags``
* `LDFLAGS=`pkg-config openglcppwrapper --libs``
* Note: in pkg-config instead of `openglcppwrapper` which refers to the last installed version,
you can specify your desired version (for example: `openglcppwrapper-0.6`).

Example with a simple file main.cpp:
```
g++ -W -Wall --std=c++11 main.cpp -o prog `pkg-config openglcppwrapper --cflags --libs`
```

**note:** You may need indicate where are shared libraries. For example on Mac OS X:

```sh
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:/your/path/to/your/lib/folder
```

Example with Makefile given in [here](https://github.com/Lecrapouille/LinkAgainstMyLibs/tree/master/OpenGL).

### What code to write in my project ?

* [Tutorial](tutorials/Core.md)
* [Examples](../examples/README.md)
* [Architecture](Architecture.md)
* [Debug OpenGL](Traces.md)

### Doxygen

Documentation of the code source can be found [here](https://lecrapouille.github.io/OpenGLCppWrapper.github.io/).
It can be localy generated as `doc/html/index.html` by typing `make doc`.

### Unit tests

If you want to help developing and write/launch unit tests. You have to
download, compile and install [googletest](https://github.com/google/googletest)
manually:

```sh
wget https://github.com/google/googletest/archive/release-1.11.0.tar.gz
tar xf release-1.11.0.tar.gz
cd googletest-release-1.11.0
cmake .
sudo make install -j8
```

To compile and run tests and get code coverage report, call:

```sh
cd OpenGLCppWrapper/tests
make coverage
```

If all tests passed, a coverage report is created inside `doc/coverage/` and the
`index.html` is opened automatically.

### Third-parts

This project depends on third-parts that are automatically downloaded with
`make download-external-libs`. They are compiled as static libraries with
`make compile-external-libs` but they are not installed on your operating
system. It's onnly when the `sudo make install` is called that their header
files are copied within the OpenGLCppWrapper header files.

Here the list of third-parts:
* [MyMakefile](https://github.com/Lecrapouille/MyMakefile) my GitHub Makefile
  project used for compiling all my GitHub projects. It's automatically cloned
  as a git submodule. You have nothing to do except not to forget to add the
  option `--recurse-submodules` when you git clone the project.
* [OpenGLCppWrapper-data](https://github.com/Lecrapouille/OpenGLCppWrapper-data)
  this repo contains all textures needed for examples. This repo is
  automatically downloaded the Makefile. You have nothing to do.
* [SOIL](https://github.com/kbranigan/Simple-OpenGL-Image-Library.git) this
  library is used for loading picture files (jpeg, png ...). This library is
  downloaded by the Makefile rule `download-external-libs`.
* [Dear IMgui](https://github.com/ocornut/imgui) this library is used for adding
  widgets to your applications. This library is downloaded by the Makefile rule
  `download-external-libs`.
* [bullet3](https://github.com/bulletphysics/bullet3) this library is used for
  Physic engine. This library is downloaded by the Makefile rule
  `download-external-libs`.
* [units](https://github.com/nholthaus/units) this library is used for SI units.
  This library is downloaded by the Makefile rule `download-external-libs`.
* [json](https://github.com/nlohmann/json) this library is used for reading JSON.
  This library is downloaded by the Makefile rule `download-external-libs`.
* [backward-cpp](https://github.com/bombela/backward-cpp), this library is used
  for displaying the stack when a crash occured. It is used on debug mode. This
  library is downloaded by the Makefile rule `download-external-libs`.
* glfw glew libraries for OpenGL.
