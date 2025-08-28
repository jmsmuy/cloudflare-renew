#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <stddef.h>

// Memory structure for HTTP response data
struct http_response {
    char *data;
    size_t size;
};

// Initialize an HTTP response structure
void http_response_init(struct http_response *response);

// Free memory allocated for HTTP response
void http_response_free(struct http_response *response);

// libcurl write callback for HTTP responses
size_t http_write_callback(void *data, size_t size, size_t nmemb, void *userp);

#endif // HTTP_UTILS_H
