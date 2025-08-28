#ifndef SOCKET_HTTP_H
#define SOCKET_HTTP_H

#include <stddef.h>
#include <stdbool.h>

// HTTP method types
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} http_method_t;

// HTTP response structure
struct http_response {
    char *data;
    size_t size;
    int status_code;
    bool success;
};

// HTTP header structure
struct http_header {
    char *name;
    char *value;
    struct http_header *next;
};

// Initialize an HTTP response structure
void http_response_init(struct http_response *response);

// Free memory allocated for HTTP response
void http_response_free(struct http_response *response);

// Add a header to the header list
struct http_header *http_header_add(struct http_header *headers, const char *name, const char *value);

// Free all headers in the list
void http_headers_free(struct http_header *headers);

// Perform HTTP request using POSIX sockets
int http_request(const char *url, http_method_t method, const char *body, 
                struct http_header *headers, struct http_response *response);

// Helper function to parse URL into components
int parse_url(const char *url, char *host, size_t host_size, int *port, 
              char *path, size_t path_size, bool *is_https);

// Helper function to build HTTP request string
char *build_http_request(const char *host, const char *path, http_method_t method,
                        const char *body, struct http_header *headers);

// Cleanup function for OpenSSL resources
void http_cleanup(void);

#endif // SOCKET_HTTP_H
