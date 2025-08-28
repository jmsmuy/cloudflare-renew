#include "http_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize an HTTP response structure
void http_response_init(struct http_response *response)
{
    response->data = NULL;
    response->size = 0;
}

// Free memory allocated for HTTP response
void http_response_free(struct http_response *response)
{
    if (response && response->data) {
        free(response->data);
        response->data = NULL;
        response->size = 0;
    }
}

// libcurl write callback for HTTP responses
size_t http_write_callback(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct http_response *response = (struct http_response *) userp;

    char *ptr = realloc(response->data, response->size + realsize + 1);
    if (ptr == NULL) {
        return 0; // out of memory
    }

    response->data = ptr;
    memcpy(&(response->data[response->size]), data, realsize);
    response->size += realsize;
    response->data[response->size] = 0; // null terminate

    return realsize;
}
