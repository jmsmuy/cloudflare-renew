#!/bin/bash
# Build the actual program incrementally

echo "Incremental Build for MT7621"
echo "============================"
echo ""

mkdir -p incremental

docker run --rm -v $(pwd):/work alpine:latest sh -c '
    echo "Setting up toolchain..."
    apk add --no-cache wget make >/dev/null 2>&1
    cd /tmp
    wget -q https://musl.cc/mipsel-linux-musl-cross.tgz
    tar xzf mipsel-linux-musl-cross.tgz
    export CC="/tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-gcc"
    export STRIP="/tmp/mipsel-linux-musl-cross/bin/mipsel-linux-musl-strip"
    
    cd /work
    
    # Version 1: Just cloudflare_renew.c with stubs
    echo "Version 1: Main only with stubs..."
    cat > stubs.c << "EOF"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Stub all the library functions
void http_response_init(void *r) {}
void http_response_free(void *r) {}
void http_cleanup(void) {}
int http_request(const void *a, int b, const void *c, void *d, void *e) { return 0; }
void* http_header_add(void *h, const char *n, const char *v) { return h; }
void http_headers_free(void *h) {}

// Stub JSON functions
void* json_parse(const char *s) { return NULL; }
void json_free(void *j) {}
void* json_get(void *j, const char *k) { return NULL; }
const char* json_string(void *j) { return ""; }
int json_bool(void *j) { return 0; }

// Stub other functions
int getip_main(int argc, char **argv) { 
    printf("getip_main called\n");
    return 0; 
}
int setip_main(int argc, char **argv) { 
    printf("setip_main called\n");
    return 0; 
}
char* get_public_ip(void) { 
    return strdup("127.0.0.1"); 
}

// Main program
int main(int argc, char **argv) {
    printf("Cloudflare Renew v1 - Stubs only\n");
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("1.0-stub\n");
    }
    return 0;
}
EOF
    
    $CC -static -Os -o incremental/cf_v1_stubs stubs.c
    $STRIP incremental/cf_v1_stubs
    echo "  ✓ Built v1 (stubs)"
    
    # Version 2: Add JSON library only
    echo "Version 2: Main + JSON..."
    $CC -static -Os -o incremental/cf_v2_json \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -I. \
        stubs.c lib/json.c 2>/dev/null || {
            echo "  ✗ JSON library causes issues"
            # Try without optimization
            $CC -static -o incremental/cf_v2_json \
                -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
                -I. \
                stubs.c lib/json.c 2>/dev/null || {
                    echo "  ✗ JSON library failed completely"
                }
        }
    
    if [ -f incremental/cf_v2_json ]; then
        $STRIP incremental/cf_v2_json
        echo "  ✓ Built v2 (with JSON)"
    fi
    
    # Version 3: Try the actual main with minimal dependencies
    echo "Version 3: Simplified real program..."
    
    # Create simplified versions of required files
    cat > simple_main.c << "EOF"
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CONFIG_FILE "cloudflare.conf"
#define TOKEN_FILE "cloudflare.token"
#define LAST_IP_FILE "last.ip"
#define LOG_FILE "cloudflare.log"

static void write_log(const char *message) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    
    fprintf(log, "[%s] %s\n", timestamp, message);
    fclose(log);
}

static char *read_ip_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;
    
    char *ip = malloc(64);
    if (!ip) {
        fclose(file);
        return NULL;
    }
    
    if (fgets(ip, 64, file) == NULL) {
        free(ip);
        fclose(file);
        return NULL;
    }
    
    // Remove newline
    ip[strcspn(ip, "\n")] = 0;
    fclose(file);
    return ip;
}

static void write_ip_to_file(const char *filename, const char *ip) {
    FILE *file = fopen(filename, "w");
    if (!file) return;
    fprintf(file, "%s\n", ip);
    fclose(file);
}

// Stub function
char* get_public_ip(void) {
    return strdup("1.2.3.4");
}

int main(int argc, char *argv[]) {
    printf("Cloudflare DNS Updater\n");
    
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            printf("Version 1.0-simplified\n");
            return 0;
        }
        if (strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [--help] [--version]\n", argv[0]);
            return 0;
        }
    }
    
    // Simple test of core functionality
    write_log("Starting cloudflare_renew");
    
    char *current_ip = get_public_ip();
    if (current_ip) {
        printf("Current IP: %s\n", current_ip);
        write_ip_to_file(LAST_IP_FILE, current_ip);
        free(current_ip);
    }
    
    write_log("Cloudflare_renew completed");
    printf("Done.\n");
    
    return 0;
}
EOF
    
    $CC -static -Os -o incremental/cf_v3_simple simple_main.c
    $STRIP incremental/cf_v3_simple
    echo "  ✓ Built v3 (simplified main)"
    
    # Version 4: Try with -O0 (no optimization)
    echo "Version 4: No optimization..."
    $CC -static -O0 \
        -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
        -o incremental/cf_v4_noopt \
        simple_main.c
    $STRIP incremental/cf_v4_noopt
    echo "  ✓ Built v4 (no optimization)"
    
    # Clean up
    rm -f stubs.c simple_main.c
'

echo ""
echo "Incremental builds created:"
echo "==========================="
ls -lh incremental/

echo ""
echo "Test these on your router:"
echo "=========================="
echo "scp incremental/* root@router:/tmp/"
echo ""
echo "ssh root@router"
echo "cd /tmp"
echo ""
echo "# Test each version:"
echo "./cf_v1_stubs --version        # Should work"
echo "./cf_v2_json --version         # Tests JSON library"
echo "./cf_v3_simple --version       # Simplified main"
echo "./cf_v4_noopt --version        # No optimization"
echo ""
echo "The first one that fails tells us the problem!"