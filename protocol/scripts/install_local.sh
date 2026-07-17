#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PRESET=${1:-release}
PREFIX=${2:-$PROJECT_ROOT/out/install/$PRESET}

cmake --build --preset "$PRESET" --parallel
cmake --install "$PROJECT_ROOT/build/$PRESET" --prefix "$PREFIX"

echo "Biblioteca instalada en: $PREFIX"
