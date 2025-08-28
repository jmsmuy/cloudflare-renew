#include "../lib/publicip.h"

#include <stdio.h>
#include <stdlib.h>

// Test comment to trigger CI workflow

int main(void)
{
    char *ip = get_public_ip();
    if (ip) {
        printf("%s\n", ip);
        free(ip);
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to get public IP\n");
        return 1;
    }
}
