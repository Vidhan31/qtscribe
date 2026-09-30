#!/usr/bin/env bash
#
# Wayland-only packaging guard.
# Mirrors the X11/XCB absence assertion from packaging/deb/Dockerfile.deb at
# source level so regressions fail in ctest/CI before packaging runs.
#
# Usage: check-wayland-only.sh [DEPLOY_DIR...]
#   With no args, only the CMake deploy configuration is checked.
#   With deploy dirs, each tree is additionally scanned for X11 artifacts.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
fail=0

check_token() {
    if ! grep -q -- "$1" "$ROOT/CMakeLists.txt"; then
        echo "ERROR: expected token '$1' missing from CMakeLists.txt" >&2
        fail=1
    fi
}

for token in qxcb qeglfs qlinuxfb qvnc qvkkhrdisplay qminimalegl xcbglintegrations egldeviceintegrations; do
    check_token "$token"
done
for token in qwayland qwayland-generic; do
    check_token "$token"
done

for dir in "$@"; do
    if [ ! -d "$dir" ]; then
        continue
    fi
    for pattern in '*xcb*' '*X11*' '*eglfs*'; do
        matches="$(find "$dir" -name "$pattern" 2>/dev/null || true)"
        if [ -n "$matches" ]; then
            echo "ERROR: X11 artifact found under $dir:" >&2
            echo "$matches" >&2
            fail=1
        fi
    done
done

exit "$fail"
