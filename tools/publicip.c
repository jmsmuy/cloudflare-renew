#include <stdio.h>
#include <stdlib.h>
#include "../lib/publicip.h"

int main(void) {
    char* ip = get_public_ip();
    if (ip) {
        printf("%s\n", ip);
        free(ip);
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to get public IP\n");
        return 1;
    }
}
