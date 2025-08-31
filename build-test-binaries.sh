#!/bin/bash
# Build simple test binaries to identify the segfault issue

set -e

echo "Building MIPS test binaries..."
echo "=============================="
echo ""

mkdir -p test-builds

# Test 1: Minimal static binary with musl
echo "1. Building minimal static (musl)..."
docker run --rm -v $(pwd):/src alpine:latest sh -c "
    apk add --no-cache build-base wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -static -Os -o /src/test-builds/test-musl-static \
        /src/test-mips.c
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip \
        /src/test-builds/test-musl-static
"
echo "  Created: test-musl-static"

# Test 2: Dynamic binary with glibc
echo "2. Building dynamic (glibc)..."
docker run --rm -v $(pwd):/src debian:bullseye sh -c "
    apt-get update >/dev/null 2>&1
    apt-get install -y gcc-mipsel-linux-gnu >/dev/null 2>&1
    mipsel-linux-gnu-gcc -Os -o /src/test-builds/test-glibc-dynamic \
        /src/test-mips.c
    mipsel-linux-gnu-strip /src/test-builds/test-glibc-dynamic
"
echo "  Created: test-glibc-dynamic"

# Test 3: Generic MIPS32
echo "3. Building generic MIPS32..."
docker run --rm -v $(pwd):/src alpine:latest sh -c "
    apk add --no-cache build-base wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -march=mips32 -static -Os -o /src/test-builds/test-mips32 \
        /src/test-mips.c
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip \
        /src/test-builds/test-mips32
"
echo "  Created: test-mips32"

# Test 4: With soft-float
echo "4. Building with soft-float..."
docker run --rm -v $(pwd):/src alpine:latest sh -c "
    apk add --no-cache build-base wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -msoft-float -static -Os -o /src/test-builds/test-softfloat \
        /src/test-mips.c 2>/dev/null || \
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -static -Os -o /src/test-builds/test-softfloat \
        /src/test-mips.c
    /tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip \
        /src/test-builds/test-softfloat
"
echo "  Created: test-softfloat"

echo ""
echo "Test binaries created:"
ls -lh test-builds/
echo ""
echo "To test on your router:"
echo "1. Copy test binaries:"
echo "   scp test-builds/* root@router:/tmp/"
echo ""
echo "2. Run each test:"
echo "   ssh root@router"
echo "   cd /tmp"
echo "   ./test-musl-static   # Should work on most OpenWrt"
echo "   ./test-glibc-dynamic # For glibc-based routers"
echo "   ./test-mips32        # Generic MIPS32"
echo "   ./test-softfloat     # With soft-float"
echo ""
echo "3. The one that works indicates the correct build config!"