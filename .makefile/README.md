# MyMakefile

[MyMakefile](https://github.com/Lecrapouille/MyMakefile) is a build system based on GNU Make for C++ projects. It eliminates the need to write complex Makefile rules from scratch by offering plenty of rules and macros.

**Note:** I no longer develop in C but MyMakefile should also work.

## 🤔 Why MyMakefile?

While CMake is a popular choice, MyMakefile serves as an alternative to CMake for medium-sized projects. Indeed, it offers several advantages for smaller C++ projects:

- **Simplicity**: No complex CMake syntax or its obscure function names.
- **Efficiency**: Smaller code you have to write, more readable Makefiles.
- **Modern Features**: Call engineer tools you usually need without the complexity.

**Limitations:** MyMakefile does not support multiple targets in a single Makefile file, but you can build a project that generates multiple libraries, standalone applications, and demos.

## 🌟 MyMakefile Goals

- Encapsulate all complex Makefile rules in two files that you can include in your project, handling:
  - C++ file compilation.
  - A small manifest file for downloading third-party libraries.
  - Compilation flag management.
  - Application bundling (including macOS bundles).
  - Shared/static library creation (contrary to CMake, both are created).
  - Installation on your system (header files, doc, libraries).
  - Creation/installation of pkg-config files.
  - Documentation generation (Doxygen).
  - Unit tests and code coverage.
  - RPM creation.
  - Code formatting (clang-format integration).
  - Static analysis (cppcheck integration).
  - call dev tools (cppcheck, clang-format, asan, benchmark, gprof, coverity-scan, OpenSuse Build System, )
- Define your project structure in just a few lines of Makefile syntax.
- Let MyMakefile handle all the complex build rules for you.

## 🌐 Cross-Platform Support

- 🐧 Linux
- 🍎 Mac OS
- ~~🪟 Windows~~ (not yet supported)
- 🌐 [Emscripten](https://emscripten.org)
- 🧸 [ExaequOS](https://www.exaequos.com)

## 🚀 Quick Start

1. **Include MyMakefile in your project**:

Either you copy directly this repo in your project or better to include it as submodule.

```bash
# Option 1: Copy-paste
cp -r MyMakefile your-project/.makefile

# Option 2: Git submodule (recommended)
git submodule add https://github.com/Lecrapouille/MyMakefile.git .makefile
```

The dot prefix allows you to hide MyMakefile in your project, but it's not mandatory. The folder name is up to you.

2. **Create your first Makefile**:

```makefile
# Relative location of your project root folder P and MyMakefile folder M
P := .
M := $(P)/.makefile

# Your minimal project definition
PROJECT_NAME := my-project
PROJECT_VERSION := 1.0.0
TARGET_NAME := my-app
TARGET_DESCRIPTION := brief explanation of the target

# Optional definitions
COMPILATION_MODE := release
CXX_STANDARD := --std=c++14

# Include MyMakefile project file after your project definition
include $(M)/project/Makefile

# Define what and how to compile your target
INCLUDES := $(P)/include $(P)/src
VPATH := $(P)/src
DEFINES :=
SRC_FILES += src/main.cpp
# For compiling a library, replace SRC_FILES with LIB_FILES

# Include MyMakefile rules file after your project configuration
include $(M)/rules/Makefile

# Optionally: add your custom Makefile rules here
```

First, you must define macros P and M. P indicates the relative path to the root project. M indicates where to find the MyMakefile folder.

The project definition is mandatory and allows you to define your project name and the target name (which is either a stand-alone application or library). You have to define a description to your target and a version to your project.

Compilation definitions are up to you: release, debug, unit test, and the C++ standard.

You must include the two MyMakefile files at the correct location in your Makefile. One sets variables for your project and the other defines Makefile rules. As a consequence, order and position matter.

Finally, you can give options to your compiler for finding C++ sources and header files, define project macros and the list of C++ files to compile.

3. **Build your project**:

```bash
make help                     # Show all available options
make [OPTIONS1]               # Start compilation
sudo make install [OPTIONS2]  # Install your project on your system
```

Like traditional makefile you can pass OPTIONS1 to makefile, for example which compiler you prefer (CXX). OPTIONS define where to install your project on your system.

## 📚 Projects Using MyMakefile

Here are some of my projects that use MyMakefile instead of CMake:

- 🎨 [TimedPetriNetEditor](https://github.com/Lecrapouille/TimedPetriNetEditor)
- 🎮 [OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox)
- 📦 [Zipper](https://github.com/Lecrapouille/zipper)
- 🛣️ [Highway](https://github.com/Lecrapouille/Highway)
- 🎮 [Compages](https://github.com/Lecrapouille/Compages)
- 🔬 [SimTaDyn](https://github.com/Lecrapouille/SimTaDyn)
- 🧮 [SimForth](https://github.com/Lecrapouille/SimForth)
- ♟️ [ChessNeuNeu](https://github.com/Lecrapouille/ChessNeuNeu)
- 🔗 [LinkAgainstMyLibs](https://github.com/Lecrapouille/LinkAgainstMyLibs)

**Note:** They may not refer to the HEAD of MyMakefile and may use old syntax.

## 📋 Prerequisites

- 🐚 Bash interpreter (MyMakefile requires some bash assistance)
- 🛠️ Optional tools, called by MyMakefile:
  - `g++` or `clang++` (for compilation).
  - `gcovr` (for code coverage).
  - `doxygen` (for documentation).
  - `git` (for downloading third-parties).

## 🔧 Compiling for MacOS

By default, the behavior will compile like done with Linux. To create Bundle applications, you have to add the following code:

```makefile
ifeq ($(OS),Darwin)
BUILD_MACOS_APP_BUNDLE := 1
APPLE_IDENTIFIER := lecrapouille
MACOS_BUNDLE_ICON := <your path>/<your icon name>.icns
endif
```

## 🔧 Compiling for Emscripten

Nothing to do, follow the Emscripten documentation for compiling a project. MyMakefile shall detect it and compile for Emscripten.

## 🔧 Compiling for ExaequOS

[ExaequOS](https://www.exaequos.com) is a fork of [Emscripten](https://emscripten.org) and better alternative (POSIX compliant). MyMakefile also supports the compilation for this platform.

- Install the ExaequOS Docker image:

```bash
# Follow instructions at https://github.com/Lecrapouille/docker-exa
```

- Run the ExaequOS Docker image against your project folder.
- Simply run `make` - your project will compile like a native Linux application.
- After compilation, run `make install`
- Open [https://www.exaequos.com/](https://www.exaequos.com) in your browser.
- In Havoc (ExaequOS console), run:

```bash
/media/localhost/<your-application>
```

## 📦 Compiling to RPM

```bash
make rpm
```

## Compiling standalone applications

Use, `SRC_FILES` to set the local paths to C++ files you want to compile. For example `SRC_FILES := $(call rwildcard,$(CURRENT_DIR),*.cpp)`.

## Compiling an internal library

You can mix with `LIB_FILES` instead of `SRC_FILES`. One library managed by Makefile.

## Compiling standalone applications using several internal libraries

You have to set:

- Use the macro `$(call internal-lib,xxx)` to set a library.
- Set `INTERNAL_LIBS` with the list of libraries previously defined.
- Set `DIRS_WITH_MAKEFILE` the list of folder in which there is a Makefile to compile the lib.
- Set the order of library compilation with rules using paths (not library names).

Example

```makefile
LIB_FOO := $(call internal-lib,foo)
LIB_BAR := $(call internal-lib,my-project-bar)
INTERNAL_LIBS := $(LIB_FOO) $(LIB_BAR)
DIRS_WITH_MAKEFILE := $(P)/src/path/to/foo $(P)/src/path/to/bar
$(P)/src/path/to/foo $(P)/src/path/to/bar
```

We suppose that a Makefile exists in `$(P)/src/path/to/foo` and in `$(P)/src/path/to/bar`.

## Pre and Post compilation

You can use `pre-build::` and `post-build::` goals. Place them after `include $(M)/rules/Makefile`. You can compile it to demos and examples.

## Using third-parties libraries

Create a folder named `external` create a file named `manifest` with list of GitHub repos to clone. For example:

```txt
zeux/pugixml
ocornut/imgui@docking
```

To download them, call in the console:

```bash
make download-external-libs
```

The `external` folder will be populated.

In Makefile: use the variable `$(THIRD_PARTIES_DIR)` to refer to the `external` folder. For example:

```makefile
INCLUDES += $(THIRD_PARTIES_DIR)/imgui
VPATH += $(THIRD_PARTIES_DIR)/imgui
```

## Orchestrator Makefile

This Makefile does not compile `SRC_FILES` or `LIB_FILES` files but only call other Makefiles that will be used to compile internal lib, demos, ...

## 📖 Documentation

- The `help` rule will display you all variables you can override and prebuilt rules you can call. Note that rules with `::` allow you to append your own commands.
- For detailed API documentation, see [API.md](doc/API.md).

```bash
make help
Usage:
  [VERBOSE=1] make [flags...] <target>

You can override the following flags:

  AUTHOR                                                                Auto-detect author from git config or system user
    Default value: $(shell git config --global user.name 2>/dev/null                                                                                                       echo $(USER))
  ECHO                                                                  Echo command for colors
    Default value: echo
  ECHO_COLOR                                                            Echo command for colors
    Default value: $(ECHO) -e
  DESTDIR                                                               Define where the project will be installed: $(DESTDIR)$(PREFIX)/lib and $(DESTDIR)$(PREFIX)/bin.
    Default value:
  PREFIX                                                                Define where the project will be installed: $(DESTDIR)$(PREFIX)/lib and $(DESTDIR)$(PREFIX)/bin
    Default value: /usr/local
  INCLUDEDIR                                                            Define where to install includes.
    Default value: $(PREFIX)/include
  LIBDIR                                                                Define where to install libraries.
    Default value: $(PREFIX)/lib
  PKGLIBDIR                                                             Define where to install pkgconfig files.
    Default value: $(LIBDIR)/pkgconfig
  DATADIR                                                               Define where to install data and documentation.
    Default value: $(PREFIX)/share
  BINDIR                                                                Define where to install standalone applications.
    Default value: $(PREFIX)/bin
  TMPDIR                                                                Define where to you can store temporary files.
    Default value: /tmp
  BUILD                                                                 Name of the directory holding the compilation artifacts.
    Default value: build
  THIRD_PARTIES_FOLDER_NAME                                             Name of the directory holding third parties libraries (external libraries).
    Default value: external
  DOC_FOLDER_NAME                                                       Name of the directory holding documentation.
    Default value: doc
  PROJECT_GENERATED_FOLDER_NAME                                         Name of the directory holding generated reports.
    Default value: doc
  GENERATED_DOXYGEN_DIR                                                 Directory where Doxygen is generated
    Default value: $(PROJECT_GENERATED_DOC_DIR)/doxygen
  GPROF_ANALYSIS                                                        Path of the generated profiling analysis
    Default value: $(PROJECT_GENERATED_DOC_DIR)/profiling/analysis.txt
  COVERAGE_DIR                                                          Path of the generated code coverage report
    Default value: $(PROJECT_GENERATED_DOC_DIR)/code-coverage
  COVERAGE_HTML_RAPPORT                                                 Path of the generated code coverage report
    Default value: $(COVERAGE_DIR)/coverage.html
  COVERAGE_LCOV_RAPPORT                                                 Path of the generated code coverage report
    Default value: $(COVERAGE_DIR)/lcov.info
  DATA_FOLDER_NAME                                                      Define the directory holding project data.
    Default value: data
  PROJECT_TESTS                                                         Define the directory holding unit tests.
    Default value: tests
  MACOS_BUNDLE_ICON                                                     Path to the default icon for MacOS bundle applications.
    Default value: $(M)/assets/$(DATA_FOLDER_NAME)/macos.icns
  PATH_PROJECT_LOGO                                                     Path to the default icon for MacOS bundle applications.
    Default value: $(abspath $(M)/assets/icons/logo.png)
  DOXYGEN_INPUTS                                                        Path to the default icon for MacOS bundle applications.
    Default value: $(abspath $(P)/README.md $(P)/src $(P)/include)
  PROJECT_TEMP_DIR                                                      Path where to store temporary project files.
    Default value: $(TMPDIR)/$(PROJECT_NAME)/$(PROJECT_VERSION)
  PROJECT_OTHER_FILES                                                    Path where to store temporary project files.
    Default value: AUTHORS LICENSE README.md ChangeLog VERSION
  REGEXP_CXX_HEADER_FILES                                               Path where to store temporary project files.
    Default value: *.hpp *.ipp *.tpp *.hh *.h *.hxx *.incl
  PREVENT_SHARED_LIB_UNLOAD                                             Use nodelete flag for shared libraries
    Default value:
  CUSTOM_RPATH                                                          Custom runtime library search paths (rpath) - colon separated
    Default value:
  DO_NOT_COMPILE_STATIC_LIB                                             Do not compile static library
    Default value:
  DO_NOT_COMPILE_SHARED_LIB                                             Do not compile shared library
    Default value:
  LICENSE                                                               Do not compile shared library
    Default value: $(if $(DETECTED_LICENSE),$(DETECTED_LICENSE),MIT)
  CXX_STANDARD                                                          Select the C++ standard.
    Default value: --std=c++14
  EMCC                                                                  Select the C++ standard.
    Default value: emcc
  EMCXX                                                                 Select the C++ standard.
    Default value: em++
  EMAR                                                                  Select the C++ standard.
    Default value: emar
  CC                                                                    Select the C++ standard.
    Default value:
  CXX                                                                   Select the C++ standard.
    Default value:
  AR                                                                    Select the C++ standard.
    Default value: ar
  ARFLAGS                                                               Select the C++ standard.
    Default value: crs
  RUN                                                                   Select the C++ standard.
    Default value:
  STRIP                                                                 Select the C++ standard.
    Default value: strip -R .comment -R .note -R .note.ABI-tag
  PKG_CONFIG_SEARCH_PATH                                                Select the C++ standard.
    Default value:

Targets:

  all:                        Compile the standalone application or static/shared libraries.
  asan:                       Launch the executable with address sanitizer (if enabled).
  benchmark:                  Run benchmark.
  build-stats:                Show build statistics.
  check:                      Call unit-tests with code coverage.
  check-compiled-with-gprof:  Launch the executable with gprof.
  check-deps:                 Check dependencies.
  check-harden:               Check if your project is harden
  clean::                     Clean the build folder.
  compilation-mode:           Display the compilation mode of the project.
  compile-external-libs:      Compile external projects needed.
  compiler-info:              Display the compiler version and information.
  coverage:                   Generate the code coverage html rapport.
  coverity-scan:              Create a tarball for Coverity Scan a static analysis of code.
  creating-build-folder:      Ensure .clang-format file exists at the project root.
  cxx-standard:               Display the C++ standard used.
  doc:                        Generate the code source documentation with doxygen.
  download-external-libs::    Download external github code source needed by this project.
  ensure-clang-format:        Ensure .clang-format file exists at the project root or install it.
  format-source-code:         Format source code with .clang-format file (else a default one is installed).
  gprof:                      Launch the executable with gprof.
  help:                       Show this help.
  install::                   Install the project artifacts on the operating system
  internal-libs-deps:         Compile the standalone application or static/shared libraries.
  lint:                       Run static analysis.
  list-targets:               List all available targets.
  list-variables:             List all key project variables.
  obs:                        Create an uploadable tarball for the OpenSuse Build Service.
  post-build::                Compile the standalone application or static/shared libraries.
  post-build::                Post-build actions: rules to extend after the build process.
  pre-build::                 Pre-build actions: rules to extend before the build process.
  project-name:               Display the name of the project.
  project-version:            Display the version of the project.
  rebuild:                    Rebuild the project.
  rpm:                        Build RPM package using auto-generated spec file.
  run:                        Run the binary with optional arguments. For example make run -- foo --help
  show-flags:                 Show compilation flags.
  show-paths:                 Show project paths.
  size-analysis:              Show size analysis.
  tarball:                    Compress project sources without in the goal to backup the code or share it.
  target-description:         Display the description of the target.
  target-name:                Display the name of the target.
  tests:                      Call unit-tests with code coverage.
  veryclean::                 Clean everything: build, third-parties, documentation, and generated files.
```

## 📝 License

MIT License - See [LICENSE](LICENSE) file for details.
