#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

DEFAULT_VERSION=$(tr -d '[:space:]' < "${REPO_ROOT}/VERSION" 2>/dev/null || echo "1.0.0")
FEDORA_VERSION="${1:-44}"
APP_VERSION="${2:-$DEFAULT_VERSION}"
APP_RELEASE="${3:-1}"
OUTPUT_DIR="${REPO_ROOT}/dist/rpm"

echo "======================================================================"
echo " Building QtScribe RPM for Fedora (${FEDORA_VERSION})"
echo " Version: ${APP_VERSION}-${APP_RELEASE}"
echo " Output : ${OUTPUT_DIR}"
echo "======================================================================"

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

docker buildx build \
    --file "${SCRIPT_DIR}/Dockerfile.rpm" \
    --build-arg FEDORA_VERSION="${FEDORA_VERSION}" \
    --build-arg APP_VERSION="${APP_VERSION}" \
    --build-arg APP_RELEASE="${APP_RELEASE}" \
    --target export \
    --output type=local,dest="${OUTPUT_DIR}" \
    "${REPO_ROOT}"

echo "======================================================================"
echo " Build successful! RPM artifacts exported to: ${OUTPUT_DIR}"
ls -lh "${OUTPUT_DIR}"
echo "======================================================================"
