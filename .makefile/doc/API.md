# API Documentation

## Makefile Help System

To view available variables that can be overridden and rules that can be called, execute in your console:

```bash
make help
```

To extend MyMakefile rules and documentation:

- Document default variables using `#? my single-line comment` before the variable declaration
- Document rules using `#? my single-line comment` before the rule definition
- Constraint: Only single-line comments are supported

### Example (Makefile)

```makefile
#? This is my variable documentation
FOO ?= bar

#? This is my rule documentation
foobar:
    ...
```

### Example (make help output)

```bash
> make help

Usage:
  [VERBOSE=1] make [flags...] <target>

You can override the following flags:
  FOO               This is my variable documentation
    Default value: bar

Targets:
  foobar:           This is my rule documentation
```

## Debug and Verbose Options

The following variables can be combined for enhanced debugging:

- `VERBOSE` (or `V`): When set, displays Makefile rules content during execution
- `DEBUG` (or `D`): When set, shows shell commands during execution

Best practice is to add them, combined with other makefile debug options:

- `--warn-undefined-variables`
- `--just-print`
- `--print-data-base`

### Example (debugging)

```bash
> V=1 D=1 make all --warn-undefined-variables
```

## Compiler Configuration

You can override the default C/C++ compiler settings. It's recommended to set these when calling make:

- `CC`: Defines your C compiler
- `CXX`: Defines your C++ compiler
- `AR`: Defines your archiver (for static libraries)

**Note:** Additional variables can be overridden. Use `make help` to view all available options.

### Example (compiler override)

```bash
> make CXX=clang++ all
```

## Project Configuration

The following mandatory variables must be defined before including `project/Makefile` and at the top of your Makefile:

- `P`: Local path to project root. Typically `P := .` for the root Makefile but can be `P := ../..` for nested folder. The local path is converted to absolute path by MyMakefile.
- `M`: Local path to MyMakefile root (suggested: `M := $(P)/.makefile`).
- `PROJECT_NAME`: Project identifier (used when your project is installed on your system).
- `PROJECT_VERSION`: Version number in `major.minor.patch` format.
- `TARGET_NAME`: Executable or library target name (library or standalone application).
- `TARGET_DESCRIPTION`: Brief project description (used for documentation, pkg-config, RPMs).
- `AUTHOR`: Project author name (default: git user or system user).
- `LICENSE`: Default license for RPM packages (default: `MIT`). Shall match your license file!
- `COMPILATION_MODE`: Build mode selection.
  - `release`: No debug symbols, stripped binary.
  - `debug`: GDB symbols with stack trace on segfault.
  - `normal`: GDB symbols without stack trace.
  - `test`: Debug mode with code coverage (for unit testing).
  - `release-with-debug`: Optimized build (O2) with debug symbols.
  - `size`: Optimize for minimal binary size (Os with LTO and garbage collection).
  - `fast`: Maximum performance optimizations (Ofast with unsafe math).

Optional variables:

- `FORCE_LOWER_CASE`: When set, forces `PROJECT_NAME` and `TARGET_NAME` to lowercase. Useful when you used 'title case' convention but you want to install on operating system that need lower case (i.e. RedHat).

### Example (project configuration)

```makefile
P := .
M := $(P)/.makefile

PROJECT_NAME := MySuperProject
PROJECT_VERSION := 0.2.0
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := This is a real super project
COMPILATION_MODE := release
FORCE_LOWER_CASE := 1

include $(M)/project/Makefile
```

## Project Directory Structure

Optional configuration variables for project structure (define before including `project/Makefile`):

- `BUILD`: Build artifacts directory (default: `build`).
- `THIRD_PARTIES`: Third-party libraries directory (default: `external`).
- `PROJECT_DATA`: Project data directory (default: `data`).
- `DOC_FOLDER_NAME`: Documentation directory (default: `doc`).
- `PROJECT_GENERATED_DOC`: Generated documentation directory (default: `doc`).
- `PROJECT_TESTS`: Unit tests directory (default: `tests`).
- `PROJECT_TEMP_DIR`: Temporary files directory (default: `$(TMPDIR)/$(PROJECT_NAME)/$(PROJECT_VERSION)`).

### Third-parties Repository Cloning Syntax

The manifest file (default: `external/manifest`) supports the following syntax for cloning repositories:

