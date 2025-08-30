#!/bin/bash
# Quick build script for MIPSEL-24k cross-compilation
# Usage: ./build-mipsel.sh [method]

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Default method
METHOD=${1:-auto}

echo -e "${BLUE}cloudflare-renew MIPSEL-24k Cross-Compilation${NC}"
echo "=============================================="
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to build with generic MIPSEL toolchain
build_generic() {
    echo -e "${GREEN}Building with generic MIPSEL toolchain...${NC}"
    
    if ! command_exists mipsel-linux-gnu-gcc; then
        echo -e "${YELLOW}Installing MIPSEL toolchain...${NC}"
        if command_exists apt-get; then
            sudo apt-get update && sudo apt-get install -y gcc-mipsel-linux-gnu
        elif command_exists dnf; then
            sudo dnf install -y gcc-mipsel-linux-gnu
        else
            echo -e "${RED}Cannot auto-install toolchain. Please install manually.${NC}"
            exit 1
        fi
    fi
    
    echo "Cleaning previous build..."
    make -f Makefile.mipsel clean
    
    echo "Building binaries..."
    make -f Makefile.mipsel all
    
    echo "Stripping binaries..."
    make -f Makefile.mipsel strip
    
    echo "Creating distribution package..."
    make -f Makefile.mipsel dist
    
    echo -e "\n${GREEN}✓ Build complete!${NC}"
    make -f Makefile.mipsel size
}

# Function to build with OpenWrt SDK
build_openwrt() {
    echo -e "${GREEN}Building with OpenWrt SDK...${NC}"
    
    # Source environment
    if [ -f setup-openwrt-env.sh ]; then
        source ./setup-openwrt-env.sh
    fi
    
    if ! command_exists ${CROSS_COMPILE}gcc 2>/dev/null; then
        echo -e "${RED}OpenWrt SDK not found or not configured.${NC}"
        echo "Please install OpenWrt SDK first."
        exit 1
    fi
    
    echo "Cleaning previous build..."
    make -f Makefile.cross clean
    
    echo "Building binaries..."
    make -f Makefile.cross all
    
    echo "Stripping binaries..."
    make -f Makefile.cross strip
    
    echo -e "\n${GREEN}✓ Build complete!${NC}"
}

# Function to build with Docker
build_docker() {
    echo -e "${GREEN}Building with Docker...${NC}"
    
    if ! command_exists docker; then
        echo -e "${RED}Docker is not installed.${NC}"
        echo "Please install Docker first: https://docs.docker.com/get-docker/"
        exit 1
    fi
    
    echo "Building in Docker container..."
    make -f Makefile.docker
    
    echo -e "\n${GREEN}✓ Build complete!${NC}"
    echo "Binaries are in ./build-output/"
    ls -lh build-output/
}

# Auto-detect best method
auto_build() {
    echo "Auto-detecting best build method..."
    
    if command_exists mipsel-linux-gnu-gcc; then
        echo -e "${GREEN}Found generic MIPSEL toolchain${NC}"
        build_generic
    elif command_exists mipsel-openwrt-linux-musl-gcc; then
        echo -e "${GREEN}Found OpenWrt SDK${NC}"
        build_openwrt
    elif command_exists docker; then
        echo -e "${GREEN}Found Docker${NC}"
        build_docker
    else
        echo -e "${YELLOW}No toolchain found. Attempting to install generic MIPSEL...${NC}"
        build_generic
    fi
}

# Main logic
case "$METHOD" in
    generic|mipsel)
        build_generic
        ;;
    openwrt|sdk)
        build_openwrt
        ;;
    docker)
        build_docker
        ;;
    auto|"")
        auto_build
        ;;
    help|--help|-h)
        echo "Usage: $0 [method]"
        echo ""
        echo "Methods:"
        echo "  auto    - Auto-detect best method (default)"
        echo "  generic - Use generic MIPSEL toolchain"
        echo "  openwrt - Use OpenWrt SDK"
        echo "  docker  - Use Docker"
        echo "  help    - Show this help"
        echo ""
        echo "Examples:"
        echo "  $0           # Auto-detect"
        echo "  $0 generic   # Force generic toolchain"
        echo "  $0 docker    # Use Docker"
        exit 0
        ;;
    *)
        echo -e "${RED}Unknown method: $METHOD${NC}"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac

echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "1. Copy binaries to your router:"
echo "   scp cloudflare_renew root@router:/usr/bin/"
echo ""
echo "2. Configure on router:"
echo "   ssh root@router"
echo "   vi /etc/cloudflare/cloudflare.conf"
echo "   vi /etc/cloudflare/cloudflare.token"
echo ""
echo "3. Test:"
echo "   /usr/bin/cloudflare_renew"
echo ""
echo "4. Add to cron for automatic updates:"
echo "   echo '*/15 * * * * /usr/bin/cloudflare_renew' >> /etc/crontabs/root"