#!/bin/bash
# Find the right architecture flags for MT7621

echo "Finding correct architecture flags for MT7621"
echo "============================================="
echo ""

mkdir -p arch-test

# Create a simple test program that uses various features
cat > test_arch.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("Architecture test for MT7621\n");
    
    // Test some basic operations
    int a = 100;
    int b = 200;
    int c = a * b / 3;
    
    printf("Math test: %d * %d / 3 = %d\n", a, b, c);
    
    // Test memory allocation
    char *buf = malloc(1024);
    if (buf) {
        strcpy(buf, "Memory allocation works");
        printf("%s\n", buf);
        free(buf);
    }
    
    printf("All tests passed!\n");
    return 0;
}
EOF

echo "Building test binaries with different architecture flags..."
echo ""

# Test different architecture combinations
declare -a ARCH_FLAGS=(
    ""                                    # No flags (most compatible)
    "-march=mips32"                       # MIPS32 (older)
    "-march=mips2"                        # MIPS II (very old, very compatible)
    "-march=mips32 -mtune=mips32"        # MIPS32 with tuning
    "-march=24kc"                         # Specific 24kc
    "-march=mips32r2 -mno-mips16"        # MIPS32R2 without MIPS16
    "-march=mips32 -msoft-float"         # With soft float
)

declare -a ARCH_NAMES=(
    "generic"
    "mips32"
    "mips2"
    "mips32-tuned"
    "24kc"
    "mips32r2-no16"
    "mips32-soft"
)

# Build each variant
for i in "${!ARCH_FLAGS[@]}"; do
    FLAGS="${ARCH_FLAGS[$i]}"
    NAME="${ARCH_NAMES[$i]}"
    
    echo "Building variant: $NAME"
    echo "  Flags: ${FLAGS:-'(none)'}"
    
    docker run --rm -v $(pwd):/work alpine:latest sh -c "
        apk add --no-cache wget >/dev/null 2>&1
        cd /tmp
        wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
        tar xzf mipsel-linux-musl-cross.tgz
        
        ./mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc \
            -static -Os $FLAGS \
            -o /work/arch-test/test_$NAME \
            /work/test_arch.c 2>/dev/null
        
        if [ -f /work/arch-test/test_$NAME ]; then
            ./mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip \
                /work/arch-test/test_$NAME
            echo '  ✓ Built successfully'
        else
            echo '  ✗ Build failed'
        fi
    " 2>/dev/null || echo "  ✗ Docker error"
    
    echo ""
done

# Show results
echo "Test binaries created:"
echo "====================="
ls -lh arch-test/ | grep test_

echo ""
echo "Testing instructions:"
echo "===================="
echo "1. Copy ALL test binaries to your router:"
echo "   scp arch-test/test_* root@router:/tmp/"
echo ""
echo "2. Test each one on the router:"
echo "   ssh root@router"
echo "   cd /tmp"
echo "   for f in test_*; do"
echo '     echo "Testing $f:"'
echo '     chmod +x $f'
echo '     ./$f && echo "SUCCESS" || echo "FAILED"'
echo "     echo ""
echo "   done"
echo ""
echo "3. The one that prints 'All tests passed!' without errors"
echo "   tells us the correct architecture flags to use."
echo ""
echo "Most likely 'test_generic' or 'test_mips32' will work."

# Clean up
rm -f test_arch.c