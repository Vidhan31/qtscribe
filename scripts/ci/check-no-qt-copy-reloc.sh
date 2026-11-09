#!/usr/bin/env bash
#
# No-Qt-COPY-reloc link guard.
# A COPY relocation against a Qt symbol in the qtscribe executable means a
# TU was compiled with executable-semantics codegen (-fPIE + direct extern
# access) instead of -fPIC. The executable then shadows the Qt definition with
# its own slot; for mutable singletons such as QCoreApplication::self the
# shadow stays null while libQt6Core writes its private copy, and the first
# cross-library read (QGuiApplication::screenAdded during Wayland startup)
# segfaults. Any Qt COPY reloc therefore fails this test.
#
# Usage: check-no-qt-copy-reloc.sh [BINARY...]
#   With no args, $ROOT/build/qtscribe is checked.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [ "$#" -gt 0 ]; then
    binaries=("$@")
else
    binaries=("$ROOT/build/qtscribe")
fi

if ! command -v readelf >/dev/null 2>&1; then
    echo "ERROR: readelf not found; cannot verify relocations" >&2
    exit 1
fi

fail=0
for bin in "${binaries[@]}"; do
    if [ ! -f "$bin" ]; then
        echo "ERROR: binary not found: $bin" >&2
        fail=1
        continue
    fi
    matches="$(readelf -rW "$bin" 2>/dev/null | grep 'R_X86_64_COPY' | grep -E '@Qt_6|@Qt6' || true)"
    if [ -n "$matches" ]; then
        echo "ERROR: COPY relocation(s) against Qt symbols in $bin:" >&2
        echo "$matches" >&2
        fail=1
    fi
done

exit "$fail"
