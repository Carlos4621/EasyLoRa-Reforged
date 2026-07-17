#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
MODE=${1:---apply}

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format no fue encontrado." >&2
    exit 1
fi

mapfile -d '' FILES < <(
    find "$PROJECT_ROOT/include" "$PROJECT_ROOT/src" "$PROJECT_ROOT/tests" \
        -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0
)

if [[ "$MODE" == "--check" ]]; then
    clang-format --dry-run --Werror "${FILES[@]}"
elif [[ "$MODE" == "--apply" ]]; then
    clang-format -i "${FILES[@]}"
else
    echo "Uso: bash scripts/format.sh [--apply|--check]" >&2
    exit 2
fi
