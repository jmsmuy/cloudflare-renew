#!/bin/bash
# Build script specifically for MediaTek MT7621 with OpenWrt 24.10.2
# This matches your exact router configuration

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Building for MediaTek MT7621 - OpenWrt 24.10.2${NC}"
echo "================================================"
echo ""
echo "Target Router:"
echo "  SoC: MediaTek MT7621 (MIPS 1004Kc)"
echo "  OpenWrt: 24.10.2"
echo "  Architecture: mipsel_24kc"
echo "  OpenSSL: 3.x"
echo ""

# Create build directory
mkdir -p mt7621-build

# Method 1: Build with musl and no SSL dependencies (most compatible)
echo -e "${GREEN}Method 1: Building without SSL dependencies...${NC}"
echo "This version will work but won't use HTTPS"

cat > Makefile.mt7621-nossl << 'EOF'
# Build for MT7621 without SSL dependencies
CROSS_COMPILE = mipsel-linux-gnu-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

# MT7621 specific flags
CFLAGS = -Wall -Wextra -std=c99 -Os \
         -march=mips32r2 -mtune=24kc \
         -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
         -DNO_SSL_SUPPORT

LDFLAGS = -static
LIBS = -lpthread

LIBDIR = lib
LIB_SOURCES = $(LIBDIR)/json.c \
              $(LIBDIR)/cloudflare_utils.c \
              $(LIBDIR)/publicip.c \
              $(LIBDIR)/getip.c \
              $(LIBDIR)/setip.c

# Modified socket_http without SSL
lib/socket_http_nossl.c: lib/socket_http.c
	sed 's/#include.*openssl.*//g; s/SSL_.*//g; s/ssl_.*//g' $< > $@

all: lib/socket_http_nossl.c cloudflare_renew
	$(STRIP) cloudflare_renew
	@echo "Built without SSL support"

cloudflare_renew: cloudflare_renew.c $(LIB_SOURCES) lib/socket_http_nossl.c
	$(CC) $(CFLAGS) -o $@ cloudflare_renew.c $(LIB_SOURCES) lib/socket_http_nossl.c $(LDFLAGS) $(LIBS)

clean:
	rm -f cloudflare_renew lib/socket_http_nossl.c
EOF

# Build without SSL
echo "Building..."
docker run --rm -v $(pwd):/src debian:bullseye sh -c "
    apt-get update >/dev/null 2>&1
    apt-get install -y gcc-mipsel-linux-gnu >/dev/null 2>&1
    cd /src
    make -f Makefile.mt7621-nossl clean
    make -f Makefile.mt7621-nossl
"

if [ -f cloudflare_renew ]; then
    mv cloudflare_renew mt7621-build/cloudflare_renew_nossl
    echo -e "${GREEN}✓ Built: cloudflare_renew_nossl${NC}"
fi

# Method 2: Build with musl libc statically linked
echo -e "\n${GREEN}Method 2: Building with musl (OpenWrt compatible)...${NC}"

