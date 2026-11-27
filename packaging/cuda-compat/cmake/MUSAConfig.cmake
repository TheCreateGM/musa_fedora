# MUSAConfig.cmake — package-mode entry point for the MUSA toolchain
#
# Part of musa-cuda-compat (https://github.com/TheCreateGM/musa_fedora).
# SPDX-License-Identifier: MIT
#
# Installed under %{_libdir}/cmake/musa/, so a plain
#
#   find_package(MUSA)            # or find_package(MUSA REQUIRED)
#
# finds the validated Fedora MUSA stack without any CMAKE_MODULE_PATH edits.
# Ports that do the vendor idiom first (`list(APPEND CMAKE_MODULE_PATH
# /usr/local/musa/cmake); find_package(MUSA)`) also work: the module-mode
# search never runs here because CONFIG mode resolves first, and both modes
# execute the same FindMUSA implementation installed next to this file.

get_filename_component(_musa_config_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
include("${_musa_config_dir}/FindMUSA.cmake")

unset(_musa_config_dir)
