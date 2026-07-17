#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
VCPKG_DIR="$PROJECT_ROOT/.tools/vcpkg"
VCPKG_REPOSITORY=${VCPKG_REPOSITORY:-https://github.com/microsoft/vcpkg.git}
VCPKG_COMMIT=${VCPKG_COMMIT:-}

if ! command -v git >/dev/null 2>&1; then
    echo "Error: git no está instalado." >&2
    exit 1
fi

if [[ ! -d "$VCPKG_DIR/.git" ]]; then
    mkdir -p "$(dirname -- "$VCPKG_DIR")"
    git clone --depth 1 "$VCPKG_REPOSITORY" "$VCPKG_DIR"
fi

if [[ -n "$VCPKG_COMMIT" ]]; then
    git -C "$VCPKG_DIR" fetch --depth 1 origin "$VCPKG_COMMIT"
    git -C "$VCPKG_DIR" checkout --detach "$VCPKG_COMMIT"
fi

if [[ "${OS:-}" == "Windows_NT" ]]; then
    "$VCPKG_DIR/bootstrap-vcpkg.bat" -disableMetrics
else
    "$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics
fi

echo "vcpkg preparado en: $VCPKG_DIR"
echo "Commit: $(git -C "$VCPKG_DIR" rev-parse HEAD)"
