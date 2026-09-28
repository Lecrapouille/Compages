##=============================================================================
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
##=============================================================================

###############################################################################
# Performance and parallelization improvements
###############################################################################

# Auto-detect number of CPU cores for parallel compilation
ifeq ($(OS),Linux)
    NPROCS := $(shell nproc)
else ifeq ($(OS),Darwin)
    NPROCS := $(shell sysctl -n hw.ncpu)
else ifeq ($(OS),Windows)
    NPROCS := $(or $(NUMBER_OF_PROCESSORS),$(shell nproc 2>/dev/null),8)
else
    NPROCS := 1
endif

# Set default parallel jobs if not specified and not in recursive make
# This avoids the "forced in makefile: resetting jobserver mode" warning
ifeq ($(MAKELEVEL),0)
    ifeq ($(findstring -j,$(MAKEFLAGS)),)
        MAKEFLAGS += -j$(NPROCS)
    endif
endif

# Optimize make for better performance
.NOTPARALLEL: clean veryclean install
.DELETE_ON_ERROR:
.SUFFIXES:

# Force the default goal to be all: else with DIRS_WITH_MAKEFILE we have to
# set an order of compilation and the rules will be placed after the all: goal.
.DEFAULT_GOAL := all

###############################################################################
# C/C++ compilers
###############################################################################

#? Select the C++ standard.
CXX_STANDARD ?= --std=c++14

ifeq ($(OS),Emscripten)
    #? Define your C compiler.
    EMCC ?= emcc
    CC := $(EMCC)

    #? Define your C++ compiler.
    EMCXX ?= em++
    CXX := $(EMCXX)

    #? Define your static library tool and its default flags.
    EMAR ?= emar
    AR := $(EMAR)

    # Force default flags for ar command.
    ifdef EXAEQUOS
        ARFLAGS := crs
    endif

    #? command for running the application
    RUN := emrun

    # Do not strip symbols
    STRIP :=
else
    #? Define your C compiler.
    CC ?=

    #? Define your C++ compiler.
    CXX ?=

    #? Define your static library tool and its default flags.
    AR ?= ar

    #? Define default flags for ar command.
    ARFLAGS ?= crs

    #? command for running the application
    RUN ?=

    #? Strip command. See http://reverse.lostrealm.com/protect/strip.html
    ifeq ($(OS),Linux)
        ifeq ($(COMPILATION_MODE),release)
            STRIP ?= strip -R .comment -R .note -R .note.ABI-tag
        else
            STRIP :=
        endif
    endif
endif

###############################################################################
# Search order for finding project:
#  - data folder inside the project (when compiled in debug mode).
#  - data folder inside the user operating system (read-write access).
#  - data folder installed on the operating system (read-only access).
#  - folder data on the same folder than the executable (maybe read-only access).
#  - folder local to the executable (maybe read-only access).
###############################################################################

SEARCH_DATA_PATHS := $(INSTALL_LOCAL_PROJECT_DATA_DIR):$(INSTALL_PROJECT_DIR)/$(DATA_FOLDER_NAME):.
ifeq ($(COMPILATION_MODE),debug)
    SEARCH_DATA_PATHS := $(PROJECT_DATA_DIR):$(SEARCH_DATA_PATHS)
endif

###############################################################################
# Compiler detection and version management
###############################################################################

ifdef ORCHESTRATOR_MODE
    COMPILER := $(CXX) $(CXX_STANDARD)
else ifneq ($(filter %.cpp,$(strip $(SRC_FILES) $(LIB_FILES))),)
    COMPILER := $(CXX) $(CXX_STANDARD)
else
    COMPILER := $(CC)
endif

