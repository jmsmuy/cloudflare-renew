#ifndef SETIP_H
#define SETIP_H

// Function to set IP in Cloudflare DNS
// Returns 0 on success, 1 on failure
int set_cloudflare_ip(const char* config_file, const char* token_file, const char* ip_address, const char* domain_name);

#endif // SETIP_H
