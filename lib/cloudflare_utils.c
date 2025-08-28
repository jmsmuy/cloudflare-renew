#define _POSIX_C_SOURCE 200809L
#include "cloudflare_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to trim whitespace
char *trim_whitespace(char *str)
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

// Helper function to read a single value from file (like the old read_value_from_file)
static char *read_token_from_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open token file '%s'\n", filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "Error: Token file '%s' is empty\n", filename);
        fclose(file);
        return NULL;
    }

    // Allocate memory for token (including null terminator)
    char *token = malloc(file_size + 1);
    if (!token) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read the token
    size_t bytes_read = fread(token, 1, (size_t) file_size, file);
    fclose(file);

    if (bytes_read != (size_t) file_size) {
        fprintf(stderr, "Error: Failed to read complete token from file\n");
        free(token);
        return NULL;
    }

    // Remove any trailing whitespace/newlines and null terminate
    token[file_size] = '\0';
    // Trim trailing whitespace
    char *end = token + file_size - 1;
    while (end > token && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }

    return token;
}

// Parse array index from key like "ZONE_ID[0]"
static int parse_array_index(const char *key, char *base_key, size_t base_key_size)
{
    const char *bracket_start = strchr(key, '[');
    if (!bracket_start) {
        return -1; // Not an array format
    }

    const char *bracket_end = strchr(bracket_start, ']');
    if (!bracket_end) {
        return -1; // Invalid format
    }

    // Extract base key
    size_t base_len = bracket_start - key;
    if (base_len >= base_key_size) {
        return -1; // Key too long
    }
    strncpy(base_key, key, base_len);
    base_key[base_len] = '\0';

    // Extract index
    char index_str[16];
    size_t index_len = bracket_end - bracket_start - 1;
    if (index_len >= sizeof(index_str)) {
        return -1; // Index too long
    }
    strncpy(index_str, bracket_start + 1, index_len);
    index_str[index_len] = '\0';

    char *endptr = NULL;
    long result = strtol(index_str, &endptr, 10);
    return (int) result;
}

// Load configuration from two separate files
cloudflare_config_t *load_cloudflare_config(const char *config_file, const char *token_file)
{
    // First read the main config file
    FILE *file = fopen(config_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open config file '%s'\n", config_file);
        return NULL;
    }

    cloudflare_config_t *config = malloc(sizeof(cloudflare_config_t));
    if (!config) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Initialize fields
    config->entries = NULL;
    config->entry_count = 0;
    config->cloudflare_token = NULL;

    // First pass: count maximum index to allocate array
    int max_index = -1;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char *trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }

        char *equals = strchr(trimmed, '=');
        if (!equals)
            continue;

        *equals = '\0';
        const char *key = trim_whitespace(trimmed);

        char base_key[64];
        int index = parse_array_index(key, base_key, sizeof(base_key));
        if (index >= 0 && index > max_index) {
            max_index = index;
        }
    }

    if (max_index < 0) {
        fprintf(stderr, "Error: No valid array entries found in config file\n");
        free(config);
        fclose(file);
        return NULL;
    }

    // Allocate entries array
    config->entry_count = max_index + 1;
    config->entries = calloc(config->entry_count, sizeof(cloudflare_entry_t));
    if (!config->entries) {
        fprintf(stderr, "Error: Memory allocation failed for entries\n");
        free(config);
        fclose(file);
        return NULL;
    }

    // Second pass: parse values
    if (fseek(file, 0, SEEK_SET) != 0) {
        free_cloudflare_config(config);
        fclose(file);
        return NULL;
    }
    while (fgets(line, sizeof(line), file)) {
        char *trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }

        char *equals = strchr(trimmed, '=');
        if (!equals)
            continue;

        *equals = '\0';
        const char *key = trim_whitespace(trimmed);
        const char *value = trim_whitespace(equals + 1);

        char base_key[64];
        int index = parse_array_index(key, base_key, sizeof(base_key));

        if (index >= 0 && index < config->entry_count) {
            if (strcmp(base_key, "ZONE_ID") == 0) {
                config->entries[index].zone_id = strdup(value);
            } else if (strcmp(base_key, "DNS_RECORD_ID") == 0) {
                config->entries[index].dns_record_id = strdup(value);
            } else if (strcmp(base_key, "DOMAIN_NAME") == 0) {
                config->entries[index].domain_name = strdup(value);
            }
        }
    }

    fclose(file);

    // Now read the token from the separate file
    config->cloudflare_token = read_token_from_file(token_file);
    if (!config->cloudflare_token) {
        free_cloudflare_config(config);
        return NULL;
    }

    // Validate that at least the first entry has all required fields
    if (config->entry_count > 0) {
        const cloudflare_entry_t *first_entry = &config->entries[0];
        if (!first_entry->zone_id || !first_entry->dns_record_id || !first_entry->domain_name) {
            fprintf(stderr, "Error: First entry missing required fields\n");
            free_cloudflare_config(config);
            return NULL;
        }
    }

    return config;
}

// Free configuration memory
void free_cloudflare_config(cloudflare_config_t *config)
{
    if (config) {
        if (config->entries) {
            for (int i = 0; i < config->entry_count; i++) {
                free(config->entries[i].zone_id);
                free(config->entries[i].dns_record_id);
                free(config->entries[i].domain_name);
            }
            free(config->entries);
        }
        free(config->cloudflare_token);
        free(config);
    }
}

// Find entry by domain name
cloudflare_entry_t *find_entry_by_domain(cloudflare_config_t *config, const char *domain_name)
{
    if (!config || !config->entries || !domain_name) {
        return NULL;
    }

    for (int i = 0; i < config->entry_count; i++) {
        if (config->entries[i].domain_name && strcmp(config->entries[i].domain_name, domain_name) == 0) {
            return &config->entries[i];
        }
    }

    return NULL;
}

// Get entry by index
cloudflare_entry_t *get_entry_by_index(cloudflare_config_t *config, int index)
{
    if (!config || !config->entries || index < 0 || index >= config->entry_count) {
        return NULL;
    }

    return &config->entries[index];
}

// Build Cloudflare DNS URL
void build_cloudflare_dns_url(char *url_buffer,
                              size_t buffer_size,
                              const char *zone_id,
                              const char *dns_record_id,
                              const char *domain_name,
                              const char *record_type)
{
    if (dns_record_id != NULL) {
        // For specific record operations (PUT/DELETE) - setip
        snprintf(url_buffer,
                 buffer_size,
                 "https://api.cloudflare.com/client/v4/zones/%s/dns_records/%s",
                 zone_id,
                 dns_record_id);
    } else if (domain_name != NULL && record_type != NULL) {
        // For querying records (GET) - getip
        snprintf(url_buffer,
                 buffer_size,
                 "https://api.cloudflare.com/client/v4/zones/%s/dns_records?name=%s&type=%s",
                 zone_id,
                 domain_name,
                 record_type);
    } else {
        // Just the base URL for listing all records
        const char *base_url = "https://api.cloudflare.com/client/v4/zones/%s/dns_records";
        snprintf(url_buffer, buffer_size, base_url, zone_id);
    }
}
