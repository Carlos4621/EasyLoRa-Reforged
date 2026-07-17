#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PRESET=${1:-clang-debug}

cmake --build --preset "$PRESET" --parallel
