#!/bin/bash
# Automatically rebuild SONAME symbolic links for dynamic libraries
# Add LD_LIBRARY_PATH information 
# 
# Usage: ./rebuild_library_links.sh


set -e

# Get project root directory
UNITREE_SDK2_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "== UNITREE_SDK2_ROOT: $UNITREE_SDK2_ROOT"

# Detect system architecture
ARCH=$(uname -m)
case "$ARCH" in
    x86_64)
        LIB_ARCH="x86_64"
        ;;
    aarch64|arm64)
        LIB_ARCH="aarch64"
        ;;
    *)
        echo "Error: Unsupported architecture: $ARCH"
        echo "Supported architectures: x86_64, aarch64"
        exit 1
        ;;
esac

echo "== Detected architecture: $ARCH -> $LIB_ARCH"

# Library directories list for current architecture
# LIB_DIRS=(
#     "$UNITREE_SDK2_ROOT/thirdparty/lib/$LIB_ARCH"
#     "$UNITREE_SDK2_ROOT/lib/$LIB_ARCH"
# )
LIB_DIRS=(
    "$UNITREE_SDK2_ROOT/thirdparty/lib/aarch64"
    "$UNITREE_SDK2_ROOT/thirdparty/lib/x86_64"
    "$UNITREE_SDK2_ROOT/lib/aarch64"
    "$UNITREE_SDK2_ROOT/lib/x86_64"
)



echo "Rebuilding dynamic library symbolic links..."

for lib_dir in "${LIB_DIRS[@]}"; do
    [ ! -d "$lib_dir" ] && continue
    
    echo "Processing directory: $lib_dir"
    cd "$lib_dir"
    
    link_count=0
    for lib in *.so*; do
        [[ "$lib" == "*.so*" ]] && continue  # No matching files
        [ -f "$lib" ] && [ ! -L "$lib" ] || continue
        
        soname=$(readelf -d "$lib" 2>/dev/null | grep "Library soname:" | sed 's/.*Library soname: \[\(.*\)\]/\1/' || true)
        [ -n "$soname" ] && [ "$soname" != "$lib" ] || continue
        
        echo "  $soname -> $lib"
        ln -sf "$lib" "$soname" || true
        ((link_count++)) || true
    done
    
    echo "  Created $link_count symbolic links"
done

echo "Symbolic link rebuilding completed!"
