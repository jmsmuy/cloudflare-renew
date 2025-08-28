#ifndef GETIP_H
#define GETIP_H

// Function to get IP from Cloudflare DNS
char *get_cloudflare_ip(const char *config_file, const char *token_file, const char *domain_name);

#endif // GETIP_H
