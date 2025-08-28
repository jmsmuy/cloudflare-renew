#include "setip.h"

#include "cloudflare_utils.h"
#include "http_utils.h"
#include "json.h"

#include <curl/curl.h>
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
    cloudflare_entry_t *entry = NULL;
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

    CURL *curl;
    CURLcode res;
    struct http_response response;
    int result = 1; // Default to failure

    http_response_init(&response);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", config->cloudflare_token);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");

        char url[1024];
        build_cloudflare_dns_url(url, sizeof(url), entry->zone_id, entry->dns_record_id, NULL, NULL);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK && response.data) {
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
                        free(content_values);
                    }
                }

                free(response_root);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    http_response_free(&response);
    free(json_string);
    free_cloudflare_config(config);
    free(json_root);

    return result;
}