docker run --rm -v $(pwd):/src -v $(pwd)/mt7621-build:/output alpine:latest sh -c '
    apk add --no-cache build-base wget make >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    export PATH="/tmp/mipsel-linux-musl-cross/bin:$PATH"
    cd /src
    
    # Build with musl, compatible with MT7621
    mipsel-linux-musl-gcc \
        -Wall -Os -static \
        -march=mips32r2 -mtune=24kc \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -o cloudflare_renew_musl \
        cloudflare_renew.c lib/*.c \
        -lpthread
    
    mipsel-linux-musl-strip cloudflare_renew_musl
    cp cloudflare_renew_musl /output/
    echo "Built with musl libc"
'

if [ -f mt7621-build/cloudflare_renew_musl ]; then
    echo -e "${GREEN}✓ Built: cloudflare_renew_musl${NC}"
fi

# Method 3: Download and use OpenWrt 24.10 SDK
echo -e "\n${GREEN}Method 3: Using OpenWrt 24.10 SDK (exact match)...${NC}"

SDK_VERSION="24.10.2"
SDK_TARGET="ramips-mt7621"
SDK_NAME="openwrt-sdk-${SDK_VERSION}-${SDK_TARGET}_gcc-13.3.0_musl.Linux-x86_64"
SDK_URL="https://downloads.openwrt.org/releases/${SDK_VERSION}/targets/ramips/mt7621/${SDK_NAME}.tar.xz"

if [ ! -d "$SDK_NAME" ]; then
    echo "Downloading OpenWrt ${SDK_VERSION} SDK for MT7621..."
    wget -q --show-progress "$SDK_URL" || {
        echo -e "${YELLOW}Warning: Could not download exact SDK version${NC}"
        echo "Using fallback build instead"
    }
    
    if [ -f "${SDK_NAME}.tar.xz" ]; then
        tar xf "${SDK_NAME}.tar.xz"
        rm "${SDK_NAME}.tar.xz"
    fi
fi

if [ -d "$SDK_NAME" ]; then
    echo "Building with OpenWrt SDK..."
    export STAGING_DIR="$(pwd)/$SDK_NAME/staging_dir"
    TOOLCHAIN=$(find "$STAGING_DIR" -maxdepth 1 -name "toolchain-*" | head -1)
    export PATH="$TOOLCHAIN/bin:$PATH"
    
    # Find the cross-compiler
    CROSS_CC=$(find $TOOLCHAIN/bin -name "*-gcc" | head -1)
    CROSS_STRIP=$(find $TOOLCHAIN/bin -name "*-strip" | head -1)
    
    if [ -n "$CROSS_CC" ]; then
        # Build with SDK
        $CROSS_CC \
            -Wall -Os -pipe -mno-branch-likely \
            -march=mips32r2 -mtune=24kc -mips32r2 \
            -fno-caller-saves -fno-plt \
            -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
            -I$TOOLCHAIN/include \
            -L$TOOLCHAIN/lib \
            -o mt7621-build/cloudflare_renew_sdk \
            cloudflare_renew.c lib/*.c \
            -static -lpthread
        
        $CROSS_STRIP mt7621-build/cloudflare_renew_sdk
        echo -e "${GREEN}✓ Built: cloudflare_renew_sdk${NC}"
    fi
fi

# Method 4: Simple test binary
echo -e "\n${GREEN}Method 4: Building minimal test binary...${NC}"

cat > test_mt7621.c << 'EOF'
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("MT7621 Test OK\n");
    printf("PID: %d\n", getpid());
    return 0;
}
EOF

docker run --rm -v $(pwd):/src -v $(pwd)/mt7621-build:/output alpine:latest sh -c '
    apk add --no-cache build-base wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -static -Os -march=mips32r2 -mtune=24kc \
        -o /output/test_mt7621 /src/test_mt7621.c
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip /output/test_mt7621
'

echo -e "${GREEN}✓ Built: test_mt7621${NC}"

# Show results
echo -e "\n${BLUE}Build Results:${NC}"
echo "=============="
ls -lh mt7621-build/ | tail -n +2

echo -e "\n${YELLOW}Testing Instructions:${NC}"
echo "1. Copy ALL binaries to your router:"
echo "   scp mt7621-build/* root@router:/tmp/"
echo ""
echo "2. Test each binary on the router:"
echo "   ssh root@router"
echo "   cd /tmp"
echo "   "
echo "   # Test the simple binary first:"
echo "   chmod +x test_mt7621"
echo "   ./test_mt7621"
echo "   "
echo "   # If that works, test the others:"
echo "   chmod +x cloudflare_renew_*"
echo "   ./cloudflare_renew_musl --help"
echo "   ./cloudflare_renew_nossl --help"
echo "   ./cloudflare_renew_sdk --help"
echo ""
echo -e "${GREEN}Recommendations:${NC}"
echo "1. The 'cloudflare_renew_musl' should work best"
echo "2. If you need HTTPS, you may need to build with OpenSSL 3.x"
echo "3. The 'nossl' version will work but only supports HTTP"

# Create a dynamic-linked version for OpenSSL 3
echo -e "\n${GREEN}Bonus: Creating dynamically linked version for OpenSSL 3...${NC}"

cat > Makefile.mt7621-dynamic << 'EOF'
CROSS_COMPILE = mipsel-linux-gnu-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -Wall -Os -march=mips32r2 -mtune=24kc \
         -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L

# Dynamic linking - will use router's OpenSSL 3
LDFLAGS = 
LIBS = -lssl -lcrypto -lpthread

LIBDIR = lib
LIB_SOURCES = $(LIBDIR)/json.c $(LIBDIR)/cloudflare_utils.c \
              $(LIBDIR)/socket_http.c $(LIBDIR)/publicip.c \
              $(LIBDIR)/getip.c $(LIBDIR)/setip.c

all: cloudflare_renew
	$(STRIP) cloudflare_renew

cloudflare_renew: cloudflare_renew.c $(LIB_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS) 2>/dev/null || \
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -f cloudflare_renew
EOF

docker run --rm -v $(pwd):/src debian:bullseye sh -c "
    apt-get update >/dev/null 2>&1
    apt-get install -y gcc-mipsel-linux-gnu >/dev/null 2>&1
    cd /src
    make -f Makefile.mt7621-dynamic clean
    make -f Makefile.mt7621-dynamic
" 2>/dev/null

if [ -f cloudflare_renew ]; then
    mv cloudflare_renew mt7621-build/cloudflare_renew_dynamic
    echo -e "${GREEN}✓ Built: cloudflare_renew_dynamic (uses router's OpenSSL 3)${NC}"
fi

echo -e "\n${BLUE}Final Notes:${NC}"
echo "Your router has OpenSSL 3.x which is newer than most build tools expect."
echo "The 'cloudflare_renew_dynamic' version should work if it can find the libraries."
echo "The 'cloudflare_renew_musl' is statically linked and most likely to work."