#!/bin/bash
# Build script with multiple architecture options to fix segfault
# This creates builds for different MIPS variants

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Multi-Architecture MIPSEL Build${NC}"
echo "================================"
echo ""

# Create output directory
mkdir -p builds

# Function to build with Docker
build_variant() {
    local VARIANT=$1
    local ARCH_FLAGS=$2
    local LIBC=$3
    local DESC=$4
    
    echo -e "${GREEN}Building variant: $VARIANT${NC}"
    echo "  Architecture: $ARCH_FLAGS"
    echo "  C Library: $LIBC"
    echo "  Description: $DESC"
    
    # Create variant-specific Dockerfile
    if [ "$LIBC" = "musl" ]; then
        cat > Dockerfile.$VARIANT << EOF
FROM alpine:latest
RUN apk add --no-cache build-base wget make linux-headers
RUN wget https://musl.cc/mipsel-linux-musl-cross.tgz && \
    tar xzf mipsel-linux-musl-cross.tgz
ENV PATH="/mipsel-linux-musl-cross/bin:\${PATH}"
COPY . /src
WORKDIR /src
RUN mipsel-linux-musl-gcc $ARCH_FLAGS -static -Os -o cloudflare_renew \
    cloudflare_renew.c lib/*.c -lpthread && \
    mipsel-linux-musl-strip cloudflare_renew
CMD cp cloudflare_renew /output/cloudflare_renew.$VARIANT
EOF
    else
        cat > Dockerfile.$VARIANT << EOF
FROM debian:bullseye
RUN apt-get update && apt-get install -y \
    gcc-mipsel-linux-gnu libc6-dev-mipsel-cross build-essential
COPY . /src
WORKDIR /src
RUN mipsel-linux-gnu-gcc $ARCH_FLAGS -Os -o cloudflare_renew \
    cloudflare_renew.c lib/*.c -lpthread -static-libgcc && \
    mipsel-linux-gnu-strip cloudflare_renew
CMD cp cloudflare_renew /output/cloudflare_renew.$VARIANT
EOF
    fi
    
    # Build
    docker build -t mipsel-$VARIANT -f Dockerfile.$VARIANT . >/dev/null 2>&1
    docker run --rm -v $(pwd)/builds:/output mipsel-$VARIANT
    
    if [ -f builds/cloudflare_renew.$VARIANT ]; then
        echo -e "  ${GREEN}✓ Built successfully${NC}"
    else
        echo -e "  ${RED}✗ Build failed${NC}"
    fi
    echo ""
}

# Build different variants
echo -e "${YELLOW}Building multiple variants to avoid segfault...${NC}"
echo ""

# Variant 1: MIPS32 with musl (most compatible with OpenWrt)
build_variant "mips32-musl" \
    "-march=mips32 -msoft-float" \
    "musl" \
    "Most compatible with older OpenWrt"

# Variant 2: MIPS32R2 with musl
build_variant "mips32r2-musl" \
    "-march=mips32r2" \
    "musl" \
    "For newer OpenWrt routers"

# Variant 3: Generic MIPS with minimal flags
build_variant "generic-musl" \
    "" \
    "musl" \
    "Generic build, maximum compatibility"

# Variant 4: MIPS32 with glibc
build_variant "mips32-glibc" \
    "-march=mips32" \
    "glibc" \
    "For routers using glibc"

# Clean up Dockerfiles
rm -f Dockerfile.mips32-* Dockerfile.generic-*

# Show results
echo -e "${BLUE}Build Results:${NC}"
echo "=============="
ls -lh builds/ | grep cloudflare_renew || echo "No builds succeeded"

echo ""
echo -e "${YELLOW}Testing Instructions:${NC}"
echo "1. First, run diagnostics on your router:"
echo "   scp diagnose-router.sh root@router:/tmp/"
echo "   ssh root@router 'sh /tmp/diagnose-router.sh'"
echo ""
echo "2. Copy ALL variants to router and test:"
echo "   scp builds/* root@router:/tmp/"
echo ""
echo "3. On the router, test each variant:"
echo "   cd /tmp"
echo "   for f in cloudflare_renew.*; do"
echo "     echo \"Testing \$f:\""
echo "     chmod +x \$f"
echo "     ./\$f --version || echo \"Failed\""
echo "   done"
echo ""
echo "4. Use the variant that works!"
echo ""
echo -e "${GREEN}Tip:${NC} The 'generic-musl' variant is most likely to work."