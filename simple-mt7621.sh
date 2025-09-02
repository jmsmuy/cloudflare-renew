#!/bin/bash
# Simplest working build for MT7621

echo "Simple MT7621 Build"
echo "==================="

# Clean up
rm -rf simple-output
mkdir -p simple-output

# Build the simplest test first
echo "Step 1: Building test binary..."
docker run --rm -v $(pwd):/work alpine:latest sh -c '
    apk add --no-cache wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    
    # Simple test
    cat > test.c << "EOF"
#include <stdio.h>
int main() { 
    printf("MT7621 works!\n"); 
    return 0; 
}
EOF
    
    ./mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -static -Os -o /work/simple-output/test test.c
    
    ./mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip \
        /work/simple-output/test
'

echo "Step 2: Building main program (without SSL for now)..."

# Create a minimal working version
cat > minimal_build.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("Cloudflare Renew - MT7621 Build\n");
    
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --help, -h    Show this help\n");
            printf("  --version     Show version\n");
            printf("\nNote: This is a test build for MT7621\n");
        } else if (strcmp(argv[1], "--version") == 0) {
            printf("Version 1.0-mt7621\n");
        }
    } else {
        printf("This is a test build. SSL support will be added.\n");
        printf("Use --help for options.\n");
    }
    
    return 0;
}
EOF

docker run --rm -v $(pwd):/work alpine:latest sh -c '
    apk add --no-cache wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    
    ./mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
        -static -Os -march=mips32r2 -mtune=24kc \
        -o /work/simple-output/cloudflare_minimal \
        /work/minimal_build.c
    
    ./mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip \
        /work/simple-output/cloudflare_minimal
'

# Try to build with actual source (no SSL)
echo "Step 3: Attempting full build without SSL..."

docker run --rm -v $(pwd):/work alpine:latest sh -c '
    apk add --no-cache wget >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    export CC="/tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc"
    
    cd /work
    
    # Try to compile just the JSON parser as a test
    $CC -static -Os -march=mips32r2 -mtune=24kc \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -c lib/json.c -o json.o 2>/dev/null
    
    if [ -f json.o ]; then
        echo "JSON module compiled OK"
        
        # Try to build a version with just JSON functionality
        cat > test_json.c << "EOF"
#include <stdio.h>
#include "lib/json.h"

int main() {
    printf("JSON test for MT7621\n");
    char *test = "{\"test\": \"value\"}";
    printf("Test JSON: %s\n", test);
    return 0;
}
EOF
        
        $CC -static -Os -march=mips32r2 -mtune=24kc \
            -I. -o simple-output/test_json \
            test_json.c json.o 2>/dev/null || \
            echo "JSON test build failed"
    fi
'

# Clean up temp files
rm -f minimal_build.c test_json.c json.o

echo ""
echo "Build complete!"
echo "=============="
ls -lh simple-output/ 2>/dev/null || echo "No files built"

echo ""
echo "To test on your MT7621 router:"
echo "=============================="
echo "1. Copy and test the basic binary:"
echo "   scp simple-output/test root@router:/tmp/"
echo "   ssh root@router '/tmp/test'"
echo ""
echo "   If this prints 'MT7621 works!' then the architecture is correct."
echo ""
echo "2. Test the minimal version:"
echo "   scp simple-output/cloudflare_minimal root@router:/tmp/"
echo "   ssh root@router '/tmp/cloudflare_minimal --help'"
echo ""
echo "If these work, we can proceed to build the full version with SSL support."