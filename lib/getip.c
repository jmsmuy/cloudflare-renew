#define _POSIX_C_SOURCE 200809L
#include "getip.h"

#include "cloudflare_utils.h"
#include "socket_http.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to extract IP from JSON response
static char *extract_ip_from_json(const char *json_text)
{
    struct json_root *root = parse_json(json_text);
    if (!root) {
        return NULL;
    }

    // Use the recursive search API to find all "content" values
    int count = 0;
    char **content_values = get_string_values(root, "content", &count);

    char *ip_address = NULL;
    if (content_values && count > 0) {
        ip_address = strdup(content_values[0]);

        // Free the array of strings
        for (int i = 0; i < count; i++) {
            free(content_values[i]);
        }
        free((void *) content_values);
    }

    free(root);
    return ip_address;
}

// Get IP from Cloudflare DNS
char *get_cloudflare_ip(const char *config_file, const char *token_file, const char *domain_name)
{
    cloudflare_config_t *config = load_cloudflare_config(config_file, token_file);
    if (!config) {
        return NULL;
    }

    // Determine which entry to use
    const cloudflare_entry_t *entry = NULL;
    if (domain_name) {
        entry = find_entry_by_domain(config, domain_name);
    } else {
        entry = get_entry_by_index(config, 0);
    }

    if (!entry) {
        free_cloudflare_config(config);
        return NULL;
    }

    struct http_response response;
    char *result = NULL;

    http_response_init(&response);

    // Build headers
    struct http_header *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", config->cloudflare_token);
    headers = http_header_add(headers, "Authorization", auth_header);
    headers = http_header_add(headers, "Content-Type", "application/json");

    char api_url[1024];
    build_cloudflare_dns_url(api_url, sizeof(api_url), entry->zone_id, NULL, entry->domain_name, "A");

    // Keep HTTPS for Cloudflare API

    int http_result = http_request(api_url, HTTP_GET, NULL, headers, &response);
    if (http_result == 0 && response.success && response.data) {
        result = extract_ip_from_json(response.data);
    }

    http_headers_free(headers);
    http_response_free(&response);
    free_cloudflare_config(config);
    return result;
}