```text
user/repo[:recurse][@ref]
```

Where:

- `user/repo`: GitHub repository path
- `:recurse`: Optional flag to clone submodules recursively
- `@ref`: Optional reference (branch, tag, or commit hash)

Examples:

```text
# Clone specific commit
SFML/SFML@a6579ecf5f58e9c0ae95edaac790526e024c59f6

# Clone specific branch
ocornut/imgui@docking

# Clone specific tag
SFML/imgui-sfml@2.6.x

# Clone with submodules
SFML/imgui-sfml:recurse@2.6.x

# Clone default branch
SFML/SFML
```

### Example (project structure configuration)

```makefile
P := .
M := $(P)/.makefile

PROJECT_NAME := MySuperProject
PROJECT_VERSION := 0.2.0
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := This is a real super project
COMPILATION_MODE := release

BUILD := out
THIRD_PARTIES_FOLDER_NAME := thirdparties
DATA_FOLDER_NAME := demo/data
DOC_FOLDER_NAME := docs
PROJECT_GENERATED_FOLDER_NAME := docs/gen
PROJECT_TESTS := check

include $(M)/project/Makefile
```

## C/C++ Compiler and Linker Configuration

Optional compiler and linker settings (define before including `project/Makefile`):

- `CXX_STANDARD`: C++ standard (default: `--std=c++14`)
- `REGEXP_CXX_HEADER_FILES`: C++ header file patterns (default: `*.hpp *.ipp *.tpp *.hh *.h *.hxx *.incl`)
- `USER_CXXFLAGS`: Override default C++ compilation flags
- `USER_CCFLAGS`: Override default C compilation flags
- `USER_LDFLAGS`: Override default linker flags

### Example (C++ standard and flags)

```makefile
P := .
M := $(P)/.makefile

PROJECT_NAME := MySuperProject
PROJECT_VERSION := 0.2.0
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := This is a real super project
COMPILATION_MODE := release

CXX_STANDARD := --std=c++11
USER_CXXFLAGS := -Wno-old-style-cast

include $(M)/project/Makefile
```

## Main Makefile

If your main Makefile does not compile files but just launching Makefiles for compiling other internal libraries add the following line:

```makefile
ORCHESTRATOR_MODE := 1
```

## Compiling static and shared libraries

The following optional variables can be defined before including `project/Makefile`:

- Use `LIB_FILES` instead of SRC_FILES. Do not add `lib` to TARGET_NAME because MyMakefile will manage it.
- `PREVENT_SHARED_LIB_UNLOAD`: Use nodelete flag for shared libraries to prevent unloading (default: unset).
- `CUSTOM_RPATH`: Custom runtime library search paths, colon-separated (default: unset).
- `DO_NOT_COMPILE_STATIC_LIB`: Skip static library compilation (default: unset).
- `DO_NOT_COMPILE_SHARED_LIB`: Skip shared library compilation (default: unset).

## Documentation Configuration

- `DOXYGEN_INPUTS`: Doxygen input files (default: `$(P)/README.md $(P)/src $(P)/include`)
- `GENERATED_DOXYGEN_DIR`: Doxygen output directory (default: `$(PROJECT_GENERATED_DOC_DIR)/doxygen`)
- `GPROF_ANALYSIS`: Profiling analysis output (default: `$(PROJECT_GENERATED_DOC_DIR)/profiling/analysis.txt`)
- `COVERAGE_HTML_RAPPORT`: Code coverage HTML report location (default: `$(COVERAGE_DIR)/coverage.html`)
- `COVERAGE_LCOV_RAPPORT`: Code coverage LCOV report location (default: `$(COVERAGE_DIR)/lcov.info`)
- `MACOS_BUNDLE_ICON`: macOS application bundle icon (default: `$(M)/assets/$(DATA_FOLDER_NAME)/macos.icns`)
- `PATH_PROJECT_LOGO`: Project logo path (default: `$(M)/assets/icons/logo.png`)

## System Information

The following variables are automatically determined by MyMakefile and available after including `project/Makefile`:

- `OS`: Operating system identifier
  - `Linux`: Linux-based systems (Ubuntu, Debian, etc.)
  - `Windows`: Windows systems (with `WIN_ENV` for environment specifics)
  - `Darwin`: macOS systems
  - `Emscripten`: Emscripten environment
  - `ExaequOS`: ExaequOS environment (check with `ifdef EXAEQUOS`)
