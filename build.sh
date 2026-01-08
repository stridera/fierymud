#!/bin/bash
# FieryMUD Build Script - Limits parallel jobs to prevent WSL crashes
#
# Usage:
#   ./build.sh              # Build all targets
#   ./build.sh fierymud     # Build specific target
#   ./build.sh -j4          # Override job limit
#   ./build.sh clean        # Clean build directory
#   ./build.sh reconfigure  # Reconfigure CMake

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Default to half of available cores, minimum 2
TOTAL_CORES=$(nproc 2>/dev/null || echo 4)
DEFAULT_JOBS=$(( (TOTAL_CORES + 1) / 2 ))
DEFAULT_JOBS=$(( DEFAULT_JOBS < 2 ? 2 : DEFAULT_JOBS ))

JOBS=$DEFAULT_JOBS
TARGET=""
ACTION="build"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -j*)
            if [[ ${#1} -gt 2 ]]; then
                JOBS="${1:2}"
            else
                shift
                JOBS="$1"
            fi
            ;;
        clean)
            ACTION="clean"
            ;;
        reconfigure)
            ACTION="reconfigure"
            ;;
        *)
            TARGET="$1"
            ;;
    esac
    shift
done

case $ACTION in
    clean)
        echo "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        echo "Done."
        exit 0
        ;;
    reconfigure)
        echo "Reconfiguring CMake..."
        rm -rf "$BUILD_DIR"
        cmake -B "$BUILD_DIR" -G Ninja "$SCRIPT_DIR"
        echo "Done. Run ./build.sh to build."
        exit 0
        ;;
esac

# Configure if needed
if [[ ! -f "$BUILD_DIR/build.ninja" ]]; then
    echo "Configuring CMake..."
    cmake -B "$BUILD_DIR" -G Ninja "$SCRIPT_DIR"
fi

# Build
echo "Building with $JOBS parallel jobs (out of $TOTAL_CORES cores)..."
if [[ -n "$TARGET" ]]; then
    cmake --build "$BUILD_DIR" --target "$TARGET" -j "$JOBS"
else
    cmake --build "$BUILD_DIR" -j "$JOBS"
fi

echo "Build complete."
