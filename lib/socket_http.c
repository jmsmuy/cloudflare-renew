#define _POSIX_C_SOURCE 200809L
#include "socket_http.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Global SSL context
static SSL_CTX *ssl_ctx = NULL;
static bool ssl_initialized = false;

// Initialize OpenSSL
static int init_openssl(void)
{
    if (ssl_initialized) {
        return 0;
    }

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (!ssl_ctx) {
        return -1;
    }

    // Set verification mode (for production, you might want stricter verification)
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);

    ssl_initialized = true;
    return 0;
}

// Cleanup OpenSSL
static void cleanup_openssl(void)
{
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
    EVP_cleanup();
    ERR_free_strings();
    ssl_initialized = false;
}

// Public cleanup function for external use
// cppcheck-suppress unusedFunction
void http_cleanup(void)
{
    cleanup_openssl();
}

// Initialize an HTTP response structure
void http_response_init(struct http_response *response)
{
    response->data = NULL;
    response->size = 0;
    response->status_code = 0;
    response->success = false;
}

// Free memory allocated for HTTP response
void http_response_free(struct http_response *response)
{
    if (response && response->data) {
        free(response->data);
        response->data = NULL;
        response->size = 0;
        response->status_code = 0;
        response->success = false;
    }
}

// Add a header to the header list
struct http_header *http_header_add(struct http_header *headers, const char *name, const char *value)
{
    struct http_header *new_header = malloc(sizeof(struct http_header));
    if (!new_header) {
        return headers;
    }

    new_header->name = strdup(name);
    new_header->value = strdup(value);
    new_header->next = headers;

    if (!new_header->name || !new_header->value) {
        free(new_header->name);
        free(new_header->value);
        free(new_header);
        return headers;
    }

    return new_header;
}

// Free all headers in the list
void http_headers_free(struct http_header *headers)
{
    while (headers) {
        struct http_header *next = headers->next;
        free(headers->name);
        free(headers->value);
        free(headers);
        headers = next;
    }
}

// Helper function to parse URL into components
// cppcheck-suppress staticFunction
static int
parse_url(const char *url, char *host, size_t host_size, int *port, char *path, size_t path_size, bool *is_https)
{
    if (!url || !host || !port || !path || !is_https) {
        return -1;
    }

    // Initialize defaults
    *port = 80;
    *is_https = false;
    strncpy(path, "/", path_size - 1);
    path[path_size - 1] = '\0';

    // Check for https://
    if (strncmp(url, "https://", 8) == 0) {
        *is_https = true;
        *port = 443;
        url += 8;
    } else if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    } else {
        return -1; // Invalid protocol
    }

    // Find host:port/path
    const char *host_start = url;
    const char *host_end = strchr(url, '/');
    const char *port_start = strchr(url, ':');

    if (host_end) {
        // Copy path
        strncpy(path, host_end, path_size - 1);
        path[path_size - 1] = '\0';
    }

    if (port_start && (!host_end || port_start < host_end)) {
        // Extract host
        size_t host_len = port_start - host_start;
        if (host_len >= host_size) {
            return -1;
        }
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';

        // Extract port
        const char *port_end = host_end ? host_end : port_start + strlen(port_start);
        char port_str[16];
        size_t port_len = port_end - (port_start + 1);
        if (port_len >= sizeof(port_str)) {
            return -1;
        }
        strncpy(port_str, port_start + 1, port_len);
        port_str[port_len] = '\0';
        *port = atoi(port_str);
    } else {
        // No port specified, extract host
        size_t host_len = host_end ? (host_end - host_start) : strlen(host_start);
        if (host_len >= host_size) {
            return -1;
        }
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';
    }

    return 0;
}

// Helper function to build HTTP request string
// cppcheck-suppress staticFunction
static char *build_http_request(const char *host,
                                const char *path,
                                http_method_t method,
                                const char *body,
                                struct http_header *headers)
{
    // Calculate total size needed
    size_t total_size = 1024; // Base size for request line and basic headers
    if (body) {
        total_size += strlen(body);
    }