- `ARCHI`: Architecture identifier (e.g., 32, 64, x86_64)

System-specific extensions:

- `EXT_BINARY`: Executable extension (`.exe` for Windows, `.app` for macOS)
- `EXT_DYNAMIC_LIB`: Shared library extension (`.so` for Linux, `.dll` for Windows, `.dylib` for macOS)
- `EXT_STATIC_LIB`: Static library extension (`.a` for Linux, `.lib` for Windows)

## Makefile Rules

### Build Targets

- `all`: Compile the standalone application or static/shared libraries (default target)
- `pre-build`: Pre-build hook for custom actions before the build process
- `post-build`: Post-build hook for custom actions after the build process
- `rebuild`: Clean and rebuild (alias for `veryclean all`)
- `run`: Execute the compiled application with optional arguments (e.g., `make run -- --arg1 --arg2`)

### Code Quality Targets

- `format-source-code`: Format source code using clang-format
- `lint`: Run static analysis with cppcheck
- `check-harden`: Check if binary has security hardening enabled

### Diagnostic Targets

- `compiler-info`: Display the compiler version and information
- `target`: Display the description of the project
- `project-version`: Display the version of the project
- `project-name`: Display the name of the project
- `target-name`: Display the name of the target
- `target-description`: Display the description of the target
- `compilation-mode`: Display the compilation mode of the project
- `cxx-standard`: Display the C++ standard used
- `list-variables`: List all key project variables
- `list-targets`: List all available targets
- `show-flags`: Display compilation flags
- `show-paths`: Display project paths
- `check-deps`: Check if required dependencies are installed
- `build-stats`: Show build statistics (file counts, etc.)
- `size-analysis`: Show binary/library size analysis

### Profiling Targets

```bash
make USE_GPROF=1 gprof
make USE_GPROF=1 check-compiled-with-gprof
```

Profiling analysis is generated to `$(GPROF_ANALYSIS)` location.

### Testing Targets

- `check` or `tests`: Run unit tests with code coverage
- `coverage`: Generate code coverage report (HTML and LCOV formats)
- `asan`: Run with address sanitizer (requires `USE_ASAN=1`)
- `benchmark`: Run performance benchmark

### Packaging Targets

- `tarball`: Compress project sources for backup or sharing
- `rpm`: Build RPM package using auto-generated spec file
- `coverity-scan`: Create a tarball for Coverity Scan static analysis
- `obs`: Create an uploadable tarball for the OpenSuse Build Service

### Clean Targets

- `clean`: Clean the build folder
- `veryclean`: Deep clean (build, third-parties, documentation, and generated files)

### Install Targets

- `install`: Install the project artifacts on the operating system

## GNU Standard Installation Paths

Following the [GNU standards](https://www.gnu.org/prep/standards/html_node/Directory-Variables.html):

- `DESTDIR`: Installation root (default: empty)
- `PREFIX`: Installation prefix (default: `/usr/local`)
- `INCLUDEDIR`: Header files location (default: `$(PREFIX)/include`)
- `LIBDIR`: Library files location (default: `$(PREFIX)/lib`)
- `PKGLIBDIR`: pkg-config files location (default: `$(LIBDIR)/pkgconfig`)
- `DATADIR`: Data and documentation location (default: `$(PREFIX)/share`)
- `BINDIR`: Executable files location (default: `$(PREFIX)/bin`)
- `TMPDIR`: Temporary files location (default: `/tmp`)

**Note:** For ExaequOS, binaries are installed to `/media/localhost/$(TARGET_NAME)/exa` regardless of `DESTDIR` and `BINDIR` settings.

## Print Helper Functions

- `print-simple($1,$2)`: Print colored text
- `print-from($1,$2,$3)`: Print colored text for incoming information
- `print-to($1,$2,$3)`: Print colored text for outgoing information

## Common Commands

- `make help`: Show all available targets and variables
- `make doc`: Generate documentation with Doxygen- `make run`: Execute the compiled application (for non-library projects)
- `make run -- your list -of --arguments`: Run with command-line arguments
- `make doc`: Generate documentation with Doxygen
- **Note:** For Emscripten, the `run` command will launch your web browser
