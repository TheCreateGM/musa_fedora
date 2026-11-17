#!/usr/bin/env bash
# Select an installed MUSA prefix or an isolated staged official SDK tree.
# This file is meant to be sourced; it does not install packages, edit loader
# configuration, load modules, or query the GPU.
#
# Installed-prefix use (only after an SDK is deliberately installed):
#   source scripts/musa-env.sh
#
# Compile-only use from the official packages staged under musa-data:
#   MUSA_USE_STAGING=1 source scripts/musa-env.sh

_MUSA_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_MUSA_PROJECT_DIR="$(dirname "$_MUSA_SCRIPT_DIR")"
_MUSA_DATA_DIR="$(dirname "$_MUSA_PROJECT_DIR")/musa-data"

if [[ "${MUSA_USE_STAGING:-0}" == "1" ]]; then
    _MUSA_EXTRACTED="$_MUSA_DATA_DIR/research/official-musa-5.2.0/packages/extracted"
    MUSA_HOME="$_MUSA_EXTRACTED/mtcc-5-2_5.2.0_amd64/usr/local/musa-5.2"
    _MUSA_MUSART_HOME="$_MUSA_EXTRACTED/musa-musart-5-2_5.2.0_amd64/usr/local/musa-5.2"
    _MUSA_MUSART_DEV_HOME="$_MUSA_EXTRACTED/musa-musart-dev-5-2_5.2.0_amd64/usr/local/musa-5.2"
    _MUSA_MTXAS_HOME="$_MUSA_EXTRACTED/mtxas-5-2_5.2.0_amd64/usr/local/musa-5.2"
    _MUSA_COMPUTE_LIB="$_MUSA_EXTRACTED/libmthreads-compute_5.2.0_amd64/usr/lib/x86_64-linux-gnu"
    _MUSA_DRM_LIB="$_MUSA_EXTRACTED/libdrm-mtgpu_5.2.0_amd64/usr/lib/x86_64-linux-gnu"
    _MUSA_LINK_LIB="$_MUSA_DATA_DIR/research/official-musa-5.2.0/link-libs"

    _MUSA_REQUIRED=(
        "$MUSA_HOME/bin/mcc"
        "$_MUSA_MUSART_HOME/lib/libmusart.so.5.2.0"
        "$_MUSA_MUSART_DEV_HOME/include/musa_runtime.h"
        "$_MUSA_MTXAS_HOME/bin/mtxas"
        "$_MUSA_COMPUTE_LIB/libmusa.so.5.2.0"
        "$_MUSA_DRM_LIB/libdrm_mtgpu.so.1.0.0"
        "$_MUSA_LINK_LIB/libmusart.so"
    )
    for _MUSA_REQUIRED_FILE in "${_MUSA_REQUIRED[@]}"; do
        if [[ ! -e "$_MUSA_REQUIRED_FILE" ]]; then
            printf 'MUSA staging file is missing: %s\n' "$_MUSA_REQUIRED_FILE" >&2
            return 1
        fi
    done

    MUSA_INCLUDE_PATH="$_MUSA_MUSART_DEV_HOME/include"
    MUSA_COMPILER_INCLUDE="$MUSA_HOME/include"
    MUSA_RT_LIB_PATH="$_MUSA_LINK_LIB"
    PATH="$MUSA_HOME/bin:$_MUSA_MUSART_HOME/bin:$_MUSA_MTXAS_HOME/bin:$_MUSA_EXTRACTED/libmthreads-compute_5.2.0_amd64/usr/bin:$PATH"

    # The stub libmusa.so under lib/stubs is link-only and intentionally
    # excluded. Use the real runtime and driver libraries for runtime lookup.
    _MUSA_NEW_LIB_PATH="$_MUSA_MUSART_HOME/lib:$MUSA_HOME/lib:$_MUSA_COMPUTE_LIB:$_MUSA_DRM_LIB"
else
    MUSA_HOME="${MUSA_HOME:-${MUSA_PATH:-/usr/local/musa-5.2}}"
    MUSA_INCLUDE_PATH="${MUSA_INCLUDE_PATH:-$MUSA_HOME/include}"
    MUSA_COMPILER_INCLUDE="${MUSA_COMPILER_INCLUDE:-$MUSA_HOME/include}"
    MUSA_RT_LIB_PATH="${MUSA_RT_LIB_PATH:-$MUSA_HOME/lib}"
    PATH="$MUSA_HOME/bin:$PATH"
    _MUSA_NEW_LIB_PATH="$MUSA_HOME/lib"
fi

MUSA_PATH="$MUSA_HOME"
MUSA_MCC="${MUSA_MCC:-$MUSA_HOME/bin/mcc}"
if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
    LD_LIBRARY_PATH="$_MUSA_NEW_LIB_PATH:$LD_LIBRARY_PATH"
else
    LD_LIBRARY_PATH="$_MUSA_NEW_LIB_PATH"
fi

export MUSA_HOME MUSA_PATH MUSA_INCLUDE_PATH MUSA_COMPILER_INCLUDE
export MUSA_RT_LIB_PATH MUSA_MCC PATH LD_LIBRARY_PATH

unset _MUSA_SCRIPT_DIR _MUSA_PROJECT_DIR _MUSA_DATA_DIR _MUSA_EXTRACTED
unset _MUSA_MUSART_HOME _MUSA_MUSART_DEV_HOME _MUSA_MTXAS_HOME
unset _MUSA_COMPUTE_LIB _MUSA_DRM_LIB _MUSA_LINK_LIB _MUSA_NEW_LIB_PATH
unset _MUSA_REQUIRED _MUSA_REQUIRED_FILE
