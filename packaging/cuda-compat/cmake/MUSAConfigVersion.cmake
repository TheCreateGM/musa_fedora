# MUSAConfigVersion.cmake — version gate for find_package(MUSA <ver>)
#
# Part of musa-cuda-compat (https://github.com/TheCreateGM/musa_fedora).
# SPDX-License-Identifier: MIT
#
# PACKAGE_VERSION is the MUSA SDK version (e.g. 5.2.0), probed from the
# installed SDK header because this file runs before MUSAConfig.cmake.
# SameMajorVersion semantics apply, with one practical exception: ports that
# pin the *runtime* version they were tested against (any request below 2.0,
# e.g. MUSA 1.5.4) are considered compatible, because this package
# deliberately pairs the 5.2.x compiler with the validated 1.5.4 runtime.

set(PACKAGE_VERSION "")
foreach(_root IN ITEMS "$ENV{MUSA_HOME}" /usr/local/musa-5.2 /usr/local/musa)
    if(_root AND EXISTS "${_root}/include/musa.h")
        file(STRINGS "${_root}/include/musa.h" _line REGEX
             "^[ \t]*#[ \t]*define[ \t]+MUSA_VERSION[ \t]+[0-9]+" LIMIT_COUNT 1)
        if(_line MATCHES "MUSA_VERSION[ \t]+([0-9]+)")
            math(EXPR _maj "${CMAKE_MATCH_1} / 10000")
            math(EXPR _min "${CMAKE_MATCH_1} / 100 % 100")
            math(EXPR _pat "${CMAKE_MATCH_1} % 100")
            set(PACKAGE_VERSION "${_maj}.${_min}.${_pat}")
        endif()
        break()
    endif()
endforeach()

if(NOT PACKAGE_VERSION)
    # No SDK header found: let MUSAConfig/FindMUSA produce the real error.
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
elseif(NOT PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
elseif(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
elseif(PACKAGE_FIND_VERSION VERSION_LESS 2.0)
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_VERSION STREQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
elseif(PACKAGE_VERSION_MAJOR EQUAL PACKAGE_FIND_VERSION_MAJOR)
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_VERSION STREQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
else()
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
endif()
