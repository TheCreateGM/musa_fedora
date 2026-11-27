# FindMUSA.cmake — locate the MUSA toolchain on Fedora (MTT S30 validated stack)
#
# Part of musa-cuda-compat (https://github.com/TheCreateGM/musa_fedora).
# SPDX-License-Identifier: MIT
#
# This module targets the layout shipped by the musa-fedora RPM set:
#
#   * compiler + headers : /usr/local/musa-5.2   (musa-sdk, MUSA 5.2.0, mcc)
#   * runtime libraries  : /usr/local/musa/lib   (musa-runtime, libmusart 1.5.4)
#
# The split is deliberate: the 5.2.0 runtime is ABI-incompatible with the
# mtgpu 3.1.0 kernel driver on the S30 (muInit fails with -22), so this module
# always links the 1.5.4 runtime and never the SDK's own copy.
#
# Search order for the two roots (first hit wins):
#   SDK root    : MUSA_TOOLKIT_ROOT_DIR (cache) -> MUSA_HOME env ->
#                 /usr/local/musa-5.2 -> /usr/local/musa-* -> PATH mcc
#   Runtime root: MUSA_RUNTIME_ROOT_DIR (cache) -> MUSA_RUNTIME_HOME env ->
#                 /usr/local/musa
#
# Defines (module mode; the package-mode MUSAConfig sets the same names):
#   MUSA_FOUND, MUSA_VERSION, MUSA_VERSION_MAJOR
#   MUSA_TOOLKIT_ROOT_DIR, MUSA_RUNTIME_ROOT_DIR
#   MUSA_INCLUDE_DIRS, MUSA_LIBRARY_DIRS
#   MUSA_LIBRARIES            full paths: runtime + driver API libraries
#   MUSA_RUNTIME_LIBRARY, MUSA_DRIVER_LIBRARY
#   MUSA_MCC_EXECUTABLE, MUSA_ARCH            (mp_10 for the S30)
#   MUSA_CUDA_COMPAT_ROOT, MUSA_NVCC_EXECUTABLE   optional, cuda-compat layer
#
# Imported targets:
#   MUSA::musart, MUSA::musa       shared runtime/driver libraries
#   MUSA::toolkit                  interface: SDK include dirs
#
# Commands:
#   musa_add_executable(<name> sources... [OPTIONS extra-mcc-flags...])
#   musa_add_library(<name> sources... [OPTIONS ...])
#     Compile MUSA sources (.mu, or CUDA-named .cu after musify) with mcc for
#     the real device and link against the validated 1.5.4 runtime. Mirrors
#     the vendor `musa_add_executable` used by Moore Threads ports, so a
#     CMakeLists written for the vendor MUSA CMake module configures
#     unchanged. Every -arch spelling collapses to mp_10 on this GPU.
#
# Usage (CMake >= 3.14, no CMAKE_MODULE_PATH edits needed):
#   find_package(MUSA REQUIRED)
#   musa_add_executable(app src/kernel.mu src/main.cpp)
#
# The variable names follow the MUSA convention used by Moore Threads ports
# (eigen-musa, palabos-musa); this implementation is written for the Fedora
# RPM layout and the S30 validated toolchain split.

