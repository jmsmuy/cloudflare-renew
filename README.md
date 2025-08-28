# Cloudflare DNS Renew

A C-based dynamic DNS client for Cloudflare that automatically updates DNS records when your public IP changes.

## Features

- **Automatic IP Detection**: Fetches your public IP from `ipinfo.io`
- **DNS Record Management**: Gets and sets DNS records via Cloudflare API
- **Multi-Domain Support**: Manages multiple domains from a single configuration
- **State Tracking**: Remembers last IP to avoid unnecessary updates
- **Comprehensive Logging**: Logs all operations to `cloudflare.log`
- **Cross-Platform**: Compiles for multiple architectures
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

### Development Tools (Optional)
For code formatting and linting:
- clang-format (code formatting)
- clang-tidy (static analysis)
- cppcheck (additional static analysis)

Install with: `make install-tools`

### Compile for local architecture
```bash
make
```

### Build individual components
```bash
make tools          # Build all tools
make tests           # Build all tests
make clean           # Clean build artifacts
```

### Code quality and formatting
```bash
make install-tools  # Install development tools (clang-format, clang-tidy, cppcheck)
make format         # Format all source code
make check-format   # Check if code is properly formatted
make lint           # Run all linters (cppcheck, clang-tidy)
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

## Code Quality

This project includes comprehensive code quality tools:

- **Automatic Formatting**: Uses `clang-format` with consistent C99 style
- **Static Analysis**: `cppcheck` for bug detection and code quality
- **Linting**: `clang-tidy` for additional code analysis and best practices
- **Pre-configured**: All tools come with project-specific configurations

### Running Quality Checks
```bash
make format         # Format all code automatically
make check-format   # Verify code formatting
make lint           # Run all static analysis tools
```

## Continuous Integration

The project uses GitHub Actions for automated quality assurance and releases:

### 🔍 **CI Pipeline** (Pull Requests)
- **Triggers**: On pull requests to `main` branch
- **Checks**: 
  - Code formatting verification
  - Static analysis (cppcheck, clang-tidy)
  - Build verification for all programs
  - Test suite execution
  - Binary functionality testing

### 🚀 **Release Pipeline** (Main Branch)
- **Triggers**: On pushes to `main` branch or manual dispatch
- **Actions**:
  - Automatic version detection (from `VERSION` file or date-based)
  - Release creation with detailed changelog
  - Linux x86_64 binary compilation with static linking
  - Packaged distribution with setup scripts and documentation
  - Automatic documentation updates

### 📦 **Binary Releases**
Each release includes:
- Pre-compiled static binaries for Linux x86_64
- Configuration templates (`cloudflare.conf.sample`)
- Setup script (`install.sh`) for quick deployment
- Complete documentation and license files

## Security

- API token stored separately from configuration
- No hardcoded credentials
- Secure HTTPS communication with Cloudflare

## License

GNU Affero General Public License v3.0 - see LICENSE file for details.

**Important Note**: This project uses AGPL v3, which requires that if you modify this software and make it available to users over a network (e.g., as a web service), you must also provide the source code of your modifications to those users.
