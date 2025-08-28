#include "../lib/publicip.h"

#include <stdio.h>
#include <stdlib.h>

// Test comment to trigger CI workflow

int main(void)
{
    char *ip_address = get_public_ip();
    if (ip_address) {
        printf("%s\n", ip_address);
        free(ip_address);
        return 0;
    }

    fprintf(stderr, "Error: Failed to get public IP\n");
    return 1;
}