    // Add header sizes
    struct http_header *h = headers;
    while (h) {
        total_size += strlen(h->name) + strlen(h->value) + 4; // ": \r\n"
        h = h->next;
    }

    char *request = malloc(total_size);
    if (!request) {
        return NULL;
    }

    // Build request line
    const char *method_str = "GET";
    switch (method) {
        case HTTP_GET:
            method_str = "GET";
            break;
        case HTTP_POST:
            method_str = "POST";
            break;
        case HTTP_PUT:
            method_str = "PUT";
            break;
        case HTTP_DELETE:
            method_str = "DELETE";
            break;
    }

    int pos = snprintf(request, total_size, "%s %s HTTP/1.1\r\n", method_str, path);

    // Add Host header
    pos += snprintf(request + pos, total_size - pos, "Host: %s\r\n", host);

    // Add custom headers
    h = headers;
    while (h && pos < (int) (total_size - 10)) {
        pos += snprintf(request + pos, total_size - pos, "%s: %s\r\n", h->name, h->value);
        h = h->next;
    }

    // Add Content-Length if there's a body
    if (body) {
        pos += snprintf(request + pos, total_size - pos, "Content-Length: %zu\r\n", strlen(body));
    }

    // End headers
    pos += snprintf(request + pos, total_size - pos, "\r\n");

    // Add body if present
    if (body) {
        strncpy(request + pos, body, total_size - pos - 1);
        request[total_size - 1] = '\0';
    }

    return request;
}

// Perform HTTP request using POSIX sockets
int http_request(const char *url,
                 http_method_t method,
                 const char *body,
                 struct http_header *headers,
                 struct http_response *response)
{
    if (!url || !response) {
        return -1;
    }

    // HTTP request starting

    char host[256];
    char path[1024];
    int port;
    bool is_https;
    int sockfd = -1;
    int result = -1;

