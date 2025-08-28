#include "../lib/getip.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <config_file> <token_file> [domain_name]\n", argv[0]);
        fprintf(stderr, "Example: %s cloudflare.conf cloudflare.token\n", argv[0]);
        fprintf(stderr, "         %s cloudflare.conf cloudflare.token jmsmuy.com\n", argv[0]);
        return 1;
    }

    const char *domain_name = (argc == 4) ? argv[3] : NULL;
    char *ip_address = get_cloudflare_ip(argv[1], argv[2], domain_name);

    if (ip_address) {
        printf("%s\n", ip_address);
        free(ip_address);
        return 0;
    }

    fprintf(stderr, "Failed to get IP from Cloudflare\n");
    return 1;
}
