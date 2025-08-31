#!/bin/bash
# Build script specifically for Debian/Ubuntu systems
# Handles the soft-float library issue

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}MIPSEL Cross-Compilation for OpenWrt (Debian/Ubuntu)${NC}"
echo "======================================================"
echo ""

# Check if running on Debian/Ubuntu
if ! (lsb_release -d 2>/dev/null | grep -E "Debian|Ubuntu" > /dev/null); then
    echo -e "${YELLOW}Warning: This script is optimized for Debian/Ubuntu${NC}"
fi

# Step 1: Install dependencies
echo -e "${GREEN}Step 1: Checking/Installing dependencies...${NC}"

PACKAGES_NEEDED=""

# Check for MIPSEL GCC
if ! command -v mipsel-linux-gnu-gcc &> /dev/null; then
    PACKAGES_NEEDED="$PACKAGES_NEEDED gcc-mipsel-linux-gnu"
fi

# Check for MIPSEL libc dev
if ! dpkg -l | grep -q "libc6-dev-mipsel-cross"; then
    PACKAGES_NEEDED="$PACKAGES_NEEDED libc6-dev-mipsel-cross"
fi

# Check for OpenSSL dev
if ! dpkg -l | grep -q "libssl-dev"; then
    PACKAGES_NEEDED="$PACKAGES_NEEDED libssl-dev"
fi

if [ -n "$PACKAGES_NEEDED" ]; then
    echo "Installing required packages: $PACKAGES_NEEDED"
    sudo apt-get update
    sudo apt-get install -y $PACKAGES_NEEDED
else
    echo -e "${GREEN}✓ All dependencies installed${NC}"
fi

# Step 2: Test compiler
echo -e "\n${GREEN}Step 2: Testing compiler configuration...${NC}"

cat > test_mipsel.c << 'EOF'
#include <stdio.h>
int main() {
    printf("MIPSEL test successful\n");
    return 0;
}
EOF

echo -n "Testing basic compilation: "
if mipsel-linux-gnu-gcc test_mipsel.c -o test_mipsel 2>/dev/null; then
    echo -e "${GREEN}✓${NC}"
    rm -f test_mipsel
else
    echo -e "${RED}✗${NC}"
    echo "Basic compilation failed!"
    exit 1
fi

echo -n "Testing with MIPS32R2 flags: "
if mipsel-linux-gnu-gcc -march=mips32r2 test_mipsel.c -o test_mipsel 2>/dev/null; then
    echo -e "${GREEN}✓${NC}"
    rm -f test_mipsel
    ARCH_FLAGS="-march=mips32r2"
else
    echo -e "${YELLOW}✗ Using default flags${NC}"
    ARCH_FLAGS=""
fi

echo -n "Testing soft-float (may fail on Debian): "
if mipsel-linux-gnu-gcc -msoft-float test_mipsel.c -o test_mipsel 2>/dev/null; then
    echo -e "${GREEN}✓ Soft-float supported${NC}"
    FLOAT_ABI="soft"
    rm -f test_mipsel
else
    echo -e "${YELLOW}✗ Using hard-float (default)${NC}"
    FLOAT_ABI="hard"
fi

rm -f test_mipsel.c test_mipsel

# Step 3: Build
echo -e "\n${GREEN}Step 3: Building cloudflare-renew...${NC}"
echo "Configuration:"
echo "  Compiler: mipsel-linux-gnu-gcc"
echo "  Architecture: MIPS32R2"
echo "  Float ABI: $FLOAT_ABI"
echo ""

# Clean previous build
make -f Makefile.debian-mipsel clean 2>/dev/null || true

# Build
echo "Compiling..."
if make -f Makefile.debian-mipsel all; then
    echo -e "${GREEN}✓ Build successful!${NC}"
else
    echo -e "${RED}Build failed!${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "1. Try installing additional libraries:"
    echo "   sudo apt-get install libssl-dev:mipsel"
    echo ""
    echo "2. If OpenSSL linking fails, try without SSL:"
    echo "   Edit Makefile.debian-mipsel and remove '-lssl -lcrypto' from LIBS"
    echo ""
    echo "3. Check the error messages above for specific issues"
    exit 1
fi

# Step 4: Strip binaries
echo -e "\n${GREEN}Step 4: Stripping binaries...${NC}"
make -f Makefile.debian-mipsel strip

# Step 5: Show results
echo -e "\n${GREEN}Step 5: Build results...${NC}"
echo "Binary information:"
file cloudflare_renew
echo ""
echo "Binary sizes:"
ls -lh cloudflare_renew tools/getip tools/setip tools/publicip | awk '{print "  " $NF ": " $5}'
echo ""
echo "Architecture check:"
mipsel-linux-gnu-readelf -h cloudflare_renew | grep -E "Class:|Machine:|Flags:"

# Step 6: Package
echo -e "\n${GREEN}Step 6: Creating distribution package...${NC}"
mkdir -p dist-mipsel
cp cloudflare_renew tools/getip tools/setip tools/publicip dist-mipsel/
cp cloudflare.conf.sample dist-mipsel/
tar czf cloudflare-renew-mipsel-debian.tar.gz -C dist-mipsel .
echo -e "${GREEN}✓ Package created: cloudflare-renew-mipsel-debian.tar.gz${NC}"

# Done
echo -e "\n${BLUE}═══════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
echo ""
echo "Next steps:"
echo "1. Copy to your router:"
echo "   scp cloudflare-renew-mipsel-debian.tar.gz root@router:/tmp/"
echo ""
echo "2. On the router:"
echo "   cd /tmp"
echo "   tar xzf cloudflare-renew-mipsel-debian.tar.gz"
echo "   mv cloudflare_renew /usr/bin/"
echo "   chmod +x /usr/bin/cloudflare_renew"
echo ""
echo "3. Test:"
echo "   cloudflare_renew --help"
echo ""
echo "Note: These binaries are dynamically linked."
echo "Your router needs compatible libraries (glibc/musl)."
echo "If you encounter library issues, try static linking."