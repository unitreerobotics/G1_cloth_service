set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

ARCH="$(uname -m)"
echo "Building for host arch: $ARCH (default compiler)"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

echo ""
echo "Build completed. Output directory: $BUILD_DIR"
