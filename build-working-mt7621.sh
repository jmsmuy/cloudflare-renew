#!/bin/bash
# Working build for MT7621 - builds with proper musl toolchain

set -e

echo "MT7621 Working Build"
echo "===================="
echo ""

mkdir -p mt7621-output

# Method 1: Build with all source files using musl
echo "Building complete version with musl..."

docker run --rm -v $(pwd):/workspace -w /workspace alpine:latest sh -c '
    # Install musl cross-compiler
    echo "Setting up toolchain..."
    apk add --no-cache wget make curl >/dev/null 2>&1
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    rm mipsel-linux-musl-cross.tgz
    
    # Set compiler
    export PATH="$(pwd)/mipsel-linux-musl-cross/bin:$PATH"
    export CC="mipsel-linux-musl-gcc"
    export STRIP="mipsel-linux-musl-strip"
    
    # Download OpenSSL headers for compilation
    echo "Getting OpenSSL headers..."
    mkdir -p ssl-headers
    curl -sL https://github.com/openssl/openssl/archive/refs/tags/openssl-3.0.13.tar.gz | \
        tar xz --strip-components=2 -C ssl-headers \
        "openssl-openssl-3.0.13/include/*" 2>/dev/null || true
    
    # Build attempt 1: With SSL headers but no linking
    echo "Building version 1: With SSL stubs..."
    $CC -static -Os -Wall \
        -march=mips32r2 -mtune=24kc \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -I./ssl-headers/include \
        -o cloudflare_renew_v1 \
        cloudflare_renew.c lib/*.c \
        -lpthread 2>/dev/null || {
            echo "Version 1 failed, trying alternative..."
        }
    
    if [ -f cloudflare_renew_v1 ]; then
        $STRIP cloudflare_renew_v1
        echo "✓ Version 1 built"
    fi
    
    # Build attempt 2: Without any SSL
    echo "Building version 2: No SSL..."
    
    # Create modified socket_http without SSL
    sed -e "s|#include.*openssl.*||g" \
        -e "s|SSL\*|void\*|g" \
        -e "s|SSL_CTX\*|void\*|g" \
        -e "s|BIO\*|void\*|g" \
        lib/socket_http.c > lib/socket_http_nossl.c
    
    $CC -static -Os -Wall \
        -march=mips32r2 -mtune=24kc \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -DNO_SSL \
        -o cloudflare_renew_v2 \
        cloudflare_renew.c \
        lib/json.c \
        lib/cloudflare_utils.c \
        lib/socket_http_nossl.c \
        lib/publicip.c \
        lib/getip.c \
        lib/setip.c \
        -lpthread 2>/dev/null || {
            echo "Version 2 failed"
        }
    
    if [ -f cloudflare_renew_v2 ]; then
        $STRIP cloudflare_renew_v2
        echo "✓ Version 2 built"
    fi
    
    # Build a simple test
    echo "Building test binary..."
    cat > test.c << "EOF"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
int main(int argc, char **argv) {
    printf("MT7621 Test Success!\n");
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("Version 1.0 - MIPSEL build\n");
    }
    return 0;
}
EOF
    
    $CC -static -Os -march=mips32r2 -mtune=24kc -o test_mt7621 test.c
    $STRIP test_mt7621
    echo "✓ Test binary built"
'

# Move successful builds to output
for f in cloudflare_renew_v1 cloudflare_renew_v2 test_mt7621; do
    if [ -f "$f" ]; then
        mv "$f" mt7621-output/ 2>/dev/null || true
    fi
done

# Alternative: Use pre-built musl toolchain from Docker
echo ""
echo "Building alternative with Docker musl..."

docker run --rm -v $(pwd):/src -v $(pwd)/mt7621-output:/output \
    muslcc/x86_64:mipsel-linux-musl sh -c '
    cd /src
    echo "Compiling with muslcc..."
    
    # Try building without SSL libraries
    mipsel-linux-musl-gcc -static -Os \
        -march=mips32r2 -mtune=24kc \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -o /output/cloudflare_renew_musl \
        cloudflare_renew.c lib/*.c \
        -lpthread 2>/dev/null || {
            echo "Build with SSL failed, retrying without..."
            
            # Remove SSL includes and retry
            mipsel-linux-musl-gcc -static -Os \
                -march=mips32r2 -mtune=24kc \
                -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
                -DNO_SSL \
                -o /output/cloudflare_renew_musl \
                cloudflare_renew.c \
                lib/json.c lib/cloudflare_utils.c \
                lib/publicip.c lib/getip.c lib/setip.c \
                -lpthread 2>/dev/null || echo "Alternative build also failed"
        }
    
    if [ -f /output/cloudflare_renew_musl ]; then
        mipsel-linux-musl-strip /output/cloudflare_renew_musl
        echo "✓ Musl version built"
    fi
' 2>/dev/null || true

echo ""
echo "Build Results:"
echo "=============="
if [ -d mt7621-output ] && [ "$(ls -A mt7621-output)" ]; then
    ls -lh mt7621-output/
    
    echo ""
    echo "Testing Instructions:"
    echo "===================="
    echo "1. First test the simple binary:"
    echo "   scp mt7621-output/test_mt7621 root@router:/tmp/"
    echo "   ssh root@router"
    echo "   chmod +x /tmp/test_mt7621"
    echo "   /tmp/test_mt7621"
    echo ""
    echo "2. If that works, test the main binaries:"
    echo "   scp mt7621-output/cloudflare_renew_* root@router:/tmp/"
    echo "   ssh root@router"
    echo "   cd /tmp"
    echo "   for f in cloudflare_renew_*; do"
    echo '     echo "Testing $f:"'
    echo '     chmod +x $f'
    echo '     ./$f --version || echo "Failed"'
    echo "   done"
else
    echo "No binaries were built successfully."
    echo "Trying fallback method..."
    
    # Fallback: simplest possible build
    echo ""
    echo "Fallback: Building minimal version..."
    docker run --rm -v $(pwd):/src debian:bullseye sh -c '
        apt-get update && apt-get install -y gcc-mipsel-linux-gnu >/dev/null 2>&1
        cd /src
        
        # Build without SSL
        mipsel-linux-gnu-gcc -static-libgcc -Os \
            -march=mips32r2 \
            -D_GNU_SOURCE -DNO_SSL \
            -o cloudflare_fallback \
            cloudflare_renew.c lib/json.c \
            -lpthread 2>/dev/null || echo "Fallback failed"
            
        if [ -f cloudflare_fallback ]; then
            mipsel-linux-gnu-strip cloudflare_fallback
            echo "✓ Fallback version built"
        fi
    '
    
    if [ -f cloudflare_fallback ]; then
        mkdir -p mt7621-output
        mv cloudflare_fallback mt7621-output/
        ls -lh mt7621-output/
    fi
fi