#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${ROOT_DIR}/build-host"

SANITIZERS="${FIRMWARE_TEST_SANITIZERS-address,undefined,leak}"
BUILD_TYPE="${BUILD_TYPE-Debug}"
CMAKE_BIN="${CMAKE_BIN-/usr/bin/cmake}"

if [[ ! -x "${CMAKE_BIN}" ]]; then
  CMAKE_BIN="$(command -v cmake)"
fi

if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  if grep -q "CMAKE_SYSTEM_NAME:STRING=PICO" "${BUILD_DIR}/CMakeCache.txt" || \
     grep -q "CMAKE_TOOLCHAIN_FILE:FILEPATH=.*pico" "${BUILD_DIR}/CMakeCache.txt"; then
    rm -rf "${BUILD_DIR}"
  fi
fi

"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DFIRMWARE_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DFIRMWARE_TEST_SANITIZERS="${SANITIZERS}" \
  -DCMAKE_TOOLCHAIN_FILE=

"${CMAKE_BIN}" --build "${BUILD_DIR}" -j
ctest --test-dir "${BUILD_DIR}" --output-on-failure
