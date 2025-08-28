#define _POSIX_C_SOURCE 200809L
#include "publicip.h"

#include "http_utils.h"

#include <curl/curl.h>
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
    CURL *curl = NULL;
    CURLcode res;
    struct http_response response;
    char *result = NULL;

    http_response_init(&response);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://ipinfo.io/ip");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK && response.data && response.size > 0) {
            // Trim any whitespace/newlines from the response
            const char *trimmed_ip = trim_whitespace(response.data);
            if (trimmed_ip && strlen(trimmed_ip) > 0) {
                result = strdup(trimmed_ip);
            }
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    http_response_free(&response);
    return result;
}
