#include "setip.h"

#include "cloudflare_utils.h"
#include "json.h"
#include "socket_http.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to build update JSON
static struct json_root *build_update_json(const char *ip_address, const char *domain_name)
{
    struct json_object *update_object = NULL;

    append_object(&update_object, create_string_object("name", domain_name));
    append_object(&update_object, create_number_object("ttl", 3600));
    append_object(&update_object, create_string_object("type", "A"));
    append_object(&update_object, create_string_object("comment", "Domain verification record"));
    append_object(&update_object, create_string_object("content", ip_address));
    append_object(&update_object, create_boolean_object("proxied", true));

    struct json_root *root = malloc(sizeof(struct json_root));
    if (!root)
        return NULL;

    root->object = update_object;
    root->array = NULL;
    root->is_array = false;

    return root;
}

// Set IP in Cloudflare DNS
int set_cloudflare_ip(const char *config_file, const char *token_file, const char *ip_address, const char *domain_name)
{
    cloudflare_config_t *config = load_cloudflare_config(config_file, token_file);
    if (!config) {
        return 1;
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
        return 1;
    }

    // Build the JSON structure
    struct json_root *json_root = build_update_json(ip_address, entry->domain_name);
    if (!json_root) {
        free_cloudflare_config(config);
        return 1;
    }

    char *json_string = json_to_string(json_root);
    if (!json_string) {
        free_cloudflare_config(config);
        free(json_root);
        return 1;
    }

    struct http_response response;
    int result = 1; // Default to failure

    http_response_init(&response);

    // Build headers
    struct http_header *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", config->cloudflare_token);
    headers = http_header_add(headers, "Authorization", auth_header);
    headers = http_header_add(headers, "Content-Type", "application/json");

    char url[1024];
    build_cloudflare_dns_url(url, sizeof(url), entry->zone_id, entry->dns_record_id, NULL, NULL);

    // Keep HTTPS for Cloudflare API

    int http_result = http_request(url, HTTP_PUT, json_string, headers, &response);
    if (http_result == 0 && response.success && response.data) {
        // Parse the response to check if update was successful
        struct json_root *response_root = parse_json(response.data);
        if (response_root) {
            int success_count = 0;
            bool *success_values = get_boolean_values(response_root, "success", &success_count);

            bool operation_successful = false;
            if (success_values && success_count > 0) {
                operation_successful = success_values[0];
                free(success_values);
            }

            if (operation_successful) {
                // Verify the IP was set correctly
                int content_count = 0;
                char **content_values = get_string_values(response_root, "content", &content_count);

                if (content_values && content_count > 0) {
                    if (strcmp(content_values[0], ip_address) == 0) {
                        result = 0; // Success
                    }

                    // Free the content values
                    for (int i = 0; i < content_count; i++) {
                        free(content_values[i]);
                    }
                    free((void *) content_values);
                }
            }

            free(response_root);
        }
    }

    http_headers_free(headers);
    http_response_free(&response);
    free(json_string);
    free_cloudflare_config(config);
    free(json_root);

    return result;
}
