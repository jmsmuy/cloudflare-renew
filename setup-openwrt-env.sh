#!/bin/bash
# Setup script for OpenWrt cross-compilation environment
# This script helps configure the build environment for MIPSEL-24k targets

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}OpenWrt Cross-Compilation Environment Setup${NC}"
echo "============================================"

# Function to detect OpenWrt SDK
find_openwrt_sdk() {
    local search_paths=(
        "$HOME/openwrt"
        "$HOME/openwrt-sdk"
        "$HOME/lede"
        "/opt/openwrt"
        "/usr/local/openwrt"
        "."
    )
    
    for path in "${search_paths[@]}"; do
        if [ -d "$path/staging_dir" ]; then
            echo "$path"
            return 0
        fi
    done
    return 1
}

# Check if OpenWrt SDK is installed
if SDK_PATH=$(find_openwrt_sdk); then
    echo -e "${GREEN}✓${NC} Found OpenWrt SDK at: $SDK_PATH"
    
    # Find the toolchain directory
    TOOLCHAIN=$(find "$SDK_PATH/staging_dir" -maxdepth 1 -type d -name "toolchain-mipsel_*" | head -1)
    if [ -n "$TOOLCHAIN" ]; then
        echo -e "${GREEN}✓${NC} Found toolchain: $(basename $TOOLCHAIN)"
        
        # Export environment variables
        export STAGING_DIR="$SDK_PATH/staging_dir"
        export PATH="$PATH:$TOOLCHAIN/bin"
        export CROSS_COMPILE="mipsel-openwrt-linux-musl-"
        
        echo ""
        echo "Environment variables set:"
        echo "  STAGING_DIR=$STAGING_DIR"
        echo "  CROSS_COMPILE=$CROSS_COMPILE"
        echo "  PATH updated with toolchain"
        
        # Test the compiler
        if command -v ${CROSS_COMPILE}gcc &> /dev/null; then
            echo -e "\n${GREEN}✓${NC} Toolchain is working!"
            ${CROSS_COMPILE}gcc --version | head -1
        else
            echo -e "\n${YELLOW}⚠${NC} Warning: Compiler not found in PATH"
        fi
    else
        echo -e "${YELLOW}⚠${NC} No MIPSEL toolchain found in SDK"
    fi
else
    echo -e "${YELLOW}⚠${NC} OpenWrt SDK not found"
    echo ""
    echo "To install OpenWrt SDK:"
    echo "1. Download from: https://downloads.openwrt.org/releases/"
    echo "2. Choose your version and look for SDK"
    echo "3. Download: openwrt-sdk-*-mipsel_24kc_*.tar.xz"
    echo "4. Extract to $HOME/openwrt-sdk/"
    echo ""
    echo "Alternative: Use generic MIPSEL toolchain"
    echo "  Ubuntu/Debian: sudo apt-get install gcc-mipsel-linux-gnu"
    echo "  Then use: make -f Makefile.mipsel"
fi

echo ""
echo "Build commands:"
echo "  make -f Makefile.cross          # Using OpenWrt SDK"
echo "  make -f Makefile.mipsel         # Using generic MIPSEL toolchain"
echo "  make -f Makefile.docker         # Using Docker (no local toolchain needed)"