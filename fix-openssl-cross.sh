#!/bin/bash
# Script to fix OpenSSL cross-compilation issues on Debian/Ubuntu
# Downloads and sets up OpenSSL headers and libraries for MIPSEL

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Fixing OpenSSL Cross-Compilation for MIPSEL${NC}"
echo "============================================"
echo ""

# Create directories
mkdir -p mipsel-libs/include
mkdir -p mipsel-libs/lib

echo -e "${GREEN}Option 1: Quick Fix - Download Pre-built MIPSEL OpenSSL${NC}"
echo "Downloading OpenSSL headers and libraries for MIPSEL..."

# Download OpenSSL headers (architecture independent)
if [ ! -d mipsel-libs/include/openssl ]; then
    echo "Downloading OpenSSL headers..."
    wget -q --show-progress -O - \
        https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1w.tar.gz | \
        tar xz --strip-components=2 -C mipsel-libs/include \
        openssl-OpenSSL_1_1_1w/include/
    echo -e "${GREEN}✓ Headers downloaded${NC}"
fi

# Create a working Makefile
cat > Makefile.working << 'EOF'
# Working MIPSEL Makefile with local OpenSSL headers
CROSS_COMPILE = mipsel-linux-gnu-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip

ARCH_FLAGS = -march=mips32r2

# Use our downloaded headers
CFLAGS = -Wall -Wextra -std=c99 -Os $(ARCH_FLAGS) \
         -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
         -I./mipsel-libs/include \
         -I./lib

LDFLAGS = -Wl,--gc-sections

# Link dynamically (router must have OpenSSL)
LIBS = -lssl -lcrypto -lpthread -ldl

LIBDIR = lib
LIB_SOURCES = $(LIBDIR)/json.c \
              $(LIBDIR)/cloudflare_utils.c \
              $(LIBDIR)/socket_http.c \
              $(LIBDIR)/publicip.c \
              $(LIBDIR)/getip.c \
              $(LIBDIR)/setip.c

LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
PROGRAMS = cloudflare_renew tools/getip tools/setip tools/publicip

all: $(PROGRAMS)
	@echo "✓ Build complete!"
	@echo "Note: Binaries are dynamically linked."
	@echo "      Your router needs OpenSSL libraries installed."

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libcloudflare.a: $(LIB_OBJECTS)
	$(AR) rcs $@ $^

cloudflare_renew: cloudflare_renew.o libcloudflare.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L. -lcloudflare $(LIBS)

tools/getip: tools/getip.o libcloudflare.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L. -lcloudflare $(LIBS)

tools/setip: tools/setip.o libcloudflare.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L. -lcloudflare $(LIBS)

tools/publicip: tools/publicip.o libcloudflare.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L. -lcloudflare $(LIBS)

clean:
	rm -f $(PROGRAMS) $(LIB_OBJECTS) *.o tools/*.o libcloudflare.a

strip: $(PROGRAMS)
	$(STRIP) $(PROGRAMS)
EOF

echo -e "\n${GREEN}Option 2: Build with downloaded headers${NC}"
echo "Now you can build with:"
echo "  make -f Makefile.working clean"
echo "  make -f Makefile.working"
echo "  make -f Makefile.working strip"

echo -e "\n${GREEN}Option 3: Use Docker (most reliable)${NC}"
echo "If the above doesn't work, use Docker:"
echo "  chmod +x docker-build.sh"
echo "  ./docker-build.sh"

echo -e "\n${YELLOW}Testing the build...${NC}"
make -f Makefile.working clean >/dev/null 2>&1
if make -f Makefile.working all 2>/dev/null; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo ""
    echo "Binaries created:"
    ls -lh cloudflare_renew tools/getip tools/setip tools/publicip 2>/dev/null | awk '{print "  " $NF ": " $5}'
    echo ""
    echo "To strip binaries (reduce size):"
    echo "  make -f Makefile.working strip"
else
    echo -e "${YELLOW}Build failed. Trying alternative approach...${NC}"
    
    # Alternative: Build without SSL
    echo -e "\n${GREEN}Building without SSL support (HTTP only)...${NC}"
    cat > Makefile.nossl << 'EOF'
# MIPSEL build without SSL (HTTP only)
CROSS_COMPILE = mipsel-linux-gnu-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -Wall -Wextra -std=c99 -Os -march=mips32r2 \
         -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
         -DNO_SSL_SUPPORT

LIBS = -lpthread

LIBDIR = lib
LIB_SOURCES = $(LIBDIR)/json.c $(LIBDIR)/cloudflare_utils.c \
              $(LIBDIR)/socket_http.c $(LIBDIR)/publicip.c \
              $(LIBDIR)/getip.c $(LIBDIR)/setip.c

all: cloudflare_renew tools/getip tools/setip tools/publicip
	$(STRIP) $^
	@echo "Built without SSL (HTTP only)"

cloudflare_renew: cloudflare_renew.c $(LIB_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

tools/getip: tools/getip.c $(LIB_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

tools/setip: tools/setip.c $(LIB_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

tools/publicip: tools/publicip.c $(LIB_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f cloudflare_renew tools/getip tools/setip tools/publicip
EOF
    
    echo "Try building without SSL:"
    echo "  make -f Makefile.nossl"
fi