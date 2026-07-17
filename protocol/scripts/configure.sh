#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PRESET=${1:-clang-debug}

if [[ ! -x "$PROJECT_ROOT/.tools/vcpkg/vcpkg" && ! -f "$PROJECT_ROOT/.tools/vcpkg/vcpkg.exe" ]]; then
    bash "$PROJECT_ROOT/scripts/bootstrap_vcpkg.sh"
fi

cmake --preset "$PRESET" --fresh
