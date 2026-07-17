#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)

bash "$PROJECT_ROOT/scripts/configure.sh" clang-tidy
cmake --build --preset clang-tidy --parallel