# ---- roots -----------------------------------------------------------------
get_filename_component(_musa_self_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

if(NOT MUSA_TOOLKIT_ROOT_DIR)
    if(DEFINED ENV{MUSA_HOME} AND IS_DIRECTORY "$ENV{MUSA_HOME}/include")
        set(MUSA_TOOLKIT_ROOT_DIR "$ENV{MUSA_HOME}")
    else()
        set(_musa_sdk_candidates
            /usr/local/musa-5.2
            $ENV{HOME}/.local/musa
            /opt/musa
        )
        file(GLOB _musa_extra_local "/usr/local/musa-*")
        list(APPEND _musa_sdk_candidates ${_musa_extra_local})
        list(REMOVE_DUPLICATES _musa_sdk_candidates)
        foreach(_c IN LISTS _musa_sdk_candidates)
            if(IS_DIRECTORY "${_c}/include" AND EXISTS "${_c}/bin/mcc")
                set(MUSA_TOOLKIT_ROOT_DIR "${_c}")
                break()
            endif()
        endforeach()
    endif()
endif()

if(NOT MUSA_RUNTIME_ROOT_DIR)
    if(DEFINED ENV{MUSA_RUNTIME_HOME} AND IS_DIRECTORY "$ENV{MUSA_RUNTIME_HOME}/lib")
        set(MUSA_RUNTIME_ROOT_DIR "$ENV{MUSA_RUNTIME_HOME}")
    else()
        foreach(_c /usr/local/musa /opt/musa-runtime)
            if(EXISTS "${_c}/lib/libmusart.so.1.5.4" OR EXISTS "${_c}/lib/libmusart.so")
                set(MUSA_RUNTIME_ROOT_DIR "${_c}")
                break()
            endif()
        endforeach()
    endif()
endif()

# ---- pieces ----------------------------------------------------------------
find_program(MUSA_MCC_EXECUTABLE NAMES mcc
    HINTS "${MUSA_TOOLKIT_ROOT_DIR}/bin"
    PATHS /usr/local/musa-5.2/bin /usr/local/bin
    DOC "MUSA compiler driver (mcc)")
mark_as_advanced(MUSA_MCC_EXECUTABLE)

find_library(MUSA_RUNTIME_LIBRARY NAMES musart
    HINTS "${MUSA_RUNTIME_ROOT_DIR}/lib"
    PATHS /usr/local/musa/lib
    DOC "MUSA runtime library (libmusart)")
mark_as_advanced(MUSA_RUNTIME_LIBRARY)

find_library(MUSA_DRIVER_LIBRARY NAMES musa
    HINTS "${MUSA_RUNTIME_ROOT_DIR}/lib"
    PATHS /usr/local/musa/lib
    DOC "MUSA driver API library (libmusa)")
mark_as_advanced(MUSA_DRIVER_LIBRARY)

# Version reported by the SDK headers (MUSA_VERSION, e.g. 50200).
set(MUSA_VERSION "")
if(MUSA_TOOLKIT_ROOT_DIR AND EXISTS "${MUSA_TOOLKIT_ROOT_DIR}/include/musa.h")
    file(STRINGS "${MUSA_TOOLKIT_ROOT_DIR}/include/musa.h" _musa_version_line
         REGEX "^#[ \t]*define[ \t]+MUSA_VERSION[ \t]+[0-9]+" LIMIT_COUNT 1)
    if(_musa_version_line MATCHES "MUSA_VERSION[ \t]+([0-9]+)")
        math(EXPR _maj "${CMAKE_MATCH_1} / 10000")
        math(EXPR _min "${CMAKE_MATCH_1} / 100 % 100")
        math(EXPR _pat "${CMAKE_MATCH_1} % 100")
        set(MUSA_VERSION "${_maj}.${_min}.${_pat}")
    endif()
endif()

# The validated device target for the MTT S30 is mp_10. Keep the plain-number
# spelling (MUSA_ARCH=10) for ports that append it as "mp_${MUSA_ARCH}".
if(NOT MUSA_ARCH)
    set(MUSA_ARCH "mp_10" CACHE STRING "MUSA offload architecture (S30: mp_10)")
endif()
if(MUSA_ARCH MATCHES "^[0-9]+$")
    set(_musa_arch "mp_${MUSA_ARCH}")
else()
    set(_musa_arch "${MUSA_ARCH}")
endif()

# Optional CUDA source-compatibility layer (musa-cuda-compat package).
find_program(MUSA_NVCC_EXECUTABLE NAMES nvcc
    HINTS "${MUSA_CUDA_COMPAT_ROOT}/bin" /usr/lib64/musa-cuda-compat/bin
    NO_DEFAULT_PATH
    DOC "musa-cuda-compat nvcc wrapper (mcc driver accepting CUDA flags)")
find_path(MUSA_CUDA_COMPAT_ROOT NAMES include/cuda_runtime.h
    PATHS /usr/lib64/musa-cuda-compat /usr/local/cuda
    DOC "CUDA-compatibility root")
mark_as_advanced(MUSA_NVCC_EXECUTABLE MUSA_CUDA_COMPAT_ROOT)

# ---- result -----------------------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MUSA
    REQUIRED_VARS MUSA_MCC_EXECUTABLE MUSA_RUNTIME_LIBRARY MUSA_DRIVER_LIBRARY
                  MUSA_TOOLKIT_ROOT_DIR MUSA_RUNTIME_ROOT_DIR
    VERSION_VAR MUSA_VERSION)

