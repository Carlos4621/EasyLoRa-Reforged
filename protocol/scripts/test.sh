#!/usr/bin/env bash
set -euo pipefail

PRESET=${1:-clang-debug}
ctest --preset "$PRESET"