COMPILER_NAME := $(shell $(COMPILER) --version | head -1 | cut -d" " -f1 | cut -d"-" -f1)
COMPILER_VERSION := $(shell $(COMPILER) --version | head -1 | grep -o -E '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
DEFINES += -DCOMPILER_NAME=\"$(COMPILER_NAME)\"
DEFINES += -DCOMPILER_VERSION=\"$(COMPILER_VERSION)\"

# Version comparison helpers for GCC
GCC_GREATER_V4_8 := $(shell expr $(COMPILER_VERSION) \>= 40800)
GCC_GREATER_V4_9 := $(shell expr $(COMPILER_VERSION) \>= 40900)
GCC_GREATER_V6 := $(shell expr $(COMPILER_VERSION) \>= 60000)
GCC_GREATER_V7 := $(shell expr $(COMPILER_VERSION) \>= 70000)
GCC_GREATER_V8 := $(shell expr $(COMPILER_VERSION) \>= 80000)
GCC_GREATER_V9 := $(shell expr $(COMPILER_VERSION) \>= 90000)
GCC_GREATER_V10 := $(shell expr $(COMPILER_VERSION) \>= 100000)
GCC_GREATER_V11 := $(shell expr $(COMPILER_VERSION) \>= 110000)
GCC_GREATER_V12 := $(shell expr $(COMPILER_VERSION) \>= 120000)

# Version comparison helpers for Clang
CLANG_VERSION_NUM := $(shell echo $(COMPILER_VERSION) | sed 's/\([0-9]*\)\.\([0-9]*\).*/\1\2/')
CLANG_GREATER_V10 := $(shell expr $(CLANG_VERSION_NUM) \>= 100)
CLANG_GREATER_V12 := $(shell expr $(CLANG_VERSION_NUM) \>= 120)
CLANG_GREATER_V14 := $(shell expr $(CLANG_VERSION_NUM) \>= 140)
CLANG_GREATER_V15 := $(shell expr $(CLANG_VERSION_NUM) \>= 150)

###############################################################################
# Base compilation flags (common to C and C++)
###############################################################################

# Base flags common to all compilers
BASE_FLAGS := -pipe

# Performance flags (-ffast-math breaks INFINITY/NaN; enable via USE_FAST_MATH)
PERFORMANCE_FLAGS := -funroll-loops

# Modern warning flags common to C and C++
COMMON_WARNING_FLAGS := -Wall -Wextra -Wuninitialized -Wundef -Wunused \
    -Wunused-result -Wunused-parameter -Wtype-limits \
    -Wcast-align -Wcast-qual -Wconversion -Wfloat-equal \
    -Wpointer-arith -Wswitch-enum -Wpedantic -Wpacked \
    -Wdeprecated -Wvariadic-macros -Wvla -Wsign-conversion -Wshadow \
    -Wnull-dereference -Wdouble-promotion -Wformat=2

# Add GCC-specific warnings only for GCC
ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
    COMMON_WARNING_FLAGS += -Wduplicated-cond -Wduplicated-branches -Wlogical-op
endif

# Advanced security and hardening flags
HARDENING_FLAGS := -Wformat -Wformat-security -Werror=format-security \
    -fPIE -fexceptions

# _FORTIFY_SOURCE requires optimization (-O1 or higher)
ifneq ($(COMPILATION_MODE),debug)
    HARDENING_FLAGS += -D_FORTIFY_SOURCE=2
endif

# Add platform-specific hardening flags
ifneq ($(OS),Darwin)
    HARDENING_FLAGS += -fstack-clash-protection
    # Control Flow Integrity protection (not available on macOS)
    ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
        ifeq ($(GCC_GREATER_V8),1)
            HARDENING_FLAGS += -fcf-protection=full
        endif
    else ifeq ($(COMPILER_NAME),clang)
        # Only add CF protection on x86_64 Linux
        ifeq ($(OS),Linux)
            ifeq ($(ARCH),x86_64)
                HARDENING_FLAGS += -fcf-protection=full
            endif
        endif
    endif
endif

# Modern Clang-specific warning flags
CLANG_WARNING_FLAGS := -Wconditional-uninitialized \
    -Wused-but-marked-unused -Wunused-member-function \
    -Wunused-getter-return-value -Wunused-exception-parameter \
    -Wunneeded-member-function -Wunneeded-internal-declaration -Wshadow-all \
    -Wunreachable-code-aggressive -Wvector-conversion -Wzero-length-array \
    -Wthread-safety -Werror=implicit-function-declaration \
    -Wcomma -Wdocumentation -Wloop-analysis -Wshift-sign-overflow \
    -Wstring-conversion -Wtautological-overlap-compare

# Advanced GCC-specific warning flags (version dependent)
define add_gcc_version_flags
    ifeq ($(GCC_GREATER_V4_9),1)
        $(1) += -fstack-protector-strong
    endif
    ifeq ($(GCC_GREATER_V6),1)
        $(1) += -Wtautological-compare -Wshift-overflow=2 -Wshift-negative-value
    endif
    ifeq ($(GCC_GREATER_V7),1)
        $(1) += -Wduplicated-branches -Walloc-zero -Walloca
    endif
    ifeq ($(GCC_GREATER_V8),1)
        $(1) += -Wcast-align=strict -Wcatch-value=3 -Wextra-semi
    endif
    ifeq ($(GCC_GREATER_V9),1)
        $(1) += -Wpessimizing-move -Wredundant-move
    endif
    ifeq ($(GCC_GREATER_V10),1)
        $(1) += -Warith-conversion -Wcomma-subscript
    endif
    ifeq ($(GCC_GREATER_V11),1)
        $(1) += -Wrange-loop-construct -Wmismatched-tags
    endif
    ifeq ($(GCC_GREATER_V12),1)
        $(1) += -Wbidi-chars=any -Wopenacc-parallelism
    endif
endef

# Advanced Clang version-specific flags
define add_clang_version_flags
    ifeq ($(CLANG_GREATER_V10),1)
        $(1) += -Wrange-loop-analysis -Wfor-loop-analysis
    endif
    ifeq ($(CLANG_GREATER_V12),1)
        $(1) += -Wbitwise-instead-of-logical -Wsuspicious-bzero
    endif
    ifeq ($(CLANG_GREATER_V14),1)
        $(1) += -Warray-parameter -Wbitwise-op-parentheses
    endif
    ifeq ($(CLANG_GREATER_V15),1)
        $(1) += -Wunsafe-buffer-usage -Wcast-function-type-strict
    endif
endef

# Optional performance flags (can be enabled by user)
ifdef USE_FAST_MATH
    PERFORMANCE_FLAGS += -ffast-math
endif

# Optional native architecture flags (can be enabled by user)
ifdef USE_NATIVE_ARCH
    PERFORMANCE_FLAGS += -march=native -mtune=native
endif

# Optional vectorization flags (can be enabled by user)
ifdef USE_VECTORIZATION
    PERFORMANCE_FLAGS += -ftree-vectorize -fopt-info-vec
    ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
        ifeq ($(GCC_GREATER_V9),1)
            PERFORMANCE_FLAGS += -fopt-info-vec-optimized
        endif
    endif
endif

# Optional strict flags for extra pedantic compilation
STRICT_FLAGS :=
ifdef USE_STRICT_FLAGS
    STRICT_FLAGS := -Werror -Wconversion -Wsign-conversion -Wcast-qual \
        -Wwrite-strings -Wredundant-decls -Winline -Wdisabled-optimization

    ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
        STRICT_FLAGS += -Weffc++ -Wstrict-null-sentinel
    endif

    ifeq ($(COMPILER_NAME),clang)
        STRICT_FLAGS += -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic \
            -Wno-padded -Wno-weak-vtables -Wno-exit-time-destructors \
            -Wno-global-constructors
    endif
endif

###############################################################################
# C++ specific compilation flags
###############################################################################

# Check for user override
ifdef CXXFLAGS
    $(warning You have overridden default compilation flags CXXFLAGS with your own flags $(CXXFLAGS). Better to set USER_CXXFLAGS)
endif

# Initialize C++ flags
CXXFLAGS := $(BASE_FLAGS) $(COMMON_WARNING_FLAGS) $(HARDENING_FLAGS) $(PERFORMANCE_FLAGS) $(STRICT_FLAGS)

# C++ specific warnings
CXXFLAGS += -Wold-style-cast

# Modern C++ specific warnings
CXX_SPECIFIC_FLAGS := -Woverloaded-virtual -Wnon-virtual-dtor -Wctor-dtor-privacy \
    -Wdelete-non-virtual-dtor

# Add GCC-specific C++ warnings
ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
    CXX_SPECIFIC_FLAGS += -Wuseless-cast
endif

# C++11/14/17/20 specific warnings based on standard
ifeq ($(findstring c++11,$(CXX_STANDARD)),c++11)
    CXX_SPECIFIC_FLAGS += -Wc++11-compat
else ifeq ($(findstring c++14,$(CXX_STANDARD)),c++14)
    CXX_SPECIFIC_FLAGS += -Wc++14-compat
else ifeq ($(findstring c++17,$(CXX_STANDARD)),c++17)
    CXX_SPECIFIC_FLAGS += -Wc++17-compat
else ifeq ($(findstring c++20,$(CXX_STANDARD)),c++20)
    CXX_SPECIFIC_FLAGS += -Wc++20-compat
endif

CXXFLAGS += $(CXX_SPECIFIC_FLAGS)

# Compiler-specific flags for C++
ifeq ($(COMPILER_NAME),clang)
    CXXFLAGS += $(CLANG_WARNING_FLAGS) \
        -Wtautological-compare -Wno-unused-command-line-argument \
        -Wundefined-reinterpret-cast -Wsuper-class-method-mismatch \
        -Winconsistent-missing-override -Wabstract-vbase-init
    $(eval $(call add_clang_version_flags,CXXFLAGS))
else ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
    CXXFLAGS += -Wnoexcept -Wstrict-null-sentinel -Wlogical-op
    $(eval $(call add_gcc_version_flags,CXXFLAGS))
endif

###############################################################################
# C specific compilation flags
###############################################################################

# Check for user override
ifdef CCFLAGS
    $(warning You have overridden default compilation flags CCFLAGS with your own flags $(CCFLAGS). Better to set USER_CCFLAGS)
endif

# Initialize C flags
CCFLAGS := $(BASE_FLAGS) $(COMMON_WARNING_FLAGS) $(HARDENING_FLAGS) $(PERFORMANCE_FLAGS) $(STRICT_FLAGS)

# C specific warnings
CCFLAGS += -Wstrict-prototypes

# Additional C-specific warnings
C_SPECIFIC_FLAGS := -Wmissing-prototypes -Wmissing-declarations \
    -Wbad-function-cast -Wnested-externs -Wold-style-definition

CCFLAGS += $(C_SPECIFIC_FLAGS)

# Compiler-specific flags for C
ifeq ($(COMPILER_NAME),clang)
    CCFLAGS += $(CLANG_WARNING_FLAGS)
    $(eval $(call add_clang_version_flags,CCFLAGS))
else ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
    CCFLAGS += -Wjump-misses-init -Wlogical-op
    $(eval $(call add_gcc_version_flags,CCFLAGS))
endif

###############################################################################
# Shared library compilation flags
###############################################################################

# If we do not compile shared library, we do not need to set rpath and nodelete flags
ifdef DO_NOT_COMPILE_SHARED_LIB
    CUSTOM_RPATH :=
    PREVENT_SHARED_LIB_UNLOAD :=
endif

# Position Independent Code flags for shared libraries
ifeq ($(OS),Darwin)
    DLL_CXXFLAGS := -fPIC
    DLL_LDFLAGS := -dynamiclib
    DEFINES += -DAPI_LINKAGE=
else ifeq ($(OS),Linux)
    DLL_CXXFLAGS := -fPIC
    DLL_LDFLAGS := -shared
    DEFINES += -DAPI_LINKAGE=
else ifeq ($(OS),Windows)
    DLL_CXXFLAGS := -fPIC
    DLL_LDFLAGS := -shared
    ifdef DLL_LINKAGE_EXPORT
        DEFINES += -DAPI_LINKAGE="__declspec(dllexport)"
    else
        DEFINES += -DAPI_LINKAGE="__declspec(dllimport)"
    endif
    ifeq ($(WIN_ENV),MINGW)
        DLL_LDFLAGS += -lmingw32
    else
        $(error Unknown Windows environment for shared library compilation)
    endif
else ifeq ($(OS),Emscripten)
    DLL_CXXFLAGS :=
    DLL_LDFLAGS :=
    DEFINES += -DAPI_LINKAGE=
else
    $(error Unsupported OS for shared library compilation: $(OS))
endif

# Optional nodelete flag for shared libraries only
ifdef PREVENT_SHARED_LIB_UNLOAD
    DLL_LDFLAGS += -Wl,-z,nodelete
endif

# Custom runtime library search paths (rpath)
ifdef CUSTOM_RPATH
    ifeq ($(OS),Windows)
        $(warning CUSTOM_RPATH is not supported on Windows)
    else ifeq ($(OS),Emscripten)
        $(warning CUSTOM_RPATH is not supported on Emscripten)
    else
        # Convert colon-separated paths to individual -Wl,-rpath flags
        RPATH_FLAGS := $(foreach path,$(subst :, ,$(CUSTOM_RPATH)),-Wl,-rpath,$(path))
        LDFLAGS += $(RPATH_FLAGS)
        # For shared libraries, also add to DLL_LDFLAGS
        ifndef DO_NOT_COMPILE_SHARED_LIB
            DLL_LDFLAGS += $(RPATH_FLAGS)
        endif
    endif
endif

# Apply shared library flags
CXXFLAGS += $(DLL_CXXFLAGS)
CCFLAGS += $(DLL_CXXFLAGS)

# Linker flags for position independent executables (not all platforms support -pie)
ifeq ($(OS),Linux)
    LDFLAGS += -pie
endif

# Creating shared libraries from static ones.
# Note: since 2024 -noall_load are removed on MacOS.
ifneq ($(OS),Darwin)
    LINKER_ALL_LOAD := -Wl,--whole-archive
    LINKER_NO_ALL_LOAD := -Wl,--no-whole-archive
endif

###############################################################################
# Optimization and debug flags based on compilation mode
###############################################################################

# Helper function to set optimization flags
define set_optimization_flags
    ifeq ($(COMPILATION_MODE),debug)
        OPTIM_FLAGS := -O0 -g3 -fasynchronous-unwind-tables
        DEFINES += -UNDEBUG -D_GLIBCXX_ASSERTIONS
        # Debug-specific optimizations
        ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
            ifeq ($(GCC_GREATER_V7),1)
                OPTIM_FLAGS += -fvar-tracking-assignments
            endif
        endif
        ifeq ($(OS),Linux)
            # Only enable backward-cpp for standalone applications, not for libraries
            # Libraries should not include backward-cpp to avoid symbol conflicts
            ifndef LIB_FILES
                USE_BACKWARD := 1
                USE_DEBUG_MACRO := 1
            else ifdef SRC_FILES
                USE_BACKWARD := 1
                USE_DEBUG_MACRO := 1
            endif
        endif
    else ifeq ($(COMPILATION_MODE),release)
        OPTIM_FLAGS := -O3 -DNDEBUG -fomit-frame-pointer
        # Advanced release optimizations
        ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
            OPTIM_FLAGS += -flto -fuse-linker-plugin -ffat-lto-objects
            ifeq ($(GCC_GREATER_V8),1)
                OPTIM_FLAGS += -fdevirtualize-at-ltrans
            endif
            ifeq ($(GCC_GREATER_V10),1)
                OPTIM_FLAGS += -fipa-pta -floop-nest-optimize
            endif
        else ifeq ($(COMPILER_NAME),clang)
            OPTIM_FLAGS += -flto=thin
            ifeq ($(CLANG_GREATER_V12),1)
                OPTIM_FLAGS += -fwhole-program-vtables
            endif
        endif
        # Additional release flags
        OPTIM_FLAGS += -ffunction-sections -fdata-sections
        # Use garbage collection only on platforms that support it (GNU ld)
        ifeq ($(OS),Linux)
            LDFLAGS += -Wl,--gc-sections
        endif
    else ifeq ($(COMPILATION_MODE),release-with-debug)
        OPTIM_FLAGS := -O2 -g -DNDEBUG
        # Optimized but with debug info
        ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
            OPTIM_FLAGS += -flto=auto
        endif
    else ifeq ($(COMPILATION_MODE),normal)
        OPTIM_FLAGS := -O2 -g
        DEFINES += -UNDEBUG -D_GLIBCXX_ASSERTIONS
    else ifeq ($(COMPILATION_MODE),size)
        OPTIM_FLAGS := -Os -DNDEBUG -fomit-frame-pointer
        # Size optimization
        OPTIM_FLAGS += -ffunction-sections -fdata-sections -fmerge-all-constants
        # Use garbage collection only on platforms that support it (GNU ld)
        ifeq ($(OS),Linux)
            LDFLAGS += -Wl,--gc-sections
        endif
        ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
            OPTIM_FLAGS += -flto
        endif
    else ifeq ($(COMPILATION_MODE),fast)
        OPTIM_FLAGS := -Ofast -DNDEBUG -fomit-frame-pointer
        # Maximum performance (may break standards compliance)
        OPTIM_FLAGS += -ffast-math -funsafe-math-optimizations
        ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
            OPTIM_FLAGS += -flto -march=native -mtune=native
        endif
    else
        $$(error COMPILATION_MODE must be debug, release, release-with-debug, normal, size, or fast, but got $(COMPILATION_MODE))
    endif
endef

$(eval $(call set_optimization_flags))

# Additional performance optimizations (optional)
ifdef USE_PROFILE_GUIDED_OPTIMIZATION
    ifeq ($(COMPILATION_MODE),release)
        ifdef PGO_GENERATE
            OPTIM_FLAGS += -fprofile-generate
            LDFLAGS += -fprofile-generate
        endif
        ifdef PGO_USE
            OPTIM_FLAGS += -fprofile-use -fprofile-correction
            LDFLAGS += -fprofile-use
        endif
    endif
endif

# Link-time optimization settings
ifdef USE_LTO_JOBS
    ifeq ($(COMPILER_NAME),$(filter $(COMPILER_NAME),g++ gcc))
        OPTIM_FLAGS += -flto=$(USE_LTO_JOBS)
    endif
endif

###############################################################################
# Sanitizers and profiling tools
###############################################################################

# Helper function to add sanitizer flags
define add_sanitizer_flags
    ifdef USE_ASAN
        USE_BACKWARD :=
        CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
        CCFLAGS += -fsanitize=address -fno-omit-frame-pointer
        LDFLAGS += -fsanitize=address
        SANITIZER := ASAN_OPTIONS=symbolize=1:detect_leaks=1:check_initialization_order=1:strict_init_order=1 ASAN_SYMBOLIZER_PATH=$$(shell which llvm-symbolizer)
        ifdef STRIP
            $$(warning Address sanitizer requires symbols, disabling strip)
            STRIP :=
        endif
    endif

    ifdef USE_TSAN
        USE_BACKWARD :=
        CXXFLAGS += -fsanitize=thread
        CCFLAGS += -fsanitize=thread
        LDFLAGS += -fsanitize=thread
        SANITIZER := TSAN_OPTIONS=symbolize=1:second_deadlock_stack=1
        ifdef STRIP
            $$(warning Thread sanitizer requires symbols, disabling strip)
            STRIP :=
        endif
    endif

    ifdef USE_UBSAN
        CXXFLAGS += -fsanitize=undefined -fno-sanitize-recover=undefined
        CCFLAGS += -fsanitize=undefined -fno-sanitize-recover=undefined
        LDFLAGS += -fsanitize=undefined
        SANITIZER := UBSAN_OPTIONS=symbolize=1:print_stacktrace=1:halt_on_error=1
    endif

    ifdef USE_MSAN
        USE_BACKWARD :=
        CXXFLAGS += -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins=2
        CCFLAGS += -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins=2
        LDFLAGS += -fsanitize=memory
        SANITIZER := MSAN_OPTIONS=symbolize=1:print_stats=1
        ifdef STRIP
            $$(warning Memory sanitizer requires symbols, disabling strip)
            STRIP :=
        endif
    endif

    # Leak Sanitizer (can be used standalone or with ASAN)
    ifdef USE_LSAN
        CXXFLAGS += -fsanitize=leak
        CCFLAGS += -fsanitize=leak
        LDFLAGS += -fsanitize=leak
        SANITIZER := LSAN_OPTIONS=symbolize=1:print_suppressions=0
    endif

    # Control Flow Integrity (Clang only)
    ifdef USE_CFI
        ifeq ($(COMPILER_NAME),clang)
            CXXFLAGS += -fsanitize=cfi -flto -fvisibility=hidden
            CCFLAGS += -fsanitize=cfi -flto -fvisibility=hidden
            LDFLAGS += -fsanitize=cfi -flto
        else
            $$(warning CFI sanitizer is only available with Clang)
        endif
    endif

    # Safe Stack (Clang only)
    ifdef USE_SAFE_STACK
        ifeq ($(COMPILER_NAME),clang)
            CXXFLAGS += -fsanitize=safe-stack
            CCFLAGS += -fsanitize=safe-stack
            LDFLAGS += -fsanitize=safe-stack
        else
            $$(warning Safe Stack is only available with Clang)
        endif
    endif

    # Hardware-assisted AddressSanitizer (ARM64 only)
    ifdef USE_HWASAN
        ifeq ($(ARCH),aarch64)
            ifeq ($(COMPILER_NAME),clang)
                CXXFLAGS += -fsanitize=hwaddress -fno-omit-frame-pointer
                CCFLAGS += -fsanitize=hwaddress -fno-omit-frame-pointer
                LDFLAGS += -fsanitize=hwaddress
            else
                $$(warning HWAddressSanitizer requires Clang on ARM64)
            endif
        else
            $$(warning HWAddressSanitizer is only available on ARM64)
        endif
    endif
endef

$(eval $(call add_sanitizer_flags))

# Static analysis tools integration
ifdef USE_STATIC_ANALYSIS
    ifeq ($(COMPILER_NAME),clang)
        CXXFLAGS += --analyze -Xanalyzer -analyzer-output=text
        CCFLAGS += --analyze -Xanalyzer -analyzer-output=text
    endif
endif

# Valgrind-friendly compilation
ifdef USE_VALGRIND_FRIENDLY
    CXXFLAGS += -DVALGRIND_FRIENDLY -fno-inline-functions-called-once
    CCFLAGS += -DVALGRIND_FRIENDLY -fno-inline-functions-called-once
    OPTIM_FLAGS := $(filter-out -fomit-frame-pointer,$(OPTIM_FLAGS))
endif

# Profiling with gprof
ifdef USE_GPROF
    USE_BACKWARD :=
    CXXFLAGS += -pg
    CCFLAGS += -pg
    LDFLAGS += -pg
    ifdef STRIP
        $(warning Release mode with stripped symbols was desired but gprof needs them, strip command disabled!)
        STRIP :=
    endif
endif

# Code coverage
ifdef USE_COVERAGE
    OPTIM_FLAGS := -O1
    ifeq ($(COMPILATION_MODE),debug)
        OPTIM_FLAGS += -g
    endif
    CXXFLAGS += --coverage
    CCFLAGS += --coverage
    LDFLAGS += --coverage
endif

###############################################################################
# Debug libraries (backward-cpp and dbg-macro)
###############################################################################

# Activate backward-cpp library if desired
ifdef USE_BACKWARD
    # Check compatibility
    ifeq ($(COMPILATION_MODE),release)
        $(warning Release mode was desired but backward-cpp will force debug mode)
        USE_BACKWARD :=
    endif
    ifeq ($(OS),Windows)
        $(warning Cannot use backward-cpp lib for Windows. USE_BACKWARD will be disabled)
        USE_BACKWARD :=
    else ifeq ($(OS),Darwin)
        $(warning Cannot use backward-cpp lib for MacOS X. USE_BACKWARD will be disabled)
        USE_BACKWARD :=
    else ifeq ($(OS),Emscripten)
        $(warning Cannot use backward-cpp lib for Emscripten. USE_BACKWARD will be disabled)
        USE_BACKWARD :=
    else ifdef USE_GPROF
        $(warning Cannot use backward-cpp with gprof. USE_BACKWARD will be disabled)
        USE_BACKWARD :=
    endif
endif

PRIVATE_SRC_FILES :=
PRIVATE_LIB_FILES :=
ifdef USE_BACKWARD
    DEPS_DEBUG_LIBS += $(DOWNLOAD_DIR)/backward-cpp/backward.cpp
    PRIVATE_SRC_FILES += $(DOWNLOAD_DIR)/backward-cpp/backward.cpp
    PRIVATE_LIB_FILES += $(DOWNLOAD_DIR)/backward-cpp/backward.cpp
    INCLUDES += -I$(DOWNLOAD_DIR)/backward-cpp
    VPATH += $(DOWNLOAD_DIR)/backward-cpp
    DEFINES += -DBACKWARD_HAS_DW=1 -DBACKWARD_HAS_BFD=0 -DBACKWARD_HAS_DWARF=0
    DEFINES += -DBACKWARD_HAS_UNWIND=1 -DBACKWARD_HAS_BACKTRACE=0
    DEFINES += -DBACKWARD_HAS_BACKTRACE_SYMBOL=1 -DBACKWARD_HAS_LIBUNWIND=0
    LINKER_FLAGS += -ldw
endif

# Activate dbg-macro library if desired
ifdef USE_DEBUG_MACRO
    ifeq ($(COMPILATION_MODE),release)
        $(warning Release mode desired but USE_DEBUG_MACRO will force debug mode)
        USE_DEBUG_MACRO :=
    endif
endif

ifdef USE_DEBUG_MACRO
    DEPS_DEBUG_LIBS += $(DOWNLOAD_DIR)/dbg-macro/dbg.h
    INCLUDES += -I$(DOWNLOAD_DIR)/dbg-macro
    DEFINES += -DDBG_MACRO_NO_WARNING
endif

###############################################################################
# Manage the C/C++ project.
###############################################################################

# static libs are just zipped object files. When you want to merge static libs
# you have to extract first their object files then combined them into a new
# static lib.
# BEWARE: do not include $(INTERNAL_LIBS) since we do not want internal libs
# include each others: let them independent and let the generated pkg_config
# include them. Let only include static third party libs inside internal libs
# since we do not want to force installing them on the user system.
define extract-objs-from-static-libs
    @$(BASH) $(SCRIPT_EXTRACT_STATIC_LIB) $(OBJS_ROOT_DIR) $(THIRD_PARTIES_LIBS)
    $(eval EXTRACTED_OBJS=$(addsuffix /*.o,$(addprefix $(OBJS_ROOT_DIR)/,$(basename $(notdir $(filter-out \,$(THIRD_PARTIES_LIBS) $(INTERNAL_LIBS)))))))
endef

# Else, the user have supposed given some C/C++ file to compile. Check if at least one is present.
# Exception: if ORCHESTRATOR_MODE is defined, this is an orchestrator Makefile that only manages sub-projects
ifdef ORCHESTRATOR_MODE
  ifneq ($(strip $(LIB_FILES) $(SRC_FILES)),)
    $(error ORCHESTRATOR_MODE is defined but LIB_FILES or SRC_FILES are also present. Cannot compile files in orchestrator mode)
  endif
else
  ifeq ($(strip $(LIB_FILES) $(SRC_FILES)),)
    ifndef DIRS_WITH_MAKEFILE
      $(error Found no C/C++ object files to compile. Please set correctly LIB_DIRS or/and SRC_DIRS, or define ORCHESTRATOR_MODE for orchestrator mode)
    endif
  endif
endif

# Convert C/C++ application files to object files
_SRC_DIRS :=
OBJS :=
ifdef SRC_FILES
    SRC_FILES := $(foreach d,$(SRC_FILES) $(PRIVATE_SRC_FILES),$(abspath $d))
    SRC_FILES := $(patsubst $(abspath $(P))/%,%,$(SRC_FILES))
    _SRC_DIRS := $(dir $(SRC_FILES))
    OBJS := $(addprefix $(OBJS_ROOT_DIR)/,$(addsuffix .o,$(basename $(SRC_FILES))))
endif

# Convert C/C++ library files to object files
_LIB_DIRS :=
LIB_OBJS :=
ifdef LIB_FILES
    LIB_FILES := $(foreach d,$(LIB_FILES) $(PRIVATE_LIB_FILES),$(abspath $d))
    LIB_FILES := $(patsubst $(abspath $(P))/%,%,$(LIB_FILES))
    _LIB_DIRS := $(dir $(LIB_FILES))
    LIB_OBJS := $(addprefix $(OBJS_ROOT_DIR)/,$(addsuffix .o,$(basename $(LIB_FILES))))
endif

BUILD_FOLDERS := $(sort $(_SRC_DIRS) $(_LIB_DIRS))
VPATH += $(abspath $(P))

# Do we compile a library or a standalone application ?
ifdef LIB_OBJS
    # libfoo.a and libfoo.so and avoid adding extra lib if already set
    SHARED_LIB_NAME := $(TARGET_NAME)$(EXT_DYNAMIC_LIB)
    STATIC_LIB_NAME := $(TARGET_NAME)$(EXT_STATIC_LIB)
    ifneq "$(filter lib%,$(TARGET_NAME))" "$(TARGET_NAME)"
        SHARED_LIB_NAME := lib$(SHARED_LIB_NAME)
        STATIC_LIB_NAME := lib$(STATIC_LIB_NAME)
    endif

    # build/libfoo.a and build/libfoo.so
    # For Emscripten and ExaequOS shared lib are not managed
    TARGET_STATIC_LIB_NAME := $(BUILD_PATH)/$(STATIC_LIB_NAME)
    ifneq ($(OS),Emscripten)
        TARGET_SHARED_LIB_NAME := $(BUILD_PATH)/$(SHARED_LIB_NAME)
        PKG_FILE_NAME := $(TARGET_NAME).pc
        TARGET_PKG_FILE_NAME := $(BUILD_PATH)/$(PKG_FILE_NAME)
    else
        SHARED_LIB_NAME :=
    endif

    # Check if we are not trying to disable both static and shared library compilation
    ifdef DO_NOT_COMPILE_STATIC_LIB
        ifdef DO_NOT_COMPILE_SHARED_LIB
            $(error Cannot disable both static and shared library compilation when LIB_OBJS is defined. At least one must be compiled.)
        endif
    endif

    # User do not want to compile static library
    ifdef DO_NOT_COMPILE_STATIC_LIB
        TARGET_STATIC_LIB_NAME :=
        STATIC_LIB_NAME :=
    endif

    # User do not want to compile shared library
    ifdef DO_NOT_COMPILE_SHARED_LIB
        TARGET_SHARED_LIB_NAME :=
        SHARED_LIB_NAME :=
    endif
endif

# BUILD_MACOS_APP_BUNDLE is often set after include $(M)/project/Makefile
# (e.g. once OS is known). Apply bundle settings here, before target names.
ifdef BUILD_MACOS_APP_BUNDLE
ifeq ($(OS),Darwin)
EXT_BINARY := .app
MACOS_APP_BINARY ?= $(OBJS_ROOT_DIR)/$(TARGET_NAME)
endif
endif

# Define the target binary base name and name.
TARGET_BINARY_BASE_NAME :=
TARGET_BINARY_NAME :=
ifdef OBJS
    TARGET_BINARY_BASE_NAME := $(BUILD_PATH)/$(TARGET_NAME)
    TARGET_BINARY_NAME := $(TARGET_BINARY_BASE_NAME)$(EXT_BINARY)
endif
RUNNING_TARGET_NAME ?= $(TARGET_NAME)
RUNNING_TARGET_NAME := $(BUILD_PATH)/$(RUNNING_TARGET_NAME)$(EXT_BINARY)

# Convert <path>/libtoto.a into -ltoto
SHORT_INTERNAL_LIBS := $(subst $(BUILD_PATH)/lib,-l,$(INTERNAL_LIBS))
SHORT_INTERNAL_LIBS := $(subst .a,,$(SHORT_INTERNAL_LIBS))

INCLUDES := $(addprefix -I,$(abspath $(INCLUDES) $(GENERATION_DIR) $(DOWNLOAD_DIR)))

###############################################################################
# Store files dependencies in *.d files: when a file is modified this will
# compile others which depend on it.
###############################################################################

# Improved dependency tracking with better error handling
DEPFLAGS = -MT $(abspath $@) -MMD -MP -MF $(abspath $(OBJS_ROOT_DIR)/$*.Td)
POSTCOMPILE = $(MV) $(abspath $(OBJS_ROOT_DIR)/$*.Td) $(abspath $(OBJS_ROOT_DIR)/$*.d) && touch $(abspath $@)

# Create dependency directories if they don't exist
ifndef ORCHESTRATOR_MODE
  ifneq ($(strip $(OBJS) $(LIB_OBJS)),)
    $(shell $(MKDIR) $(sort $(dir $(patsubst %,%.d,$(basename $(OBJS) $(LIB_OBJS))))))
  endif
endif

###############################################################################
# Libraries installed on the operating system and known from pkg-config.
###############################################################################

ifdef PKG_LIBS
    SEARCH_CMD :=
    # Define where to install pkg-config file
    ifdef EXAEQUOS
        PKG_CONFIG_SEARCH_PATH := $(abspath $(P)/exapkgs/pkgconfigs)
    else
        PKG_CONFIG_SEARCH_PATH ?=
    endif

    ifdef PKG_CONFIG_SEARCH_PATH
        SEARCH_CMD := PKG_CONFIG_PATH="$(PKG_CONFIG_SEARCH_PATH)"
    endif

    # Cache pkg-config results to avoid repeated calls
    ifndef PKGCFG_LIBS_CACHED
        ifdef STATIC_PKG_LIBS
            PKGCFG_LIBS := $(shell $(SEARCH_CMD) pkg-config --static --libs $(PKG_LIBS) 2>/dev/null)
        else
            PKGCFG_LIBS := $(shell $(SEARCH_CMD) pkg-config --libs $(PKG_LIBS) 2>/dev/null)
        endif
        PKGCFG_CFLAGS := $(shell $(SEARCH_CMD) pkg-config --cflags $(PKG_LIBS) 2>/dev/null)
        PKGCFG_LIBS_CACHED := 1
    endif
    SEARCH_CMD :=
endif

###############################################################################
# Enhanced progress bar with time estimation
###############################################################################

# Only calculate progress if we have objects to compile
ifneq ($(words $(OBJS) $(LIB_OBJS)),0)
    PROGRESSBAR_PERCENT := 0
    PROGRESSBAR_DISPLAY := 0
    PROGRESSBAR_COUNT_ELEMENTS := $(words $(OBJS) $(LIB_OBJS))
    PROGRESSBAR_INCREMENT := $(shell expr 10000 / $(PROGRESSBAR_COUNT_ELEMENTS))
    define progressbar-update-percent
        $(eval PROGRESSBAR_PERCENT=$(shell expr $(PROGRESSBAR_PERCENT) + $(PROGRESSBAR_INCREMENT)))
        $(eval PROGRESSBAR_DISPLAY=$(shell expr $(PROGRESSBAR_PERCENT) / 100))
    endef
    progress-in = $(call progressbar-update-percent) $(ECHO_COLOR) "$(PURPLE)[$(PROGRESSBAR_DISPLAY)%] $1 $(CYAN)$2$(COLOR_DEFAULT) <= $(YELLOW)$3$(COLOR_DEFAULT)"
    progress-out = $(call progressbar-update-percent) $(ECHO_COLOR) "$(PURPLE)[$(PROGRESSBAR_DISPLAY)%] $1 $(CYAN)$2$(COLOR_DEFAULT) => $(YELLOW)$3$(COLOR_DEFAULT)"
else
    # Fallback for when no compilation is needed
    progress-in = $(ECHO_COLOR) "$(PURPLE)*** $1 $(CYAN)$2$(COLOR_DEFAULT) <= $(YELLOW)$3$(COLOR_DEFAULT)"
    progress-out = $(ECHO_COLOR) "$(PURPLE)*** $1 $(CYAN)$2$(COLOR_DEFAULT) => $(YELLOW)$3$(COLOR_DEFAULT)"
endif

###############################################################################
#? Install a list of existing files $1 to destination $2.
define INSTALL_FILES
	@$(call print-install,$(TARGET_NAME),$1,$2)
	$(Q)$(foreach folder,$(sort $(dir $1)),install -d -m 755 "$(abspath $2/$(folder))";)
 	$(Q)$(foreach file,$1,install -m 644 "$(abspath $(file))" "$2/$(file)";)
endef

###############################################################################
#? Install list of files that may not exist from folder $1 to destination folder $2.
define INSTALL_OPTIONAL_FILES
	@$(call INSTALL_FILES,$(wildcard $1),$2)
endef

###############################################################################
#? From folder $1, install a list of files $2 to destination $3.
define INSTALL_FILES_FROM_FOLDER
	@$(call print-install,$(TARGET_NAME),$2,$3, [$(abspath $1)])
	$(Q)$(foreach folder,$(sort $(dir $2)),install -d -m 755 "$(abspath $3/$(folder))";)
	$(Q)$(foreach file,$2,install -m 644 "$(abspath $1/$(file))" "$3/$(file)";)
endef

###############################################################################
#? Install recursively all C++ header files from folder $1 to destination $2.
define INSTALL_CXX_HEADER_FILES
	@$(call INSTALL_FILES_FROM_FOLDER,$1,$(strip $(subst $1/,,$(call rwildcard,$1,$(REGEXP_CXX_HEADER_FILES)))),$2)
endef

###############################################################################
#? Install a list of existing folders $1 to destination $2.
define INSTALL_FOLDERS
	@$(call print-install,$(TARGET_NAME),$1,$2)
    $(Q)install -d -m 755 $(foreach d,$1,$2/$(notdir $(d)))
    $(Q)$(foreach d,$1,cp -fr $(d)/. $2/$(notdir $(d)); )
endef

###############################################################################
#? Install a list of folders $1 that may not exist to destination $2.
define INSTALL_OPTIONAL_FOLDERS
	@$(if $(wildcard $1),$(call INSTALL_FOLDERS,$(wildcard $1),$2))
endef

###############################################################################
#? Install the project doc and generated doc folders.
define INSTALL_PROJECT_DOC
	@$(call INSTALL_OPTIONAL_FOLDERS,$(PROJECT_DOC_DIR) $(PROJECT_GENERATED_DOC),$(INSTALL_PROJECT_DIR))
endef

###############################################################################
#? Install the project data folder and its content.
define INSTALL_PROJECT_DATA
	@$(call INSTALL_OPTIONAL_FOLDERS,$(PROJECT_DATA_DIR),$(INSTALL_PROJECT_DIR))
endef

###############################################################################
#? Install the project information files: license, readme, version, changelog ...
define INSTALL_PROJECT_OTHER_FILES
	@$(call INSTALL_OPTIONAL_FILES,$(PROJECT_OTHER_FILES),$(INSTALL_PROJECT_DIR))
endef

###############################################################################
#? Install the generated project_info.hpp header file
# FIXME le chemin de destination est incorrect: il faudrait trouver le dossier de la lib qui ne correspond pas au target_name
define INSTALL_PROJECT_INFO_HEADER
	@$(call print-install,$(TARGET_NAME),$(PROJECT_INFO_HPP),$(INSTALL_INCLUDEDIR))
	@$(call INSTALL_FILES_FROM_FOLDER,$(GENERATION_DIR),$(PROJECT_INFO_HPP),$(INSTALL_INCLUDEDIR)/$(TARGET_NAME))
endef

###############################################################################
#? Install hpp files from include/ folder into INSTALL_INCLUDEDIR
define INSTALL_INCLUDE_FOLDER
    @$(call INSTALL_CXX_HEADER_FILES,$(P)/include,$(INSTALL_INCLUDEDIR))
endef

###############################################################################
#? Install hpp files from src/ folder into INSTALL_INCLUDEDIR
define INSTALL_PROJECT_HEADERS_FROM_SRC_FOLDER
    @$(call INSTALL_CXX_HEADER_FILES,$(P)/src,$(INSTALL_INCLUDEDIR))
endef

###############################################################################
#? Install the project pkg-config file
define INSTALL_PROJECT_PKG_CONFIG
	@$(call print-install,$(TARGET_NAME),$(PKG_FILE_NAME),$(INSTALL_PKGLIBDIR))
	$(Q)install -d -m 755 $(INSTALL_PKGLIBDIR)
	$(Q)install -m 644 $(TARGET_PKG_FILE_NAME) $(INSTALL_PKGLIBDIR)/$(TARGET_NAME)-$(PROJECT_VERSION).pc
	$(Q)cd $(INSTALL_PKGLIBDIR) && ln -snf $(TARGET_NAME)-$(PROJECT_VERSION).pc $(PKG_FILE_NAME)
endef

###############################################################################
#? Install shared libraries
define INSTALL_PROJECT_SHARED_LIBRARIES
	@$(call print-install,$(TARGET_NAME),$(SHARED_LIB_NAME),$(INSTALL_LIBDIR))
	$(Q)install -d -m 755 $(INSTALL_LIBDIR)
	$(Q)install -m 644 $(TARGET_SHARED_LIB_NAME) $(INSTALL_LIBDIR)/$(SHARED_LIB_NAME).$(PROJECT_VERSION)
	$(Q)cd $(INSTALL_LIBDIR) && ln -snf $(SHARED_LIB_NAME).$(PROJECT_VERSION) $(SHARED_LIB_NAME)
	$(Q)cd $(INSTALL_LIBDIR) && ln -snf $(SHARED_LIB_NAME).$(PROJECT_VERSION) $(SHARED_LIB_NAME).$(PROJECT_VERSION_MAJOR)
endef

###############################################################################
#? Install static libraries
define INSTALL_PROJECT_STATIC_LIBRARIES
	@$(call print-install,$(TARGET_NAME),$(STATIC_LIB_NAME),$(INSTALL_LIBDIR))
	$(Q)install -d -m 755 $(INSTALL_LIBDIR)
	$(Q)install -m 644 $(TARGET_STATIC_LIB_NAME) $(INSTALL_LIBDIR)
endef

###############################################################################
#? Install the project binary
ifdef EXAEQUOS
define INSTALL_PROJECT_BINARY
	@$(call print-install,$(TARGET_NAME),$(TARGET_NAME),$(INSTALL_BINDIR))
	$(Q)install -d -m 755 $(INSTALL_BINDIR)
	$(Q)install -m 644 $(TARGET_BINARY_BASE_NAME).js $(INSTALL_BINDIR)/$(TARGET_NAME).js
	$(Q)install -m 644 $(TARGET_BINARY_BASE_NAME).wasm $(INSTALL_BINDIR)/$(TARGET_NAME).wasm
	$(call CREATE_EXAEQUOS_HTML,exa.html)
endef
else
define INSTALL_PROJECT_BINARY
	@$(call print-install,$(TARGET_NAME),$(TARGET_NAME),$(INSTALL_BINDIR))
	$(Q)install -d -m 755 $(INSTALL_BINDIR)
	$(Q)install -m 644 $(TARGET_BINARY_NAME) $(INSTALL_BINDIR)/$(TARGET_NAME)-$(PROJECT_VERSION)
	$(Q)cd $(INSTALL_BINDIR) && ln -sf $(TARGET_NAME)-$(PROJECT_VERSION) $(TARGET_NAME)
endef
endif

###############################################################################
# Install all .hpp files selected by HEADER_FILES
# Helper macro: Install files from absolute paths, converting them to relative paths
# $1: List of files (absolute paths)
# $2: Destination directory
# This converts paths like $(P)/src/MyProject/foo/bar.hpp to MyProject/foo/bar.hpp
###############################################################################
define INSTALL_ABSOLUTE_FILES_AS_RELATIVE
	@$(call print-install,$(TARGET_NAME),$1,$2)
	$(Q)P_ROOT="$(abspath $(P))"; \
	DEST="$(abspath $2)"; \
	for file in $1; do \
		FILE="$$(readlink -f "$$file" 2>/dev/null || echo "$$file")"; \
		SRC_PATH="$${FILE#$$P_ROOT/src/}"; \
		INC_PATH="$${FILE#$$P_ROOT/include/}"; \
		if [ "$$SRC_PATH" != "$$FILE" ]; then \
			REL_PATH="$$SRC_PATH"; \
		elif [ "$$INC_PATH" != "$$FILE" ]; then \
			REL_PATH="$$INC_PATH"; \
		else \
			REL_PATH="$$(basename "$$FILE")"; \
		fi; \
		REL_DIR="$$(dirname "$$REL_PATH")"; \
		mkdir -p "$$DEST/$$REL_DIR" && \
		install -m 644 "$$FILE" "$$DEST/$$REL_PATH"; \
	done
endef

###############################################################################
#? Install the selected header files
define INSTALL_SELECTED_HEADERS
    @$(call INSTALL_ABSOLUTE_FILES_AS_RELATIVE,$(HEADER_FILES),$(INSTALL_INCLUDEDIR))
endef

###############################################################################
# Create a MacOS application bundle
###############################################################################

define CREATE_MACOS_INFO_PLIST
	@$(ECHO) "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > $1
	@$(ECHO) "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" >> $1
	@$(ECHO) "<plist version=\"1.0\">" >> $1
	@$(ECHO) "<dict>" >> $1
	@$(ECHO) "    <key>CFBundleExecutable</key>" >> $1
	@$(ECHO) "    <string>$(TARGET_NAME)</string>" >> $1
	@$(ECHO) "    <key>CFBundleIdentifier</key>" >> $1
	@$(ECHO) "    <string>com.$(APPLE_IDENTIFIER).$(TARGET_NAME)</string>" >> $1
	@$(ECHO) "    <key>CFBundleInfoDictionaryVersion</key>" >> $1
	@$(ECHO) "    <string>6.0</string>" >> $1
	@$(ECHO) "    <key>CFBundleName</key>" >> $1
	@$(ECHO) "    <string>$(TARGET_NAME)</string>" >> $1
	@$(ECHO) "    <key>CFBundlePackageType</key>" >> $1
	@$(ECHO) "    <string>APPL</string>" >> $1
	@$(if $(wildcard $(MACOS_BUNDLE_ICON)),\
		$(ECHO) "    <key>CFBundleIconFile</key>" >> $1 ;\
		$(ECHO) "    <string>$(TARGET_NAME).icns</string>" >> $1)
	@$(ECHO) "</dict>" >> $1
	@$(ECHO) "</plist>" >> $1
endef

###############################################################################
# Create a MacOS application bundle
###############################################################################

define CREATE_MACOS_APP_BUNDLE
	@$(call print-to,Generating bundle application,$(TARGET_NAME),$1)
	$(Q)$(RM) $1 2> /dev/null || true
	$(Q)$(MKDIR) $1/Contents/MacOS
	$(Q)$(MKDIR) $1/Contents/Resources
	$(Q)$(CP) $(MACOS_APP_BINARY) $1/Contents/MacOS/$(TARGET_NAME)
	$(Q)$(if $(wildcard $(MACOS_BUNDLE_ICON)),$(Q)$(CP) $(MACOS_BUNDLE_ICON) $1/Contents/Resources/$(TARGET_NAME).icns)
	$(Q)cp -r $(PROJECT_DATA_DIR)/* $1/Contents/Resources/
	@$(call CREATE_MACOS_INFO_PLIST,$1/Contents/Info.plist)
endef

###############################################################################
# Create the pkg-config file (if the project is a library)
###############################################################################

define CREATE_PKG_CONFIG
	@$(call print-to,Generating pkg-config,$(TARGET_NAME),$(abspath $1))
	@$(ECHO) "prefix=$(PREFIX)" > $1
	@$(ECHO) "exec_prefix=$(PREFIX)" >> $1
	@$(ECHO) "libdir=$(LIBDIR)" >> $1
	@$(ECHO) "includedir=$(INCLUDEDIR)/$(PROJECT_NAME)/$(PROJECT_VERSION)" >> $1
	@$(ECHO) "Name: lib$(TARGET_NAME)" >> $1
	@$(ECHO) "Description: $(TARGET_DESCRIPTION)" >> $1
	@$(ECHO) "Version: $(PROJECT_VERSION)" >> $1
	@$(ECHO) "Libs: -L$$""{libdir} -l$(TARGET_NAME)" >> $1
	@$(ECHO) "Libs.private: $(strip $(SHORT_INTERNAL_LIBS) $(NOT_PKG_LIBS) $(if $(PKG_LIBS),,$(PKGCFG_LIBS)))" >> $1
	$(Q)if [ -n "$(strip $(PKG_LIBS))" ]; then \
		$(ECHO) "Requires.private: $(PKG_LIBS)" >> $1; \
	fi
	@$(ECHO) "Cflags: -I$$""{includedir} $(PKGCFG_CFLAGS)" >> $1
endef

###############################################################################
# Create the RPM spec file
###############################################################################

define CREATE_RPM_SPEC
	@$(call print-to,Generating RPM spec,$(TARGET_NAME),$(abspath $1))
	@$(ECHO) "Name:           $(PROJECT_NAME)" > $1
	@$(ECHO) "Version:        $(PROJECT_VERSION)" >> $1
	@$(ECHO) "Release:        1%{?dist}" >> $1
	@$(ECHO) "Summary:        $(TARGET_DESCRIPTION)" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "License:        $(LICENSE)" >> $1
	@$(ECHO) "URL:            https://github.com/$(shell echo "$(AUTHOR)/$(PROJECT_NAME)" | tr A-Z a-z)" >> $1
	@$(ECHO) "BuildRequires:  gcc-c++ >= 4.8, make >= 3.81" >> $1
	@$(ECHO) "Requires:       glibc" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "%description" >> $1
	@$(ECHO) "$(TARGET_DESCRIPTION)" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "%prep" >> $1
	@$(ECHO) "# Working directly in source directory" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "%build" >> $1
	@$(ECHO) "cd $(abspath $(P))" >> $1
	@$(ECHO) "make BUILD=%{_builddir} download-external-libs" >> $1
	@$(ECHO) "make BUILD=%{_builddir} compile-external-libs" >> $1
	@$(ECHO) "make BUILD=%{_builddir} COMPILATION_MODE=release %{?_smp_mflags}" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "%install" >> $1
	@$(ECHO) "cd $(abspath $(P))" >> $1
	@$(ECHO) "make BUILD=%{_builddir} DESTDIR=%{buildroot} PREFIX=/usr install" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "%files" >> $1
	@if [ -n "$(TARGET_BINARY_NAME)" ]; then \
		$(ECHO) "/usr/bin/$(TARGET_NAME)" >> $1; \
		$(ECHO) "/usr/bin/$(TARGET_NAME)-$(PROJECT_VERSION)" >> $1; \
	fi
	@if [ -n "$(TARGET_SHARED_LIB_NAME)" ]; then \
		$(ECHO) "/usr/lib/lib$(TARGET_NAME).so" >> $1; \
		$(ECHO) "/usr/lib/lib$(TARGET_NAME).so.*" >> $1; \
	fi
	@if [ -n "$(TARGET_STATIC_LIB_NAME)" ]; then \
		$(ECHO) "/usr/lib/lib$(TARGET_NAME).a" >> $1; \
	fi
	@if [ -n "$(TARGET_PKG_FILE_NAME)" ]; then \
		$(ECHO) "/usr/lib/pkgconfig/$(TARGET_NAME).pc" >> $1; \
		$(ECHO) "/usr/lib/pkgconfig/$(TARGET_NAME)-$(PROJECT_VERSION).pc" >> $1; \
	fi
	@$(ECHO) "/usr/bin/*demo*" >> $1
	@$(ECHO) "%dir /usr/include/$(PROJECT_NAME)" >> $1
	@$(ECHO) "%dir /usr/include/$(PROJECT_NAME)/$(PROJECT_VERSION)" >> $1
	@$(ECHO) "/usr/include/$(PROJECT_NAME)/$(PROJECT_VERSION)/*" >> $1
	@$(ECHO) "%dir /usr/share/$(PROJECT_NAME)" >> $1
	@$(ECHO) "%dir /usr/share/$(PROJECT_NAME)/$(PROJECT_VERSION)" >> $1
	@$(ECHO) "/usr/share/$(PROJECT_NAME)/$(PROJECT_VERSION)/*" >> $1
	@$(ECHO) "" >> $1
	@$(ECHO) "%changelog" >> $1
	@if [ -f "$(P)/ChangeLog" ]; then \
		$(ECHO) "# Changelog from ChangeLog file:" >> $1; \
		head -20 "$(P)/ChangeLog" | sed 's/^/# /' >> $1; \
	else \
		$(ECHO) "* $$(date '+%a %b %d %Y') $(USER) <$(USER)@localhost> - $(PROJECT_VERSION)-1" >> $1; \
		$(ECHO) "- Release $(PROJECT_VERSION)" >> $1; \
	fi
endef

###############################################################################
# Create the exa.html file for ExaequOS
###############################################################################

define CREATE_EXAEQUOS_HTML
	@$(call print-to,"Generating","$(TARGET_NAME)","$1","")
	@$(ECHO) "<!doctype html>" > $(INSTALL_BINDIR)/$1
	@$(ECHO) "<html lang=\"en-us\">" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "<head>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "  <meta charset=\"utf-8\">" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "  <title>Emscripten-Generated Code</title>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "  <style>" >> $(INSTALL_BINDIR)/$1
 	@$(ECHO) "   html, body {" >> $(INSTALL_BINDIR)/$1
 	@$(ECHO) "     margin : 0;" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "      padding: 0;" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "      border: none;" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "      width: 100%;" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "      height: 100%;" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "    }" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "  </style>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "</head>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "<body>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "  <script async type=\"text/javascript\" src=\"$(TARGET_NAME).js\"></script>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "</body>" >> $(INSTALL_BINDIR)/$1
	@$(ECHO) "</html>" >> $(INSTALL_BINDIR)/$1
endef

###############################################################################
# Create Doxyfile files
###############################################################################

define RULE_CREATE_DOXYFILES
    $(Q)$(MKDIR) $(dir $(GENERATED_DOXYGEN_DIR))
	$(CP) $(PATH_MYMAKEFILE_DOXYFILE)/. $(GENERATED_DOXYGEN_DIR)
	# Copy appropriate footer template based on detected license
	$(Q)if [ -f "$(PATH_MYMAKEFILE_DOXYFILE)/footer-$(LICENSE).html" ]; then \
		$(CP) "$(PATH_MYMAKEFILE_DOXYFILE)/footer-$(LICENSE).html" "$(GENERATED_DOXYGEN_DIR)/footer-$(LICENSE).html"; \
	else \
		$(ECHO) "Warning: No footer template found for license $(LICENSE), using UNKNOWN.html instead" >&2; \
		$(CP) "$(PATH_MYMAKEFILE_DOXYFILE)/footer-UNKNOWN.html" "$(GENERATED_DOXYGEN_DIR)/footer-$(LICENSE).html"; \
	fi
	# Apply sed replacements to all files in the generated doxygen directory
	$(Q)for file in $(GENERATED_DOXYGEN_DIR)/Doxyfile $(GENERATED_DOXYGEN_DIR)/doxygen.css $(GENERATED_DOXYGEN_DIR)/header.html $(GENERATED_DOXYGEN_DIR)/footer-$(LICENSE).html; do \
		sed -i 's#%VERSION#$(PROJECT_VERSION)#g' $$file; \
		sed -i 's#%PROJECT#$(PROJECT_NAME)#g' $$file; \
		sed -i 's#%DESCRIPTION#$(subst /,\/,$(TARGET_DESCRIPTION))#g' $$file; \
		sed -i 's#%LOGO#$(subst /,\/,$(PATH_PROJECT_LOGO))#g' $$file; \
        sed -i 's#%TARGET#$(TARGET_NAME)#g' $$file; \
		sed -i 's#%LICENSE#$(LICENSE)#g' $$file; \
		sed -i 's#%GENDOC#$(subst /,\/,$(GENERATED_DOXYGEN_DIR))#g' $$file; \
		sed -i 's#%INPUT#$(subst /,\/,$(DOXYGEN_INPUTS))#g' $$file; \
	done
	$(MV) $(GENERATED_DOXYGEN_DIR)/Doxyfile $(abspath $(P)/Doxyfile)
endef

###############################################################################
# git clone a project to a given destination. Repo syntax "user/repo@sha1:recurse".
###############################################################################

define GIT_CLONE
	$(BASH) $(SCRIPT_GIT_CLONE) $(PROJECT_NAME) $2 $1
endef

###############################################################################
#? Compile the standalone application or static/shared libraries.
###############################################################################

all: compiler-info print-target creating-build-folder pre-build $(DIRS_WITH_MAKEFILE) $(TARGET_SHARED_LIB_NAME) $(TARGET_STATIC_LIB_NAME) $(TARGET_PKG_FILE_NAME) $(TARGET_BINARY_NAME) post-build

###############################################################################
#? Pre-build actions: rules to extend before the build process.
###############################################################################

.PHONY: pre-build
pre-build::

###############################################################################
#? Post-build actions: rules to extend after the build process.
###############################################################################

.PHONY: post-build
post-build::

###############################################################################
# Compile sub Makefiles we depend on.
###############################################################################

ifdef DIRS_WITH_MAKEFILE
.PHONY: $(DIRS_WITH_MAKEFILE)
$(DIRS_WITH_MAKEFILE):
	$(Q)$(MAKE) --no-print-directory $(addprefix --directory=,$@) all
endif

###############################################################################
#? Ensure .clang-format file exists at the project root.
###############################################################################

.PHONY: ensure-clang-format
ensure-clang-format:
ifeq ("$(wildcard $(P)/.clang-format)","")
	@$(call print-to,Copying default clang-format,$(M)/assets/format/clang-format,$(P)/.clang-format)
	$(Q)$(CP) $(M)/assets/format/clang-format $(P)/.clang-format
endif

###############################################################################
# Link sources as static library.
###############################################################################

ifdef TARGET_STATIC_LIB_NAME
$(TARGET_STATIC_LIB_NAME): $(LIB_OBJS) $(INTERNAL_LIBS) $(LIST_MYMAKEFILES) $(DIRS_WITH_MAKEFILE)
	@$(call print-to,"Creating static library","$(TARGET_NAME)","$@")
	@$(MKDIR) $(dir $@)
ifdef THIRD_PARTIES_LIBS
	@$(call extract-objs-from-static-libs)
endif
	$(Q)$(RM) $@ 2>/dev/null || true
	$(Q)$(CD) $(abspath $(OBJS_ROOT_DIR)) && \
    $(AR) $(ARFLAGS) $@ $(patsubst $(abspath $(BUILD_PATH)/objs)/%,%,$(abspath $(LIB_OBJS) $(EXTRACTED_OBJS))) && \
    ranlib $@
endif

###############################################################################
# Link sources as shared library.
###############################################################################

ifdef TARGET_SHARED_LIB_NAME
$(TARGET_SHARED_LIB_NAME): | $(TARGET_STATIC_LIB_NAME)
$(TARGET_SHARED_LIB_NAME): $(LIB_OBJS) $(INTERNAL_LIBS) $(LIST_MYMAKEFILES) $(DIRS_WITH_MAKEFILE)
	@$(call print-to,"Creating shared library","$(TARGET_NAME)","$@")
	@$(MKDIR) $(dir $@)
	$(Q)$(COMPILER) $(LDFLAGS) $(DLL_LDFLAGS) -o $@ $(abspath $(LIB_OBJS)) $(LINKER_ALL_LOAD) $(THIRD_PARTIES_LIBS) $(INTERNAL_LIBS) $(NOT_PKG_LIBS) $(PKGCFG_LIBS) $(LINKER_NO_ALL_LOAD) $(LINKER_FLAGS) $(USER_LDFLAGS)
endif

###############################################################################
# Generate the pkg-config file.
###############################################################################

ifdef TARGET_PKG_FILE_NAME
$(TARGET_PKG_FILE_NAME): | $(TARGET_SHARED_LIB_NAME)
$(TARGET_PKG_FILE_NAME): $(LIB_OBJS) $(INTERNAL_LIBS) $(LIST_MYMAKEFILES) $(DIRS_WITH_MAKEFILE)
	@$(call CREATE_PKG_CONFIG,$(TARGET_PKG_FILE_NAME))
endif

###############################################################################
# Link sources to standalone application.
###############################################################################

ifdef OBJS
$(TARGET_BINARY_NAME): $(OBJS) $(LIST_MYMAKEFILES) $(DIRS_WITH_MAKEFILE)
	@$(call print-to,Linking,$(TARGET_NAME),$@)
	@$(MKDIR) $(dir $@)
ifdef BUILD_MACOS_APP_BUNDLE
	@$(MKDIR) $(OBJS_ROOT_DIR)
	$(Q)$(COMPILER) $(LDFLAGS) $(OBJS) $(THIRD_PARTIES_OBJS) -o $(MACOS_APP_BINARY) $(THIRD_PARTIES_LIBS) $(INTERNAL_LIBS) $(NOT_PKG_LIBS) $(PKGCFG_LIBS) $(LINKER_FLAGS) $(USER_LDFLAGS)
ifdef STRIP
	@$(call print-to,Stripping,$(TARGET_NAME),$(MACOS_APP_BINARY))
	$(Q)$(STRIP) $(MACOS_APP_BINARY) || $(ECHO_COLOR) "$(YELLOW)Warning: Failed to strip symbols$(COLOR_DEFAULT)"
endif
	$(Q)$(call CREATE_MACOS_APP_BUNDLE,$(abspath $(TARGET_BINARY_NAME)))
else
	$(Q)$(COMPILER) $(LDFLAGS) $(OBJS) $(THIRD_PARTIES_OBJS) -o $(TARGET_BINARY_NAME) $(THIRD_PARTIES_LIBS) $(INTERNAL_LIBS) $(NOT_PKG_LIBS) $(PKGCFG_LIBS) $(LINKER_FLAGS) $(USER_LDFLAGS)
ifdef STRIP
	@$(call print-to,Stripping,$(TARGET_NAME),$@)
	$(Q)$(STRIP) $(TARGET_BINARY_NAME) || $(ECHO_COLOR) "$(YELLOW)Warning: Failed to strip symbols$(COLOR_DEFAULT)"
endif
endif
	@$(call print-to,Build completed,$(TARGET_NAME),$@)
endif

###############################################################################
# Compile C++ sources. Note: abspath is needed for gcov.
###############################################################################

# Create object directory structure before compilation
$(OBJS_ROOT_DIR)/%.o: %.cpp $(OBJS_ROOT_DIR)/%.d $(LIST_MYMAKEFILES) $(DEPS_DEBUG_LIBS) | $(OBJS_ROOT_DIR) $(DIRS_WITH_MAKEFILE)
	@$(call progress-in,Compiling C++,$(TARGET_NAME),$<)
	@$(MKDIR) $(dir $@)
	$(Q)$(CXX) $(CXX_STANDARD) $(DEPFLAGS) $(CXXFLAGS) $(PKGCFG_CFLAGS) $(OPTIM_FLAGS) $(DEFINES) $(INCLUDES) $(USER_CXXFLAGS) -c $(abspath $<) -o $(abspath $@)
	@$(POSTCOMPILE)

###############################################################################
# Compile C sources. Note: abspath is needed for gcov.
###############################################################################

# Create object directory structure before compilation
$(OBJS_ROOT_DIR)/%.o: %.c $(OBJS_ROOT_DIR)/%.d $(LIST_MYMAKEFILES) $(DEPS_DEBUG_LIBS) | $(OBJS_ROOT_DIR) $(DIRS_WITH_MAKEFILE)
	@$(call progress-in,Compiling C,$(TARGET_NAME),$<)
	@$(MKDIR) $(dir $@)
	$(Q)$(CC) $(DEPFLAGS) $(CCFLAGS) $(PKGCFG_CFLAGS) $(OPTIM_FLAGS) $(DEFINES) $(INCLUDES) $(USER_CCFLAGS) -c $(abspath $<) -o $(abspath $@)
	@$(POSTCOMPILE)

###############################################################################
# Assembly sources support (optional)
###############################################################################

ifdef ENABLE_ASM_SUPPORT
$(OBJS_ROOT_DIR)/%.o: %.s $(OBJS_ROOT_DIR)/%.d $(LIST_MYMAKEFILES) | $(OBJS_ROOT_DIR)
	@$(call progress-in,Assembling,$(TARGET_NAME),$<)
	@$(MKDIR) $(dir $@)
	$(Q)$(CC) $(DEPFLAGS) $(CCFLAGS) $(DEFINES) $(INCLUDES) -c $(abspath $<) -o $(abspath $@)
	@$(POSTCOMPILE)

$(OBJS_ROOT_DIR)/%.o: %.S $(OBJS_ROOT_DIR)/%.d $(LIST_MYMAKEFILES) | $(OBJS_ROOT_DIR)
	@$(call progress-in,Assembling,$(TARGET_NAME),$<)
	@$(MKDIR) $(dir $@)
	$(Q)$(CC) $(DEPFLAGS) $(CCFLAGS) $(DEFINES) $(INCLUDES) -c $(abspath $<) -o $(abspath $@)
	@$(POSTCOMPILE)
endif

###############################################################################
# Create a build folder in which compiled files will be stored.
###############################################################################

.PHONY: creating-build-folder
creating-build-folder:
ifndef ORCHESTRATOR_MODE
	@$(call print-out,Create build folder,$(TARGET_NAME),$(GENERATED_HEADER_PROJECT_INFO))
  ifneq ($(strip $(BUILD_FOLDERS)),)
	@$(MKDIR) $(addprefix $(OBJS_ROOT_DIR)/,$(BUILD_FOLDERS))
  endif
  ifneq ($(strip $(DOWNLOAD_DIR)),)
	@$(MKDIR) $(DOWNLOAD_DIR)
  endif
endif

###############################################################################
# Generate a header file with the project information.
###############################################################################

$(LIB_OBJS) $(OBJS): | $(GENERATED_HEADER_PROJECT_INFO)
$(GENERATED_HEADER_PROJECT_INFO): $(LIST_MYMAKEFILES) | creating-build-folder
	@$(call print-out,Generating projet info,$(TARGET_NAME),$(GENERATED_HEADER_PROJECT_INFO))
	@$(MKDIR) $(GENERATION_DIR)
	@$(ECHO) "VERSION=$(PROJECT_VERSION)" > $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(ECHO) "PROJECT=$(PROJECT_NAME)" >> $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(ECHO) "TARGET=$(TARGET_NAME)" >> $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(ECHO) "TARGET_DESCRIPTION=\"$(TARGET_DESCRIPTION)\"" >> $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(ECHO) "DATA_PATHS=$(SEARCH_DATA_PATHS)" >> $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(ECHO) "TEMPDIR=$(PROJECT_TEMP_DIR)" >> $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(ECHO) "COMPILATION_MODE=$(COMPILATION_MODE)" >> $(GENERATION_DIR)/$(PROJECT_INFO_TXT)
	@$(BASH) $(SCRIPT_CONFIG) $(GENERATION_DIR)/$(PROJECT_INFO)

###############################################################################
# Auto-Dependency Generation
###############################################################################

.PRECIOUS: $(OBJS_ROOT_DIR)/%.d
$(OBJS_ROOT_DIR)/%.d: ;

-include $(patsubst %,%.d,$(basename $(OBJS) $(LIB_OBJS)))

###############################################################################
#? Download external github code source needed by this project.
###############################################################################
# FIXME: find something smarter for installing Exaequos packages
.PHONY: download-external-libs
download-external-libs::
ifdef EXAEQUOS
	@$(call print-to,Downloading ExaequOS packages,$(TARGET_NAME),$(THIRD_PARTIES_DIR))
	$(Q)$(RM) exapkgs 2> /dev/null
	$(Q)epm.py install exa-wayland glfw raylib
endif
	@$(call print-to,Downloading third-party libs,$(TARGET_NAME),$(THIRD_PARTIES_DIR))
ifneq ($(wildcard $(PROJECT_MANIFEST_THIRD_PARTIES)),)
	$(Q)cd $(THIRD_PARTIES_DIR) && $(BASH) $(SCRIPT_DOWNLOAD_THIRD_PARTIES) $(PROJECT_NAME) $(PROJECT_MANIFEST_THIRD_PARTIES)
else
	$(Q)$(ECHO_COLOR) "  $(YELLOW):( $(CYAN)Missing file $(PROJECT_MANIFEST_THIRD_PARTIES) $(YELLOW):("
	$(Q)$(ECHO_COLOR) "  $(YELLOW):) $(CYAN)Or no third-party libs to download ! $(YELLOW):)$(COLOR_DEFAULT)"
endif

###############################################################################
#? Compile external projects needed.
###############################################################################

.PHONY: compile-external-libs
compile-external-libs:
	@$(call print-to,Compiling third-party libs,$(TARGET_NAME),$(THIRD_PARTIES_DIR))
ifneq ($(wildcard $(PROJECT_SCRIPT_COMPILE_THIRD_PARTIES)),)
	$(Q)$(CD) $(THIRD_PARTIES_DIR) && $(BASH) $(SCRIPT_COMPILE_THIRD_PARTIES) $(PROJECT_NAME) $(PROJECT_SCRIPT_COMPILE_THIRD_PARTIES) $(OS) $(CC) $(CXX)
else
	$(Q)$(ECHO_COLOR) "  $(YELLOW):( $(CYAN)Missing file $(PROJECT_SCRIPT_COMPILE_THIRD_PARTIES) $(YELLOW):("
	$(Q)$(ECHO_COLOR) "  $(YELLOW):) $(CYAN)Or no third-party libs to compile ! $(YELLOW):)$(COLOR_DEFAULT)"
endif

###############################################################################
#? Clean the build folder.
###############################################################################

.PHONY: clean
clean::
ifeq ($(abspath .), $(abspath $(P)))
	@$(call print-simple,Cleaning project,$(PROJECT_NAME))
else
	@$(call print-simple,Cleaning target,$(TARGET_NAME))
endif
	$(Q)$(RM) $(BUILD_PATH) 2> /dev/null || true
ifdef DIRS_WITH_MAKEFILE
	$(Q)for d in $(DIRS_WITH_MAKEFILE); do \
	$(MAKE) --no-print-directory $(addprefix --directory=,$$d) clean; \
	done
endif

###############################################################################
#? Clean everything: build, third-parties, documentation, and generated files.
###############################################################################

.PHONY: veryclean
veryclean::
	@$(call print-simple,Deep cleaning project,$(PROJECT_NAME))
	$(Q)$(RM) $(BUILD_PATH) $(GENERATED_DOXYGEN_DIR) 2> /dev/null || true
	$(Q)$(RM) $(dir $(GPROF_ANALYSIS)) $(COVERAGE_DIR) 2> /dev/null || true
	$(Q)$(RM) $(P)/Doxyfile compile_commands.json 2>/dev/null || true
ifneq ($(wildcard $(PROJECT_MANIFEST_THIRD_PARTIES)),)
	$(Q)while IFS= read -r line || [ -n "$$line" ]; do \
		if [ -n "$$line" ] && [ "$${line#\#}" = "$$line" ]; then \
			repo_name=$${line%@*}; \
			repo_name=$${repo_name%:*}; \
			lib_name=$${repo_name##*/}; \
			if [ -d "$(THIRD_PARTIES_DIR)/$$lib_name" ]; then \
				$(RM) "$(THIRD_PARTIES_DIR)/$$lib_name" 2>/dev/null || true; \
			fi; \
		fi; \
	done < $(PROJECT_MANIFEST_THIRD_PARTIES)
endif
	$(Q)find $(P) -name "*.orig" -delete 2>/dev/null || true
	$(Q)find $(P) -name "*.rej" -delete 2>/dev/null || true
	$(Q)find $(P) -name "*~" -delete 2>/dev/null || true
	$(Q)find $(P) -name "gmon.out" -delete 2>/dev/null || true
	$(Q)find $(P) -name "*.gcda" -delete 2>/dev/null || true
	$(Q)find $(P) -name "*.gcno" -delete 2>/dev/null || true
	$(Q)find $(P) -name "*.gcov" -delete 2>/dev/null || true
	$(Q)find $(P) -name "perf.data*" -delete 2>/dev/null || true
	$(Q)find $(P) -name "*.prof" -delete 2>/dev/null || true

###############################################################################
#? Install the project artifacts on the operating system
###############################################################################

.PHONY: install
install::
ifdef INSTALL_LOCAL_PROJECT_DATA_DIR
	$(Q)install -d -m 755 $(INSTALL_LOCAL_PROJECT_DATA_DIR)
endif
ifdef DIRS_WITH_MAKEFILE
	$(Q)for d in $(DIRS_WITH_MAKEFILE); do \
	$(MAKE) --no-print-directory $(addprefix --directory=,$$d) install; \
	done
endif
ifdef TARGET_BINARY_NAME
	$(Q)$(call INSTALL_PROJECT_BINARY)
endif
ifneq ($(OS),Emscripten)
  ifdef TARGET_SHARED_LIB_NAME
	$(Q)$(call INSTALL_PROJECT_SHARED_LIBRARIES)
  endif
  ifdef TARGET_STATIC_LIB_NAME
	$(Q)$(call INSTALL_PROJECT_STATIC_LIBRARIES)
  endif
  ifdef TARGET_PKG_FILE_NAME
	$(Q)$(call INSTALL_PROJECT_PKG_CONFIG)
  endif
  ifdef HEADER_FILES
	$(Q)$(call INSTALL_SELECTED_HEADERS)
	$(Q)$(call INSTALL_PROJECT_INFO_HEADER)
  endif
  ifdef LIB_OBJS
	$(Q)$(call INSTALL_INCLUDE_FOLDER)
  endif
  ifeq ($(abspath .), $(abspath $(P)))
	$(Q)$(call INSTALL_PROJECT_DOC)
	$(Q)$(call INSTALL_PROJECT_DATA)
	$(Q)$(call INSTALL_PROJECT_OTHER_FILES)
  endif
endif

###############################################################################
#? Generate the code source documentation with doxygen.
###############################################################################

.PHONY: doc
doc:
	@$(call RULE_CREATE_DOXYFILES)
	$(Q)doxygen Doxyfile
	$(Q)xdg-open $(DOXYGEN_INDEX_HTML) & >/dev/null

###############################################################################
#? Compress project sources without in the goal to backup the code or share it.
# This will ignore .git, build, generated documentations.
# Possible tarball name conflict with existing one is managed: if a tarball
# already exists, the older will stay intact and a new one is created.
###############################################################################

.PHONY: tarball
tarball:
	$(Q)$(BASH) $(SCRIPT_TAR_PROJECT) $(abspath $(P)) $(TARGET_NAME) $(THIRD_PARTIES_DIR) $(PROJECT_VERSION)

###############################################################################
#? Display the compiler version and information.
###############################################################################

.PHONY: compiler-info
compiler-info:
ifeq ($(abspath .), $(abspath $(P)))
	@$(call print-simple,About compiler,$(COMPILER),,)
	$(Q)$(COMPILER) --version
  ifeq ($(OS),Emscripten)
    ifdef EXAEQUOS
	@$(call print-info,Compiling with emscripten-exa for ExaequOS)
    else
	@$(call print-info,Compiling with emscripten)
    endif
  endif
endif

###############################################################################
#? Display the version of the project.
###############################################################################

.PHONY: project-version
project-version:
	@$(ECHO) $(PROJECT_VERSION)

###############################################################################
#? Display the name of the project.
###############################################################################

.PHONY: project-name
project-name:
	@$(ECHO) $(PROJECT_NAME)

###############################################################################
#? Display the name of the target.
###############################################################################

.PHONY: target-name
target-name:
	@$(ECHO) $(TARGET_NAME)

# Private rules
.PHONY: print-target
print-target:
	@$(call print-to,"[TARGET]",$(TARGET_DESCRIPTION),$(PROJECT_NAME))

###############################################################################
#? Display the description of the target.
###############################################################################

.PHONY: target-description
target-description:
	@$(ECHO) $(TARGET_DESCRIPTION)

###############################################################################
#? Display the compilation mode of the project.
###############################################################################

.PHONY: compilation-mode
compilation-mode:
	@$(ECHO) $(COMPILATION_MODE)

###############################################################################
#? Display the C++ standard used.
###############################################################################

.PHONY: cxx-standard
cxx-standard:
	@$(ECHO) $(subst --std=,,$(CXX_STANDARD))

###############################################################################
#? Check if your project is harden
###############################################################################

.PHONY: check-harden
check-harden: $(TARGET_NAME)
	$(Q)hardening-check ./$(OBJS_ROOT_DIR)/$(TARGET_NAME); echo -n ""
ifeq ($(COMPILATION_MODE),debug)
	@$(ECHO) " Project compiled in debug mode"
endif

##############################################################################
# Pass extra arguments to your applications. For example make run -- foo --help
##############################################################################

ifeq (run,$(firstword $(MAKECMDGOALS)))
    RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
endif

# Do not interpret arguments as makefile goals
ifdef RUN_ARGS
.PHONY: $(RUN_ARGS)
$(RUN_ARGS):;@:
endif

###############################################################################
#? Run the binary with optional arguments. For example make run -- foo --help
###############################################################################

.PHONY: run
run:
	$(Q)$(RUN) $(RUNNING_TARGET_NAME) $(RUN_ARGS) && $(ECHO_COLOR) "$(GREEN)  Error code: $$?$(COLOR_DEFAULT)" || $(ECHO_COLOR) "$(RED)  Error code: $$?$(COLOR_DEFAULT)"

###############################################################################
#? Call unit-tests with code coverage.
###############################################################################

.PHONY: check tests
check tests:
	@$(call print-simple,"Compiling unit tests",$(PROJECT_NAME))
ifneq ($(wildcard $(PROJECT_TESTS)/.),)
	@$(MAKE) -C $(PROJECT_TESTS) coverage
else
	@$(MAKE) coverage
endif

###############################################################################
#? Launch the executable with address sanitizer (if enabled).
###############################################################################

.PHONY: asan
asan: $(TARGET_BINARY_NAME)
	$(Q)$(SANITIZER) ./$(TARGET_BINARY_NAME) 2>&1 | ./$(THIRD_PARTIES_DIR)/asan_symbolize.py

###############################################################################
#? Launch the executable with gprof.
###############################################################################

.PHONY: check-compiled-with-gprof
check-compiled-with-gprof:
	@$(call print-simple,"Profiling your target",$(TARGET_NAME))
	@if [ "$(USE_GPROF)" = "" ]; then \
		$(ECHO_COLOR) "  $(RED)You have not compiled your project with gprof"; \
        $(ECHO_COLOR) "  Please add in your Makefile: USE_GPROF := 1"; \
        $(ECHO_COLOR) "  and recompile it again!$(COLOR_DEFAULT)"; \
        exit 1; \
    fi

.PHONY: gprof
gprof: check-compiled-with-gprof run
	$(Q)$(MKDIR) $(GPROF_ANALYSIS_FOLDER)
	$(Q)mv gmon.out $(GPROF_ANALYSIS_FOLDER)
	$(Q)gprof $(TARGET_BINARY_NAME) $(GPROF_ANALYSIS_FOLDER)/gmon.out > $(GPROF_ANALYSIS)
	@$(ECHO_COLOR) "$(GREEN)  You can read the report: $(GPROF_ANALYSIS)"

###############################################################################
#? Generate the code coverage html rapport.
###############################################################################

.PHONY: coverage
coverage: all
	@$(call print-to,Running,$(PROJECT_NAME),$(abspath $(TARGET_BINARY_NAME)))
	$(Q)$(TARGET_BINARY_NAME)
	@$(call print-to,Coverage report,$(abspath $(TARGET_BINARY_NAME)),$(COVERAGE_HTML_RAPPORT))
	$(Q)$(MKDIR) $(dir $(COVERAGE_HTML_RAPPORT))
	$(Q)gcovr --root $(P) --object-directory $(OBJS_ROOT_DIR) --html-details $(COVERAGE_HTML_RAPPORT) -e '/usr/*' --exclude-directories $(THIRD_PARTIES_DIR)
	$(Q)gcovr --root $(P) --object-directory $(OBJS_ROOT_DIR) --lcov $(COVERAGE_LCOV_RAPPORT) -e '/usr/*' --exclude-directories $(THIRD_PARTIES_DIR)
	$(Q)sed -i 's|$(abspath $(P))/||g' $(COVERAGE_LCOV_RAPPORT)
	$(Q)xdg-open $(COVERAGE_HTML_RAPPORT) >/dev/null

###############################################################################
#? Create a tarball for Coverity Scan a static analysis of code.
# For using Coverity Scan service, you have to download and install a gcc
# wrapper and compile your project back. A tarball should be created and you
# have to upload it to their webpage. See https://scan.coverity.com/ for more
# information.
###############################################################################

.PHONY: coverity-scan
coverity-scan: clean
	$(Q)$(RM) $(TARGET_BINARY_NAME).tgz cov-int 2> /dev/null
	$(Q)cov-build --dir cov-int $(MAKE) && tar czvf $(TARGET_BINARY_NAME).tgz cov-int

###############################################################################
#? Create an uploadable tarball for the OpenSuse Build Service.
###############################################################################

.PHONY: obs
obs:
	$(Q)$(BASH) $(SCRIPT_OBS)

###############################################################################
#? Build RPM package using auto-generated spec file.
###############################################################################

.PHONY: rpm
rpm: $(LIST_MYMAKEFILES)
	@$(call print-simple,RPM package,$(PROJECT_NAME))
	$(Q)$(RM) $(RPM_TOPDIR_PATH)
	$(Q)$(MKDIR) $(RPM_TOPDIR_PATH)/{BUILD,RPMS,SOURCES,SPECS,SRPMS,BUILDROOT}
	$(Q)$(call CREATE_RPM_SPEC,$(RPM_SPEC_FILE))
	$(Q)rpmbuild \
		--dbpath $(RPM_TOPDIR_PATH)/fake-rpmdb \
		--define "_topdir $(RPM_TOPDIR_PATH)" \
		--define "_builddir $(RPM_TOPDIR_PATH)/BUILD" \
		--define "_buildrootdir $(RPM_TOPDIR_PATH)/BUILDROOT" \
		--define "_prefix /usr" \
		-bb $(RPM_SPEC_FILE) \
		--nodeps
	@$(ECHO_COLOR) "$(GREEN)✔ RPM generated in $(RPM_TOPDIR_PATH)/RPMS/$(COLOR_DEFAULT)"
	@find $(RPM_TOPDIR_PATH)/RPMS -name "*.rpm" -type f

###############################################################################
# Git clone third-party libraries helping for the debug.
###############################################################################

ifdef USE_BACKWARD
$(DOWNLOAD_DIR)/backward-cpp/backward.cpp: creating-build-folder
	@$(call GIT_CLONE,$(DOWNLOAD_DIR),"bombela/backward-cpp")
endif

ifdef USE_DEBUG_MACRO
$(DOWNLOAD_DIR)/dbg-macro/dbg.h: creating-build-folder
	@$(call GIT_CLONE,$(DOWNLOAD_DIR),"sharkdp/dbg-macro")
endif

###############################################################################
#? Rebuild the project.
###############################################################################

.PHONY: rebuild
rebuild: veryclean all

###############################################################################
#? List all available targets.
###############################################################################

.PHONY: list-targets
list-targets:
	@$(ECHO_COLOR) "$(CYAN)Available targets:$(COLOR_DEFAULT)"
	@$(MAKE) --print-data-base --no-builtin-variables --no-builtin-rules --question 2>/dev/null | \
	awk '/^[^.%][-A-Za-z0-9_]*:/ { print substr($$1, 1, length($$1)-1) }' | \
	sort | uniq | grep -v '^Makefile$$' | \
	sed 's/^/  - /'

###############################################################################
#? List all key project variables.
###############################################################################

.PHONY: list-variables
list-variables:
	@$(ECHO_COLOR) "$(CYAN)Key project variables:$(COLOR_DEFAULT)"
	@$(ECHO_COLOR) "  $(PURPLE)PROJECT_NAME$(COLOR_DEFAULT) = $(PROJECT_NAME)"
	@$(ECHO_COLOR) "  $(PURPLE)TARGET_NAME$(COLOR_DEFAULT) = $(TARGET_NAME)"
	@$(ECHO_COLOR) "  $(PURPLE)PROJECT_VERSION$(COLOR_DEFAULT) = $(PROJECT_VERSION)"
	@$(ECHO_COLOR) "  $(PURPLE)COMPILATION_MODE$(COLOR_DEFAULT) = $(COMPILATION_MODE)"
	@$(ECHO_COLOR) "  $(PURPLE)COMPILER$(COLOR_DEFAULT) = $(COMPILER)"
	@$(ECHO_COLOR) "  $(PURPLE)OS$(COLOR_DEFAULT) = $(OS)"
	@$(ECHO_COLOR) "  $(PURPLE)ARCH$(COLOR_DEFAULT) = $(ARCH)"
	@$(ECHO_COLOR) "  $(PURPLE)BUILD_PATH$(COLOR_DEFAULT) = $(BUILD_PATH)"
	@$(ECHO_COLOR) "  $(PURPLE)NPROCS$(COLOR_DEFAULT) = $(NPROCS)"

###############################################################################
#? Show compilation flags.
###############################################################################

.PHONY: show-flags
show-flags:
	@$(ECHO_COLOR) "$(CYAN)Compilation flags:$(COLOR_DEFAULT)"
	@$(ECHO_COLOR) "  $(PURPLE)CXXFLAGS$(COLOR_DEFAULT) = $(CXXFLAGS)"
	@$(ECHO_COLOR) "  $(PURPLE)CCFLAGS$(COLOR_DEFAULT) = $(CCFLAGS)"
	@$(ECHO_COLOR) "  $(PURPLE)LDFLAGS$(COLOR_DEFAULT) = $(LDFLAGS)"
	@$(ECHO_COLOR) "  $(PURPLE)OPTIM_FLAGS$(COLOR_DEFAULT) = $(OPTIM_FLAGS)"
	@$(ECHO_COLOR) "  $(PURPLE)DEFINES$(COLOR_DEFAULT) = $(DEFINES)"

###############################################################################
#? Show project paths.
###############################################################################

.PHONY: show-paths
show-paths:
	@$(ECHO_COLOR) "$(CYAN)Project paths:$(COLOR_DEFAULT)"
	@$(ECHO_COLOR) "  $(PURPLE)Project root$(COLOR_DEFAULT) = $(P)"
	@$(ECHO_COLOR) "  $(PURPLE)MyMakefile root$(COLOR_DEFAULT) = $(M)"
	@$(ECHO_COLOR) "  $(PURPLE)Build directory$(COLOR_DEFAULT) = $(BUILD_PATH)"
	@$(ECHO_COLOR) "  $(PURPLE)Objects directory$(COLOR_DEFAULT) = $(OBJS_ROOT_DIR)"
	@$(ECHO_COLOR) "  $(PURPLE)Third-party directory$(COLOR_DEFAULT) = $(THIRD_PARTIES_DIR)"
	@$(ECHO_COLOR) "  $(PURPLE)Download directory$(COLOR_DEFAULT) = $(DOWNLOAD_DIR)"

###############################################################################
#? Check dependencies.
###############################################################################

.PHONY: check-deps
check-deps:
	@$(ECHO_COLOR) "$(CYAN)Checking dependencies:$(COLOR_DEFAULT)"
	@$(ECHO_COLOR) "  $(PURPLE)Compiler$(COLOR_DEFAULT): $(COMPILER)"
	@$(COMPILER) --version | head -1 || $(ECHO_COLOR) "$(RED)  Compiler not found!$(COLOR_DEFAULT)"
	@$(ECHO_COLOR) "  $(PURPLE)Make$(COLOR_DEFAULT): $(MAKE)"
	@$(MAKE) --version | head -1 || $(ECHO_COLOR) "$(RED)  Make not found!$(COLOR_DEFAULT)"
ifdef PKG_LIBS
	@$(ECHO_COLOR) "  $(PURPLE)pkg-config$(COLOR_DEFAULT):"
	@for lib in $(PKG_LIBS); do \
		if pkg-config --exists $$lib; then \
			$(ECHO_COLOR) "    $(GREEN)✓$(COLOR_DEFAULT) $$lib"; \
		else \
			$(ECHO_COLOR) "    $(RED)✗$(COLOR_DEFAULT) $$lib"; \
		fi; \
	done
endif

###############################################################################
#? Show size analysis.
###############################################################################

.PHONY: size-analysis
size-analysis: all
ifdef TARGET_BINARY_NAME
	@$(ECHO_COLOR) "$(CYAN)Binary size analysis:$(COLOR_DEFAULT)"
	@ls -lh $(TARGET_BINARY_NAME) | awk '{print "  Total size: " $$5}'
	@if command -v size >/dev/null 2>&1; then \
		$(ECHO_COLOR) "  $(PURPLE)Sections:$(COLOR_DEFAULT)"; \
		size $(TARGET_BINARY_NAME) | tail -n +2 | awk '{print "    text: " $$1 ", data: " $$2 ", bss: " $$3}'; \
	fi
endif
ifdef TARGET_STATIC_LIB_NAME
	@$(ECHO_COLOR) "$(CYAN)Static library size:$(COLOR_DEFAULT)"
	@ls -lh $(TARGET_STATIC_LIB_NAME) | awk '{print "  Size: " $$5}'
endif
ifdef TARGET_SHARED_LIB_NAME
	@$(ECHO_COLOR) "$(CYAN)Shared library size:$(COLOR_DEFAULT)"
	@ls -lh $(TARGET_SHARED_LIB_NAME) | awk '{print "  Size: " $$5}'
endif

###############################################################################
#? Show build statistics.
###############################################################################

.PHONY: build-stats
build-stats:
	@$(ECHO_COLOR) "$(CYAN)Build statistics for $(TARGET_NAME):$(COLOR_DEFAULT)"
	@$(ECHO_COLOR) "  $(PURPLE)Source files$(COLOR_DEFAULT): $(words $(SRC_FILES)): $(strip $(SRC_FILES))"
	@$(ECHO_COLOR) "  $(PURPLE)Library files$(COLOR_DEFAULT): $(words $(LIB_FILES)): $(strip $(LIB_FILES))"
	@$(ECHO_COLOR) "  $(PURPLE)Object files$(COLOR_DEFAULT): $(words $(OBJS) $(LIB_OBJS)): $(strip $(OBJS) $(LIB_OBJS))"
	@$(ECHO_COLOR) "  $(PURPLE)Include directories$(COLOR_DEFAULT): $(words $(INCLUDES)): $(strip $(INCLUDES))"
	@$(ECHO_COLOR) "  $(PURPLE)Internal libraries$(COLOR_DEFAULT): $(words $(INTERNAL_LIBS)): $(strip $(INTERNAL_LIBS))"
ifdef PKG_LIBS
	@$(ECHO_COLOR) "  $(PURPLE)External libraries$(COLOR_DEFAULT): $(words $(PKG_LIBS)): $(strip $(PKG_LIBS))"
endif
	$(Q)for d in $(DIRS_WITH_MAKEFILE); do \
	$(MAKE) --no-print-directory $(addprefix --directory=,$$d) build-stats; \
	done

###############################################################################
#? Format source code.
###############################################################################

.PHONY: format-source-code
format-source-code: ensure-clang-format
	@$(call print-simple,"Formatting source code",$(PROJECT_NAME))
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(P) -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | \
		grep -v $(BUILD_PATH) | grep -v $(THIRD_PARTIES_DIR) | \
		xargs clang-format -i; \
		$(ECHO_COLOR) "$(GREEN)Code formatted successfully$(COLOR_DEFAULT)"; \
	else \
		$(ECHO_COLOR) "$(RED)clang-format not found$(COLOR_DEFAULT)"; \
	fi

###############################################################################
#? Run static analysis.
###############################################################################

.PHONY: lint
lint:
	@$(call print-simple,"Running static analysis",$(PROJECT_NAME))
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --inconclusive --std=c++14 \
		--suppress=missingIncludeSystem \
		--suppress=unusedFunction \
		$(P)/src $(P)/include 2>&1 | \
		grep -v "Checking " || true; \
	else \
		$(ECHO_COLOR) "$(RED)cppcheck not found$(COLOR_DEFAULT)"; \
	fi

###############################################################################
#? Run benchmark.
###############################################################################

.PHONY: benchmark
benchmark: all
ifdef TARGET_BINARY_NAME
	@$(call print-simple,"Running benchmark",$(PROJECT_NAME))
	@if command -v time >/dev/null 2>&1; then \
		time $(TARGET_BINARY_NAME) || true; \
	else \
		$(TARGET_BINARY_NAME) || true; \
	fi
endif

###############################################################################
#? Show this help.
###############################################################################

.PHONY: help
help:
	@$(ECHO) 'Usage:'
	@$(ECHO) '  [VERBOSE=1] make [flags...] <target>'
	@$(ECHO) ''
	@$(ECHO) 'You can override the following flags:'
	@$(ECHO) ''
	@awk '/^#\? /{ comment = substr($$0,4) } comment && \
    /^ *[a-zA-Z][a-zA-Z0-9_-]+ ?\?=/ \
    { \
       printf "  \033[36m%s|\033[0m%s\n    \033[00mDefault value: \033[32m%s", $$1, comment, $$3; \
       for(i=4;i<=NF;i++){printf " %s", $$i} printf "\n" \
    }' $(MAKEFILE_LIST) | column -t -s '|'
	@$(ECHO_COLOR) -e '\033[00m'
	@$(ECHO) 'Targets:'
	@$(ECHO) ''
	@awk '/^#\? /{ comment = substr($$0,4) } comment && \
    /^[a-zA-Z][a-zA-Z0-9_-]+:/{ printf "  \033[36m%s|\033[0m%s\n", $$1, comment }' \
    $(MAKEFILE_LIST) | sort | column -t -s '|'
	@$(ECHO_COLOR) -e '\033[00m'
