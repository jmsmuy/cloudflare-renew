# Building cloudflare-renew for OpenWrt MIPSEL-24k

This guide provides multiple methods to cross-compile the cloudflare-renew project for OpenWrt routers using MIPSEL-24k architecture (common in many routers like TP-Link, Netgear, etc.).

## Prerequisites

- OpenSSL development libraries
- POSIX-compliant system
- One of the following toolchains:
  - OpenWrt SDK
  - Generic MIPSEL cross-compiler
  - Docker

## Method 1: Using Generic MIPSEL Toolchain (Easiest)

This is the simplest method if you don't have OpenWrt SDK installed.

### Install Toolchain

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install gcc-mipsel-linux-gnu
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-mipsel-linux-gnu
```

**Arch Linux:**
```bash
yay -S mipsel-linux-gnu-gcc  # From AUR
```

### Build

```bash
# Build all programs
make -f Makefile.mipsel

# Strip binaries (reduces size by ~70%)
make -f Makefile.mipsel strip

# Check binary sizes
make -f Makefile.mipsel size

# Create distribution package
make -f Makefile.mipsel dist
```

The binaries will be statically linked for maximum compatibility.

## Method 2: Using OpenWrt SDK

This method produces binaries optimized for OpenWrt with proper musl libc linking.

### Download OpenWrt SDK

1. Go to [OpenWrt Downloads](https://downloads.openwrt.org/releases/)
2. Choose your OpenWrt version (e.g., 21.02.3, 22.03.5)
3. Navigate to `targets/ramips/mt7621/` (or your specific target)
4. Download `openwrt-sdk-*-mipsel_24kc_*.tar.xz`

### Setup SDK

```bash
# Extract SDK
cd ~
tar xf openwrt-sdk-*.tar.xz
mv openwrt-sdk-* openwrt-sdk

# Setup environment
cd /path/to/cloudflare-renew
source ./setup-openwrt-env.sh
```

### Build with SDK

```bash
# Using the cross-compilation Makefile
make -f Makefile.cross

# Strip and install
make -f Makefile.cross strip
make -f Makefile.cross install DESTDIR=/tmp/cloudflare
```

## Method 3: Using Docker (No Local Toolchain Required)

This method uses Docker to avoid installing any toolchain locally.

### Prerequisites

- Docker installed and running

### Build

```bash
# Build using Docker
make -f Makefile.docker

# Binaries will be in ./build-output/
ls -la build-output/
```

### Interactive Development

```bash
# Open shell in build container
make -f Makefile.docker docker-shell

# Inside container, you can build manually
cd /src
make -f Makefile.cross
```

## Method 4: OpenWrt Package (For OpenWrt Developers)

If you're building a custom OpenWrt firmware:

### Setup Package

```bash
# In your OpenWrt buildroot
mkdir -p package/cloudflare-renew
cp /path/to/cloudflare-renew/* package/cloudflare-renew/src/
cp /path/to/Makefile.openwrt package/cloudflare-renew/Makefile
```

### Build Package

```bash
# Update package index
./scripts/feeds update -a
./scripts/feeds install -a

# Configure
make menuconfig
# Navigate to Utilities -> cloudflare-renew

# Build
make package/cloudflare-renew/compile V=s
```

## Method 5: Using Buildroot

For Buildroot-based systems:

```bash
# Copy package files
cp Makefile.buildroot buildroot/package/cloudflare-renew/cloudflare-renew.mk
cp -r . buildroot/package/cloudflare-renew/

# Configure and build
cd buildroot
make menuconfig  # Enable cloudflare-renew
make
```

## Deployment to Router

### Via SCP

```bash
# Copy binaries to router
scp cloudflare_renew root@192.168.1.1:/usr/bin/
scp tools/getip root@192.168.1.1:/usr/bin/cloudflare-getip
scp tools/setip root@192.168.1.1:/usr/bin/cloudflare-setip
scp tools/publicip root@192.168.1.1:/usr/bin/cloudflare-publicip

# Make executable
ssh root@192.168.1.1 'chmod +x /usr/bin/cloudflare*'
```

### Via OPKG (if packaged)

```bash
# Copy package to router
scp cloudflare-renew_1.0.0_mipsel_24kc.ipk root@192.168.1.1:/tmp/

# Install
ssh root@192.168.1.1 'opkg install /tmp/cloudflare-renew_1.0.0_mipsel_24kc.ipk'
```

## Configuration on Router

1. Create configuration file:
```bash
ssh root@192.168.1.1
cat > /etc/cloudflare/cloudflare.conf << EOF
# Your domains
example.com
subdomain.example.com
EOF

cat > /etc/cloudflare/cloudflare.token << EOF
your-cloudflare-api-token-here
EOF
```

2. Set up cron job for automatic updates:
```bash
echo "*/15 * * * * /usr/bin/cloudflare_renew" >> /etc/crontabs/root
/etc/init.d/cron restart
```

## Troubleshooting

### Binary Too Large

If binaries are too large for your router:

1. Use strip to remove debug symbols:
```bash
make -f Makefile.mipsel strip
```

2. Use UPX compression (if available):
```bash
upx --best cloudflare_renew
```

3. Build with size optimization:
```bash
CFLAGS="-Os" make -f Makefile.mipsel clean all
```

### Missing Libraries

If you get "library not found" errors on the router:

1. Check dependencies:
```bash
mipsel-linux-gnu-ldd cloudflare_renew
```

2. Use static linking (already done in Makefile.mipsel):
```bash
LDFLAGS="-static" make -f Makefile.mipsel
```

### SSL Certificate Issues

If SSL verification fails:

1. Update router's ca-certificates:
```bash
opkg update
opkg install ca-certificates
```

2. Or disable SSL verification (not recommended for production):
```c
// In lib/socket_http.c, change:
SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);
```

## Binary Size Optimization Tips

1. **Compiler Flags**: The Makefiles use `-Os` for size optimization
2. **Strip Symbols**: Always strip binaries before deployment
3. **Static vs Dynamic**: Static linking increases size but improves compatibility
4. **LTO**: Link-Time Optimization can reduce size:
```bash
CFLAGS="-Os -flto" LDFLAGS="-flto" make -f Makefile.mipsel
```

## Testing

Before deploying to router, test on a QEMU MIPS emulator:

```bash
# Install QEMU
sudo apt-get install qemu-user-static

# Test binary
qemu-mipsel-static ./cloudflare_renew
```

## Support Matrix

| Router Architecture | Makefile to Use | Notes |
|-------------------|-----------------|-------|
| MIPSEL 24Kc | Makefile.mipsel | Most common |
| MIPSEL 74Kc | Makefile.mipsel | Change -march=74kc |
| MIPS32r2 | Makefile.mipsel | Change -march=mips32r2 |
| ARM Cortex-A7 | Modify for ARM | Different toolchain needed |

## Common Router Models

This build configuration works for routers with MIPSEL 24Kc processors:

- TP-Link Archer C7, C9, C20, C50
- Netgear R6220, R6350
- Xiaomi Mi Router 3G, 4A
- GL.iNet GL-MT300N-V2
- Many MediaTek MT7621 based routers

## Additional Resources

- [OpenWrt Cross-Compile Guide](https://openwrt.org/docs/guide-developer/crosscompile)
- [OpenWrt SDK Documentation](https://openwrt.org/docs/guide-developer/using_the_sdk)
- [MIPS Architecture Reference](https://www.mips.com/products/architectures/mips32-2/)

## License

This build configuration maintains the original GPL-3.0 license of the cloudflare-renew project.