if(MUSA_FOUND)
    set(MUSA_INCLUDE_DIRS "${MUSA_TOOLKIT_ROOT_DIR}/include")
    set(MUSA_LIBRARY_DIRS "${MUSA_RUNTIME_ROOT_DIR}/lib")
    set(MUSA_LIBRARIES "${MUSA_RUNTIME_LIBRARY};${MUSA_DRIVER_LIBRARY}")
    set(MUSA_DEVICE_ARCH "${_musa_arch}")

    if(NOT TARGET MUSA::musart)
        add_library(MUSA::musart SHARED IMPORTED)
        set_target_properties(MUSA::musart PROPERTIES
            IMPORTED_LOCATION "${MUSA_RUNTIME_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MUSA_INCLUDE_DIRS}")
    endif()
    if(NOT TARGET MUSA::musa)
        add_library(MUSA::musa SHARED IMPORTED)
        set_target_properties(MUSA::musa PROPERTIES
            IMPORTED_LOCATION "${MUSA_DRIVER_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MUSA_INCLUDE_DIRS}")
    endif()
    if(NOT TARGET MUSA::toolkit)
        add_library(MUSA::toolkit INTERFACE IMPORTED)
        set_target_properties(MUSA::toolkit PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${MUSA_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${MUSA_LIBRARIES}")
    endif()

    # ---- device-build helpers ---------------------------------------------
    # Exact invocation validated on S30 hardware by scripts/verify-musa.sh:
    #   mcc -x musa --musa-path=<sdk> --musa-rt-lib-path=<rt>/lib
    #       --offload-arch=mp_10 -I<sdk>/include <srcs> -L<rt>/lib
    #       -Wl,-rpath,<rt>/lib -lmusart -lmusa
    function(_musa_add_device_target target kind)
        cmake_parse_arguments(MUSA_ARG "" "" "OPTIONS" ${ARGN})
        if(NOT MUSA_ARG_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR
                "musa_add_${kind}(${target}): no sources given")
        endif()
        set(_srcs ${MUSA_ARG_UNPARSED_ARGUMENTS})
        set(_abs_srcs "")
        foreach(_s IN LISTS _srcs)
            if(IS_ABSOLUTE "${_s}")
                list(APPEND _abs_srcs "${_s}")
            else()
                list(APPEND _abs_srcs "${CMAKE_CURRENT_SOURCE_DIR}/${_s}")
            endif()
        endforeach()
        set(_srcs ${_abs_srcs})
        # Output lives in a subdirectory: a generated file named exactly like
        # a make target is circular in the Makefile generator. Ports can run
        # it via the command line shown at the end of configure.
        set(_dir "${CMAKE_CURRENT_BINARY_DIR}/musa")
        set(_lrt "-L${MUSA_RUNTIME_ROOT_DIR}/lib")
        set(_rpt "-Wl,-rpath,${MUSA_RUNTIME_ROOT_DIR}/lib")
        # Undo the SDK header renames whose versioned target the 1.5.4
        # runtime does not export (musaGetDeviceProperties, muGetProcAddress,
        # the muGraph*KernelNode family, ...), so MUSA-named sources call the
        # entry points that actually exist. Generated by gen-cuda-compat.py.
        set(_rt154_flags "")
        if(MUSA_CUDA_COMPAT_ROOT AND
           EXISTS "${MUSA_CUDA_COMPAT_ROOT}/include/musa_rt154_names.h")
            list(APPEND _rt154_flags
                 -include "${MUSA_CUDA_COMPAT_ROOT}/include/musa_rt154_names.h")
        endif()
        if("${kind}" STREQUAL "EXECUTABLE")
            set(_out "${_dir}/${target}")
            set(_emit -o "${_out}")
        else()
            set(_out "${_dir}/lib${target}.so")
            set(_emit -shared -o "${_out}")
        endif()
        file(MAKE_DIRECTORY "${_dir}")
        add_custom_command(
            OUTPUT "${_out}"
            COMMAND "${MUSA_MCC_EXECUTABLE}" -x musa
                "--musa-path=${MUSA_TOOLKIT_ROOT_DIR}"
                "--musa-rt-lib-path=${MUSA_RUNTIME_ROOT_DIR}/lib"
                "--offload-arch=${MUSA_DEVICE_ARCH}"
                "-I${MUSA_TOOLKIT_ROOT_DIR}/include"
                ${_rt154_flags}
                ${MUSA_ARG_OPTIONS}
                ${_srcs}
                ${_emit}
                ${_lrt}
                ${_rpt}
                -lmusart -lmusa
            DEPENDS ${_srcs}
            COMMENT "mcc (${MUSA_DEVICE_ARCH}): ${kind} ${target} -> ${_out}"
            VERBATIM)
        add_custom_target(${target} ALL DEPENDS "${_out}")
    endfunction()

    function(musa_add_executable target)
        _musa_add_device_target(${target} EXECUTABLE ${ARGN})
    endfunction()

    function(musa_add_library target)
        _musa_add_device_target(${target} LIBRARY ${ARGN})
    endfunction()
endif()
