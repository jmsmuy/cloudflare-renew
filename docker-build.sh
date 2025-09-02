#!/bin/bash
# Docker-based build script for MIPSEL
# This handles all dependencies correctly without local toolchain issues

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Docker-based MIPSEL Cross-Compilation${NC}"
echo "======================================"
echo ""

# Check Docker
if ! command -v docker &> /dev/null; then
    echo -e "${RED}Docker is not installed!${NC}"
    echo "Please install Docker first:"
    echo "  https://docs.docker.com/get-docker/"
    exit 1
fi

# Build Docker image
echo -e "${GREEN}Step 1: Building Docker image with MIPSEL toolchain...${NC}"
echo "This will:"
echo "  - Install MIPSEL cross-compiler"
echo "  - Build OpenSSL for MIPSEL"
echo "  - Set up build environment"
echo ""

docker build -t cloudflare-mipsel-builder -f Dockerfile.mipsel . || {
    echo -e "${RED}Docker build failed!${NC}"
    exit 1
}

# Run build in container
echo -e "\n${GREEN}Step 2: Building cloudflare-renew in container...${NC}"

docker run --rm -v $(pwd)/output:/output cloudflare-mipsel-builder sh -c "
    make -f Makefile.container && \
    cp cloudflare_renew tools/getip tools/setip tools/publicip /output/ && \
    echo 'Build successful!'
" || {
    echo -e "${RED}Build failed!${NC}"
    exit 1
}

# Check output
echo -e "\n${GREEN}Step 3: Checking build output...${NC}"

if [ ! -d output ]; then
    echo -e "${RED}Output directory not found!${NC}"
    exit 1
fi

echo "Built binaries:"
ls -lh output/ | grep -v "^total"

echo ""
echo "Binary info:"
file output/cloudflare_renew

# Create package
echo -e "\n${GREEN}Step 4: Creating distribution package...${NC}"
cd output
tar czf ../cloudflare-renew-mipsel-docker.tar.gz *
cd ..

echo -e "${GREEN}✓ Package created: cloudflare-renew-mipsel-docker.tar.gz${NC}"

# Done
echo -e "\n${BLUE}═══════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
echo ""
echo "The binaries are in the 'output' directory:"
ls output/
echo ""
echo "To deploy to your router:"
echo "  scp output/* root@router:/usr/bin/"
echo ""
echo "Or use the tarball:"
echo "  scp cloudflare-renew-mipsel-docker.tar.gz root@router:/tmp/"