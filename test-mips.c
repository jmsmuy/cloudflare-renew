/* Simple test program to verify MIPS cross-compilation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("MIPS Test Program\n");
    printf("=================\n");
    
    /* Basic info */
    printf("Program: %s\n", argv[0]);
    printf("PID: %d\n", getpid());
    
    /* Architecture detection */
    #ifdef __mips__
        printf("Architecture: MIPS\n");
    #endif
    
    #ifdef __MIPSEL__
        printf("Endianness: Little Endian (MIPSEL)\n");
    #elif defined(__MIPSEB__)
        printf("Endianness: Big Endian (MIPSEB)\n");
    #endif
    
    #ifdef __mips_soft_float
        printf("Float ABI: Soft Float\n");
    #else
        printf("Float ABI: Hard Float\n");
    #endif
    
    /* Memory allocation test */
    printf("\nMemory test:\n");
    char *buf = malloc(1024);
    if (buf) {
        strcpy(buf, "Memory allocation successful");
        printf("  %s\n", buf);
        free(buf);
    } else {
        printf("  Memory allocation failed!\n");
    }
    
    printf("\nIf you see this, basic execution works!\n");
    return 0;
}