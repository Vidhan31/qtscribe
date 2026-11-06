#!/usr/bin/env bash
#
# Check QML Ahead-of-Time (AOT) compilation statistics.
# Scans generated .aotstats files or all_aotstats.txt for any bindings
# that failed AOT compilation or fell back to interpreted JavaScript.
#
# Usage: check-aot-stats.sh [BUILD_DIR]
#   Defaults to ./build, ./build-release, or ./build-relwithdebinfo.

set -euo pipefail

BUILD_DIR="${1:-}"
if [ -z "$BUILD_DIR" ]; then
    for candidate in build build-release build-relwithdebinfo; do
        if [ -d "$candidate" ]; then
            BUILD_DIR="$candidate"
            break
        fi
    done
fi

if [ -z "$BUILD_DIR" ] || [ ! -d "$BUILD_DIR" ]; then
    echo "Notice: No build directory found. Skipping AOT stats validation."
    exit 0
fi

# Locate stats file
STATS_FILE="$BUILD_DIR/.rcc/qmlcache/all_aotstats.txt"
if [ ! -f "$STATS_FILE" ] && [ -f "$BUILD_DIR/all_aotstats.txt" ]; then
    STATS_FILE="$BUILD_DIR/all_aotstats.txt"
fi

if [ ! -f "$STATS_FILE" ]; then
    echo "Notice: all_aotstats.txt not found in $BUILD_DIR. Run 'ninja -C $BUILD_DIR all_aotstats' first."
    exit 0
fi

echo "Validating QML AOT compilation statistics from: $STATS_FILE"

SUMMARY=$(grep -E "(Total results|Module QtScribe):" "$STATS_FILE" | tail -n 1 || true)
if [ -n "$SUMMARY" ]; then
    echo "Summary: $SUMMARY"
fi

INTERPRETED_COUNT=$(grep -c "result: Interpreted" "$STATS_FILE" || true)
FAILURE_COUNT=$(grep -c "result: Failure" "$STATS_FILE" || true)

if [ "$INTERPRETED_COUNT" -gt 0 ] || [ "$FAILURE_COUNT" -gt 0 ]; then
    echo "WARNING: Detected $INTERPRETED_COUNT interpreted and $FAILURE_COUNT failed QML bindings in $STATS_FILE." >&2
    grep -B 2 -E "result: (Interpreted|Failure)" "$STATS_FILE" >&2 || true
    if [ "${STRICT_AOT:-0}" = "1" ]; then
        exit 1
    fi
else
    echo "SUCCESS: 100% of QML bindings compiled to native C++ successfully."
fi
