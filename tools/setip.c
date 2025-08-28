#include <stdio.h>
#include <stdlib.h>
#include "../lib/setip.h"

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s <config_file> <token_file> <ip_address> [domain_name]\n", argv[0]);
        fprintf(stderr, "Example: %s cloudflare.conf cloudflare.token 199.99.99.99\n", argv[0]);
        fprintf(stderr, "         %s cloudflare.conf cloudflare.token 199.99.99.99 jmsmuy.com\n", argv[0]);
        return 1;
    }
    
    const char* config_file = argv[1];
    const char* token_file = argv[2];
    const char* ip_address = argv[3];
    const char* domain_name = (argc == 5) ? argv[4] : NULL;
    
    int result = set_cloudflare_ip(config_file, token_file, ip_address, domain_name);
    
    if (result == 0) {
        printf("✅ IP successfully updated to %s\n", ip_address);
        return 0;
    } else {
        printf("❌ Failed to update IP\n");
        return 1;
    }
}