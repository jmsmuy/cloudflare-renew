#define _POSIX_C_SOURCE 200809L
#include "publicip.h"

#include "socket_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to trim whitespace
static char *trim_whitespace(char *str)
{
    char *end = NULL;

    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
        str++;

    if (*str == 0)
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        end--;

    // Write new null terminator
    end[1] = '\0';

    return str;
}

// Get public IP from ipinfo.io
char *get_public_ip(void)
{
    struct http_response response;
    char *result = NULL;

    http_response_init(&response);

    int http_result = http_request("https://ipinfo.io/ip", HTTP_GET, NULL, NULL, &response);

    if (http_result == 0 && response.success && response.data && response.size > 0) {
        // Trim any whitespace/newlines from the response
        const char *trimmed_ip = trim_whitespace(response.data);
        if (trimmed_ip && strlen(trimmed_ip) > 0) {
            result = strdup(trimmed_ip);
        }
    }

    http_response_free(&response);
    return result;
}
