# Cloudflare DNS Renew

A C-based dynamic DNS client for Cloudflare that automatically updates DNS records when your public IP changes.

## Features

- **Automatic IP Detection**: Fetches your public IP from `ipinfo.io`
- **DNS Record Management**: Gets and sets DNS records via Cloudflare API
- **Multi-Domain Support**: Manages multiple domains from a single configuration
- **State Tracking**: Remembers last IP to avoid unnecessary updates
- **Comprehensive Logging**: Logs all operations to `cloudflare.log`
- **Cross-Platform**: Compiles for x86_64 and MIPS64EL architectures
- **Static Linking**: Produces standalone binaries with no dependencies

## Project Structure

```
├── cloudflare_renew.c      # Main automatic renewal program
├── tools/                  # Individual utility programs
│   ├── getip.c            # Get current DNS record IP
│   ├── setip.c            # Set DNS record IP
│   └── publicip.c         # Get public IP address
├── lib/                    # Shared libraries
│   ├── json.c/.h          # Custom JSON parser/serializer
│   ├── cloudflare_utils.c/.h  # Cloudflare API utilities
│   ├── getip.c/.h         # DNS record retrieval library
│   ├── setip.c/.h         # DNS record update library
│   ├── publicip.c/.h      # Public IP detection library
│   └── http_utils.c/.h    # HTTP response handling utilities
├── tests/                  # Test programs
├── scripts/               # Shell scripts for bulk operations
│   ├── getip-all.sh       # Check all configured domains
│   └── setip-all.sh       # Update all configured domains
└── Makefile               # Build system
```

## Configuration

### cloudflare.conf
Contains non-sensitive configuration for multiple domains:
```bash
# Entry 0: Primary domain
ZONE_ID[0]=your_zone_id_here
DNS_RECORD_ID[0]=your_dns_record_id_here
DOMAIN_NAME[0]=example.com

# Entry 1: Secondary domain
ZONE_ID[1]=your_zone_id_here
DNS_RECORD_ID[1]=your_dns_record_id_here
DOMAIN_NAME[1]=subdomain.example.com
```

### cloudflare.token
Contains your Cloudflare API token:
```
your_cloudflare_api_token_here
```

## Building

### Prerequisites
- GCC compiler
- libcurl development headers
- OpenSSL development headers
- zlib development headers

### Compile for local architecture
```bash
make
```

### Cross-compile for MIPS64EL (OpenWrt)
```bash
make mips
```

### Build individual components
```bash
make tools          # Build all tools
make tests           # Build all tests
make clean           # Clean build artifacts
```

## Usage

### Automatic Renewal (Recommended)
```bash
./cloudflare_renew
```
This is the main program that:
1. Gets your current public IP
2. Compares with stored IP in `last.ip`
3. For each domain, checks current Cloudflare DNS IP
4. Updates DNS if different from public IP
5. Logs all operations

### Manual Tools

#### Get current public IP
```bash
./tools/publicip
```

#### Get DNS record IP for a domain
```bash
./tools/getip cloudflare.conf cloudflare.token [domain_name]
```

#### Set DNS record IP for a domain
```bash
./tools/setip cloudflare.conf cloudflare.token <ip_address> [domain_name]
```

#### Bulk operations
```bash
./scripts/getip-all.sh    # Check all domains
./scripts/setip-all.sh <ip>  # Update all domains
```

## API Integration

This tool uses the Cloudflare v4 API:
- **GET** `/zones/{zone_id}/dns_records/{record_id}` - Get DNS record
- **PUT** `/zones/{zone_id}/dns_records/{record_id}` - Update DNS record

## Cross-Compilation

The project supports cross-compilation for MIPS64EL architecture (commonly used in OpenWrt routers) using Docker:

```bash
make mips-docker  # Uses Debian Bullseye with mips64el toolchain
```

## Logging

All operations are logged to `cloudflare.log` with timestamps:
```
[2024-01-01 12:00:00] Starting cloudflare_renew...
[2024-01-01 12:00:01] Public IP: 203.0.113.1
[2024-01-01 12:00:02] example.com: Current IP 203.0.113.2 != Public IP 203.0.113.1
[2024-01-01 12:00:03] example.com: Successfully updated to 203.0.113.1
```

## Error Handling

- Returns exit code 0 on success, 1 on failure
- Detailed error messages in logs
- Graceful handling of API errors and network issues

## Security

- API token stored separately from configuration
- No hardcoded credentials
- Secure HTTPS communication with Cloudflare

## License

GNU Affero General Public License v3.0 - see LICENSE file for details.

**Important Note**: This project uses AGPL v3, which requires that if you modify this software and make it available to users over a network (e.g., as a web service), you must also provide the source code of your modifications to those users.
