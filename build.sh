#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

ARM64_CC="/home/caoboxi/extra/yarnell/G1-D/aarch64--glibc--stable-2020.08-1/bin/aarch64-linux-gcc"
ARM64_CXX="/home/caoboxi/extra/yarnell/G1-D/aarch64--glibc--stable-2020.08-1/bin/aarch64-linux-g++"

show_usage() {
    echo "Usage: $0 [x86|arm64]"
    echo ""
    echo "Platforms:"
    echo "  x86   - Build with system default gcc"
    echo "  arm64 - Build with ARM64 cross compiler"
    echo ""
    echo "If no platform is specified, you will be prompted to choose one."
}

select_platform() {
    echo "Select build platform:"
    echo "  1) x86"
    echo "  2) arm64"
    read -rp "Enter choice [1-2]: " choice
    case "$choice" in
        1) PLATFORM="x86" ;;
        2) PLATFORM="arm64" ;;
        *) echo "Invalid choice: $choice" >&2; exit 1 ;;
    esac
}

PLATFORM=""

if [ $# -gt 1 ]; then
    show_usage
    exit 1
elif [ $# -eq 1 ]; then
    case "$1" in
        x86|x86_64|amd64) PLATFORM="x86" ;;
        arm64|aarch64)    PLATFORM="arm64" ;;
        -h|--help)        show_usage; exit 0 ;;
        *)                echo "Unknown platform: $1" >&2; show_usage; exit 1 ;;
    esac
else
    select_platform
fi

echo "Building for platform: $PLATFORM"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

case "$PLATFORM" in
    x86)
        cmake -S "$PROJECT_DIR" -B "$BUILD_DIR"
        ;;
    arm64)
        if [ ! -x "$ARM64_CC" ]; then
            echo "ARM64 compiler not found or not executable: $ARM64_CC" >&2
            exit 1
        fi
        export CC="$ARM64_CC"
        export CXX="$ARM64_CXX"
        cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
            -DCMAKE_C_COMPILER="$ARM64_CC" \
            -DCMAKE_CXX_COMPILER="$ARM64_CXX"
        ;;
esac

cmake --build "$BUILD_DIR"

echo ""
echo "Build completed. Output directory: $BUILD_DIR"
