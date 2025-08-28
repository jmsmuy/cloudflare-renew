#include "lib/getip.h"
#include "lib/publicip.h"
#include "lib/setip.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CONFIG_FILE "cloudflare.conf"
#define TOKEN_FILE "cloudflare.token"
#define LAST_IP_FILE "last.ip"
#define LOG_FILE "cloudflare.log"

// Function to write log messages with timestamp
void write_log(const char *message)
{
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        fprintf(stderr, "Warning: Could not open log file %s\n", LOG_FILE);
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    // Remove newline from ctime
    timestamp[strlen(timestamp) - 1] = '\0';

    fprintf(log, "[%s] %s\n", timestamp, message);
    fclose(log);
}

// Function to read IP from file
char *read_ip_from_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    char *ip = malloc(64);
    if (!ip) {
        fclose(file);
        return NULL;
    }

    if (fgets(ip, 64, file)) {
        // Remove trailing newline
        char *newline = strchr(ip, '\n');
        if (newline)
            *newline = '\0';

        fclose(file);
        return ip;
    }

    free(ip);
    fclose(file);
    return NULL;
}

// Function to write IP to file
int write_ip_to_file(const char *filename, const char *ip)
{
    FILE *file = fopen(filename, "w");
    if (!file) {
        return 1;
    }

    fprintf(file, "%s\n", ip);
    fclose(file);
    return 0;
}

// Function to get all domain names from config
char **get_all_domains(int *count)
{
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        *count = 0;
        return NULL;
    }

    char **domains = NULL;
    *count = 0;
    char line[512];

    while (fgets(line, sizeof(line), file)) {
        // Look for DOMAIN_NAME[x]= lines
        if (strstr(line, "DOMAIN_NAME[") && !strstr(line, "#")) {
            char *equals = strchr(line, '=');
            if (equals) {
                char *domain = equals + 1;
                // Trim whitespace
                while (*domain == ' ' || *domain == '\t')
                    domain++;
                char *end = domain + strlen(domain) - 1;
                while (end > domain && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
                    *end = '\0';
                    end--;
                }

                if (strlen(domain) > 0) {
                    domains = realloc(domains, (*count + 1) * sizeof(char *));
                    domains[*count] = strdup(domain);
                    (*count)++;
                }
            }
        }
    }

    fclose(file);
    return domains;
}

int main(void)
{
    char log_msg[512];

    write_log("=== Starting cloudflare_renew ===");

    // Step 1: Get current public IP
    write_log("Getting current public IP...");
    char *public_ip = get_public_ip();
    if (!public_ip) {
        write_log("ERROR: Failed to get public IP");
        return 1;
    }

    snprintf(log_msg, sizeof(log_msg), "Current public IP: %s", public_ip);
    write_log(log_msg);

    // Step 2: Check last.ip file
    char *last_ip = read_ip_from_file(LAST_IP_FILE);
    bool ip_changed = false;

    if (!last_ip) {
        write_log("last.ip file not found - first run or file missing");
        ip_changed = true;
    } else {
        snprintf(log_msg, sizeof(log_msg), "Last recorded IP: %s", last_ip);
        write_log(log_msg);

        if (strcmp(public_ip, last_ip) != 0) {
            write_log("IP address has changed!");
            ip_changed = true;
        } else {
            write_log("IP address unchanged - no updates needed");
        }
    }

    if (ip_changed) {
        // Step 3: Get all domains from config
        int domain_count = 0;
        char **domains = get_all_domains(&domain_count);

        if (!domains || domain_count == 0) {
            write_log("ERROR: No domains found in configuration");
            free(public_ip);
            free(last_ip);
            return 1;
        }

        snprintf(log_msg, sizeof(log_msg), "Found %d domains to check/update", domain_count);
        write_log(log_msg);

        // Step 4: Process each domain
        int updated_count = 0;
        for (int i = 0; i < domain_count; i++) {
            snprintf(log_msg, sizeof(log_msg), "Processing domain: %s", domains[i]);
            write_log(log_msg);

            // Get current Cloudflare IP for this domain
            char *cf_ip = get_cloudflare_ip(CONFIG_FILE, TOKEN_FILE, domains[i]);
            if (!cf_ip) {
                snprintf(log_msg, sizeof(log_msg), "ERROR: Failed to get Cloudflare IP for %s", domains[i]);
                write_log(log_msg);
                continue;
            }

            snprintf(log_msg, sizeof(log_msg), "Current Cloudflare IP for %s: %s", domains[i], cf_ip);
            write_log(log_msg);

            // Check if update is needed
            if (strcmp(cf_ip, public_ip) != 0) {
                snprintf(log_msg, sizeof(log_msg), "Updating %s from %s to %s", domains[i], cf_ip, public_ip);
                write_log(log_msg);

                // Update the IP
                if (set_cloudflare_ip(CONFIG_FILE, TOKEN_FILE, public_ip, domains[i]) == 0) {
                    snprintf(log_msg, sizeof(log_msg), "Successfully updated %s", domains[i]);
                    write_log(log_msg);

                    // Verify the update
                    char *verify_ip = get_cloudflare_ip(CONFIG_FILE, TOKEN_FILE, domains[i]);
                    if (verify_ip && strcmp(verify_ip, public_ip) == 0) {
                        snprintf(log_msg, sizeof(log_msg), "Verification successful for %s", domains[i]);
                        write_log(log_msg);
                        updated_count++;
                    } else {
                        snprintf(log_msg, sizeof(log_msg), "Verification failed for %s", domains[i]);
                        write_log(log_msg);
                    }
                    free(verify_ip);
                } else {
                    snprintf(log_msg, sizeof(log_msg), "Failed to update %s", domains[i]);
                    write_log(log_msg);
                }
            } else {
                snprintf(log_msg, sizeof(log_msg), "No update needed for %s (already correct)", domains[i]);
                write_log(log_msg);
            }

            free(cf_ip);
        }

        snprintf(log_msg, sizeof(log_msg), "Processing complete: %d domains updated", updated_count);
        write_log(log_msg);

        // Cleanup domains array
        for (int i = 0; i < domain_count; i++) {
            free(domains[i]);
        }
        free(domains);

        // Step 5: Update last.ip file
        if (write_ip_to_file(LAST_IP_FILE, public_ip) == 0) {
            write_log("Updated last.ip file with new IP");
        } else {
            write_log("ERROR: Failed to update last.ip file");
        }
    }

    free(public_ip);
    free(last_ip);

    write_log("=== cloudflare_renew completed ===");
    return 0;
}
