#!/bin/bash
# Build using actual OpenWrt SDK for maximum compatibility
# This downloads the SDK and builds properly

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}OpenWrt SDK Build (Maximum Compatibility)${NC}"
echo "=========================================="
echo ""

# OpenWrt version selection
OPENWRT_VERSION="22.03.5"  # Stable version
ARCH="mipsel_24kc"

echo "Configuration:"
echo "  OpenWrt Version: $OPENWRT_VERSION"
echo "  Architecture: $ARCH"
echo ""

# Download SDK if not present
SDK_DIR="openwrt-sdk-${OPENWRT_VERSION}-ramips-mt7621_gcc-11.2.0_musl.Linux-x86_64"
SDK_FILE="${SDK_DIR}.tar.xz"
SDK_URL="https://downloads.openwrt.org/releases/${OPENWRT_VERSION}/targets/ramips/mt7621/${SDK_FILE}"

if [ ! -d "$SDK_DIR" ]; then
    echo -e "${YELLOW}Downloading OpenWrt SDK...${NC}"
    echo "This is about 150MB, please wait..."
    wget -q --show-progress "$SDK_URL" || {
        echo -e "${RED}Failed to download SDK${NC}"
        echo "Trying alternative architecture..."
        
        # Try alternative SDK
        SDK_DIR="openwrt-sdk-${OPENWRT_VERSION}-ath79-generic_gcc-11.2.0_musl.Linux-x86_64"
        SDK_FILE="${SDK_DIR}.tar.xz"
        SDK_URL="https://downloads.openwrt.org/releases/${OPENWRT_VERSION}/targets/ath79/generic/${SDK_FILE}"
        wget -q --show-progress "$SDK_URL" || exit 1
    }
    
    echo "Extracting SDK..."
    tar xf "$SDK_FILE"
    rm "$SDK_FILE"
fi

# Set up environment
export STAGING_DIR="$(pwd)/$SDK_DIR/staging_dir"
TOOLCHAIN=$(find "$STAGING_DIR" -maxdepth 1 -name "toolchain-*" | head -1)
export PATH="$TOOLCHAIN/bin:$PATH"
CROSS_COMPILE=$(ls $TOOLCHAIN/bin/*-gcc | head -1 | sed 's/-gcc$//' | xargs basename)-

echo -e "${GREEN}SDK ready, building...${NC}"

# Create SDK-specific Makefile
cat > Makefile.sdk << EOF
# OpenWrt SDK build
STAGING_DIR = $STAGING_DIR
TOOLCHAIN = $TOOLCHAIN
CROSS_COMPILE = $CROSS_COMPILE
CC = \$(CROSS_COMPILE)gcc
STRIP = \$(CROSS_COMPILE)strip

# Flags optimized for OpenWrt
CFLAGS = -Wall -Os -pipe -mno-branch-likely -mips32r2 -mtune=24kc \
         -fno-caller-saves -fno-plt -fhonour-copts \
         -Wno-error=unused-but-set-variable -Wno-error=unused-result \
         -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
         -I\$(TOOLCHAIN)/include

LDFLAGS = -L\$(TOOLCHAIN)/lib -static
LIBS = -lpthread

LIBDIR = lib
LIB_SOURCES = \$(LIBDIR)/json.c \$(LIBDIR)/cloudflare_utils.c \\
              \$(LIBDIR)/socket_http.c \$(LIBDIR)/publicip.c \\
              \$(LIBDIR)/getip.c \$(LIBDIR)/setip.c

all: cloudflare_renew
	\$(STRIP) cloudflare_renew
	@echo "Build complete!"

cloudflare_renew: cloudflare_renew.c \$(LIB_SOURCES)
	\$(CC) \$(CFLAGS) -o \$@ \$^ \$(LDFLAGS) \$(LIBS)

clean:
	rm -f cloudflare_renew
EOF

# Build
echo "Compiling with OpenWrt SDK..."
make -f Makefile.sdk clean
make -f Makefile.sdk

if [ -f cloudflare_renew ]; then
    echo -e "\n${GREEN}✓ Build successful!${NC}"
    echo ""
    echo "Binary info:"
    file cloudflare_renew
    ls -lh cloudflare_renew
    
    # Create package
    mkdir -p openwrt-build
    cp cloudflare_renew openwrt-build/
    cp cloudflare.conf.sample openwrt-build/
    tar czf cloudflare-renew-openwrt.tar.gz -C openwrt-build .
    
    echo ""
    echo -e "${GREEN}Package created: cloudflare-renew-openwrt.tar.gz${NC}"
    echo ""
    echo "This binary should work on any OpenWrt $OPENWRT_VERSION router"
    echo "with MIPSEL 24Kc architecture."
else
    echo -e "${RED}Build failed!${NC}"
fi

echo ""
echo -e "${YELLOW}Deployment:${NC}"
echo "1. Copy to router:"
echo "   scp cloudflare_renew root@router:/tmp/"
echo ""
echo "2. Test on router:"
echo "   ssh root@router"
echo "   chmod +x /tmp/cloudflare_renew"
echo "   /tmp/cloudflare_renew --help"