    // Parse URL
    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path), &is_https) != 0) {
        return -1;
    }

    // Initialize OpenSSL if needed for HTTPS
    if (is_https && init_openssl() != 0) {
        return -1;
    }

    // SSL initialized

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    // Set socket timeout (5 seconds)
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }

    // Get server address
    const struct hostent *server = gethostbyname(host);
    if (!server) {
        close(sockfd);
        return -1;
    }

    // Setup server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }

    // Setup SSL connection if HTTPS
    SSL *ssl = NULL;
    if (is_https) {
        ssl = SSL_new(ssl_ctx);
        if (!ssl) {
            close(sockfd);
            return -1;
        }

        if (SSL_set_fd(ssl, sockfd) != 1) {
            SSL_free(ssl);
            close(sockfd);
            return -1;
        }

        if (SSL_connect(ssl) != 1) {
            SSL_free(ssl);
            close(sockfd);
            return -1;
        }
    }

    // Build HTTP request
    char *http_request = build_http_request(host, path, method, body, headers);
    if (!http_request) {
        if (ssl)
            SSL_free(ssl);
        close(sockfd);
        return -1;
    }

    // Send request
    size_t request_len = strlen(http_request);
    ssize_t sent;
    if (ssl) {
        sent = SSL_write(ssl, http_request, request_len);
    } else {
        sent = send(sockfd, http_request, request_len, 0);
    }
    free(http_request);

    if (sent != (ssize_t) request_len) {
        if (ssl)
            SSL_free(ssl);
        close(sockfd);
        return -1;
    }

    // Receive response
    char buffer[4096];
    size_t total_received = 0;
    char *response_data = NULL;

    while (1) {
        ssize_t received;
        if (ssl) {
            received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        } else {
            received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        }

        if (received <= 0) {
            // Check if this is a clean connection close or an error
            if (ssl) {
                int ssl_error = SSL_get_error(ssl, received);
                if (ssl_error == SSL_ERROR_ZERO_RETURN) {
                    // SSL connection closed cleanly
                } else {
                    // SSL error
                }
            }
            break;
        }

        // Reallocate response buffer
        char *new_data = realloc(response_data, total_received + received + 1);
        if (!new_data) {
            free(response_data);
            if (ssl)
                SSL_free(ssl);
            close(sockfd);
            return -1;
        }
        response_data = new_data;

        // Copy new data directly (don't null-terminate buffer)
        memcpy(response_data + total_received, buffer, received);
        total_received += received;

        // For now, just receive all available data without early termination
        // TODO: Implement proper response completion detection
    }

    // Null-terminate the response
    if (response_data) {
        response_data[total_received] = '\0';
    }

    // Response received

    if (ssl)
        SSL_free(ssl);
    close(sockfd);

    if (!response_data) {
        return -1;
    }

    // Response received successfully

    // Parse HTTP status code (use a copy to avoid modifying original)
    char *response_copy = strdup(response_data);
    if (response_copy) {
        const char *status_line = strtok(response_copy, "\r\n");
        if (status_line) {
            // Find status code in "HTTP/1.1 200 OK" format
            char *status_start = strchr(status_line, ' ');
            if (status_start) {
                status_start++; // Skip space
                char *status_end = strchr(status_start, ' ');
                if (status_end) {
                    *status_end = '\0';
                    response->status_code = atoi(status_start);
                }
            }
        }
        free(response_copy);
    }

    // Find start of response body (after \r\n\r\n or \n\n)
    char *body_start = strstr(response_data, "\r\n\r\n");
    if (!body_start) {
        body_start = strstr(response_data, "\n\n");
        if (body_start) {
            body_start += 2; // Skip \n\n
        }
    } else {
        body_start += 4; // Skip \r\n\r\n
    }

    if (body_start) {
        // Check if response uses chunked encoding
        bool is_chunked = (strstr(response_data, "Transfer-Encoding: chunked") != NULL);

        if (is_chunked) {
            // Handle chunked encoding
            char *unchunked_data = NULL;
            size_t unchunked_size = 0;
            char *chunk_ptr = body_start;

            while (*chunk_ptr) {
                // Read chunk size (hex)
                char *chunk_size_end = strstr(chunk_ptr, "\r\n");
                if (!chunk_size_end)
                    break;

                *chunk_size_end = '\0';
                long chunk_size = strtol(chunk_ptr, NULL, 16);
                *chunk_size_end = '\r';

                if (chunk_size == 0)
                    break; // End of chunks

                // Move to chunk data
                char *chunk_data = chunk_size_end + 2;

                // Reallocate buffer for unchunked data
                char *new_data = realloc(unchunked_data, unchunked_size + chunk_size + 1);
                if (!new_data) {
                    free(unchunked_data);
                    free(response_data);
                    return -1;
                }
                unchunked_data = new_data;

                // Copy chunk data
                memcpy(unchunked_data + unchunked_size, chunk_data, chunk_size);
                unchunked_size += chunk_size;

                // Move to next chunk
                chunk_ptr = chunk_data + chunk_size + 2; // Skip chunk data + \r\n
            }

            if (unchunked_data) {
                unchunked_data[unchunked_size] = '\0';
                response->data = unchunked_data;
                response->size = unchunked_size;
                response->success = (response->status_code >= 200 && response->status_code < 300);
                result = 0;
            }
        } else {
            // Normal response (not chunked)
            size_t body_size = strlen(body_start);

            // Allocate new buffer for just the body
            response->data = malloc(body_size + 1);
            if (response->data) {
                strcpy(response->data, body_start);
                response->size = body_size;
                response->success = (response->status_code >= 200 && response->status_code < 300);
                result = 0;
            }
        }
    } else {
        // No body separator found
    }

    free(response_data);
    return result;
}
