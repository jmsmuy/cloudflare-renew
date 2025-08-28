#ifndef CLOUDFLARE_UTILS_H
#define CLOUDFLARE_UTILS_H

#include <stddef.h>

// Configuration entry structure
typedef struct {
    char* zone_id;
    char* dns_record_id;
    char* domain_name;
} cloudflare_entry_t;

// Configuration structure with arrays
typedef struct {
    cloudflare_entry_t* entries;
    int entry_count;
    char* cloudflare_token;
} cloudflare_config_t;

// Function declarations
cloudflare_config_t* load_cloudflare_config(const char* config_file, const char* token_file);
void free_cloudflare_config(cloudflare_config_t* config);
cloudflare_entry_t* find_entry_by_domain(cloudflare_config_t* config, const char* domain_name);
cloudflare_entry_t* get_entry_by_index(cloudflare_config_t* config, int index);
void build_cloudflare_dns_url(char* url_buffer, size_t buffer_size, 
                              const char* zone_id, const char* dns_record_id, 
                              const char* domain_name, const char* record_type);

#endif // CLOUDFLARE_UTILS_H
