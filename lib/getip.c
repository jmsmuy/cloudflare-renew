#define _POSIX_C_SOURCE 200809L
#include "getip.h"

#include "cloudflare_utils.h"
#include "http_utils.h"
#include "json.h"

#include <curl/curl.h>
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

    CURL *curl = NULL;
    CURLcode res = CURLE_OK;
    struct http_response response;
    char *result = NULL;

    http_response_init(&response);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", config->cloudflare_token);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");

        char api_url[1024];
        build_cloudflare_dns_url(api_url, sizeof(api_url), entry->zone_id, NULL, entry->domain_name, "A");

        curl_easy_setopt(curl, CURLOPT_URL, api_url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK && response.data) {
            result = extract_ip_from_json(response.data);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    http_response_free(&response);
    free_cloudflare_config(config);
    return result;
}
