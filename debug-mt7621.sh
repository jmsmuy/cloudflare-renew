#!/bin/bash
# Debug build - incrementally add components to find the issue

echo "MT7621 Debug Build - Finding the problematic component"
echo "====================================================="
echo ""

mkdir -p debug-build

# Build incrementally to find which library causes issues
docker run --rm -v $(pwd):/work alpine:latest sh -c '
    echo "Setting up toolchain..."
    apk add --no-cache wget make >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    export CC="/tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc"
    export STRIP="/tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip"
    
    cd /work
    
    # Test 1: Just main program with stdio
    echo "Test 1: Main program only..."
    cat > test1.c << "EOF"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("Test 1: Basic main - OK\n");
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("Version 1.0\n");
    }
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test1 test1.c
    $STRIP debug-build/test1
    echo "  ✓ Built test1"
    
    # Test 2: Add JSON library
    echo "Test 2: Main + JSON library..."
    cat > test2.c << "EOF"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Minimal JSON test
struct json_value {
    int type;
    char *string_value;
};

struct json_value* json_parse(const char *input) {
    struct json_value *val = malloc(sizeof(struct json_value));
    if (val) {
        val->type = 1;
        val->string_value = strdup(input);
    }
    return val;
}

void json_free(struct json_value *val) {
    if (val) {
        if (val->string_value) free(val->string_value);
        free(val);
    }
}

int main(int argc, char *argv[]) {
    printf("Test 2: With JSON functions - ");
    
    struct json_value *test = json_parse("{\"test\": \"value\"}");
    if (test) {
        printf("OK\n");
        json_free(test);
    } else {
        printf("FAILED\n");
    }
    
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test2 test2.c
    $STRIP debug-build/test2
    echo "  ✓ Built test2"
    
    # Test 3: Add actual JSON library
    echo "Test 3: With real JSON library..."
    cat > test3.c << "EOF"
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[]) {
    printf("Test 3: With real JSON lib\n");
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test3 test3.c lib/json.c -I. 2>/dev/null || {
        echo "  ✗ Failed with real JSON library"
        # Try without optimization
        $CC -static -o debug-build/test3 test3.c lib/json.c -I. 2>/dev/null || {
            echo "  ✗ JSON library has issues"
        }
    }
    if [ -f debug-build/test3 ]; then
        $STRIP debug-build/test3
        echo "  ✓ Built test3"
    fi
    
    # Test 4: Test with pthread
    echo "Test 4: With pthread..."
    cat > test4.c << "EOF"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* thread_func(void* arg) {
    printf("Thread running\n");
    return NULL;
}

int main() {
    printf("Test 4: pthread test - ");
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_func, NULL) == 0) {
        pthread_join(thread, NULL);
        printf("OK\n");
    } else {
        printf("FAILED\n");
    }
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test4 test4.c -lpthread
    $STRIP debug-build/test4
    echo "  ✓ Built test4"
    
    # Test 5: Network functions
    echo "Test 5: With network functions..."
    cat > test5.c << "EOF"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    printf("Test 5: Network functions - ");
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        printf("OK\n");
        close(sock);
    } else {
        printf("FAILED\n");
    }
    
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test5 test5.c
    $STRIP debug-build/test5
    echo "  ✓ Built test5"
    
    # Test 6: Math operations that might use unsupported instructions
    echo "Test 6: Math operations..."
    cat > test6.c << "EOF"
#include <stdio.h>
#include <math.h>

int main() {
    printf("Test 6: Math operations - ");
    
    // Test division and multiplication
    int a = 1000;
    int b = 37;
    int c = a / b;
    int d = a * b;
    
    // Test floating point (might fail on soft-float issues)
    float f1 = 3.14159;
    float f2 = 2.71828;
    float f3 = f1 * f2;
    
    printf("OK (int: %d, float: %.2f)\n", c, f3);
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test6 test6.c -lm 2>/dev/null || {
        # Try without -lm
        $CC -static -Os -o debug-build/test6 test6.c 2>/dev/null || {
            echo "  ✗ Math operations failed"
        }
    }
    if [ -f debug-build/test6 ]; then
        $STRIP debug-build/test6
        echo "  ✓ Built test6"
    fi
    
    # Test 7: Build with actual cloudflare_renew.c but minimal libs
    echo "Test 7: Real main file, minimal libs..."
    cat > test7.c << "EOF"
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Stub functions to satisfy cloudflare_renew.c
int getip_main(int argc, char *argv[]) { return 0; }
int setip_main(int argc, char *argv[]) { return 0; }
char* get_public_ip(void) { return strdup("1.2.3.4"); }

int main(int argc, char *argv[]) {
    printf("Cloudflare Renew - Test Build\n");
    
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("Version 1.0-test\n");
        return 0;
    }
    
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [options]\n", argv[0]);
        return 0;
    }
    
    printf("Test 7: Real structure - OK\n");
    return 0;
}
EOF
    $CC -static -Os -o debug-build/test7 test7.c
    $STRIP debug-build/test7
    echo "  ✓ Built test7"
    
    # Clean up
    rm -f test*.c
'

echo ""
echo "Debug builds created:"
echo "===================="
ls -lh debug-build/

echo ""
echo "Testing Instructions:"
echo "===================="
echo "Copy ALL test binaries to your router and run them in order:"
echo ""
echo "scp debug-build/test* root@router:/tmp/"
echo "ssh root@router"
echo "cd /tmp"
echo "for i in 1 2 3 4 5 6 7; do"
echo "  echo \"Running test\$i:\""
echo "  chmod +x test\$i"
echo "  ./test\$i || echo \"FAILED with: \$?\""
echo "  echo \"\""
echo "done"
echo ""
echo "The first test that fails will tell us which component"
echo "is causing the illegal instruction error."