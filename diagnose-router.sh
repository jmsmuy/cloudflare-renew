#!/bin/sh
# Router architecture diagnostic script
# Run this ON YOUR ROUTER to get exact build requirements

echo "Router Architecture Diagnostics"
echo "==============================="
echo ""

# CPU Information
echo "1. CPU Information:"
if [ -f /proc/cpuinfo ]; then
    echo "Processor:"
    grep -E "system type|cpu model|processor" /proc/cpuinfo | head -5
    echo ""
fi

# Architecture
echo "2. Architecture:"
uname -m
echo ""

# Kernel version
echo "3. Kernel:"
uname -r
echo ""

# C Library
echo "4. C Library:"
if [ -f /lib/libc.so.0 ]; then
    echo "musl libc detected"
    /lib/libc.so.0 2>&1 | head -1
elif [ -f /lib/libc.so.6 ]; then
    echo "glibc detected"
    /lib/libc.so.6 | head -1
elif ldd --version 2>/dev/null | grep -q musl; then
    echo "musl libc detected (via ldd)"
elif ldd --version 2>/dev/null | grep -q GLIBC; then
    echo "glibc detected (via ldd)"
    ldd --version | head -1
else
    echo "Unknown C library"
fi
echo ""

# Check existing binaries
echo "5. Sample binary info (busybox):"
if command -v busybox >/dev/null 2>&1; then
    file $(which busybox) 2>/dev/null || echo "file command not available"
    readelf -h $(which busybox) 2>/dev/null | grep -E "Class:|Machine:|Flags:" || echo "readelf not available"
fi
echo ""

# OpenSSL
echo "6. OpenSSL availability:"
if [ -f /usr/lib/libssl.so* ] || [ -f /lib/libssl.so* ]; then
    echo "OpenSSL libraries found:"
    ls -la /usr/lib/libssl* /lib/libssl* 2>/dev/null | head -3
else
    echo "No OpenSSL libraries found"
fi
echo ""

# Memory info
echo "7. Memory:"
free 2>/dev/null || cat /proc/meminfo | grep -E "MemTotal|MemFree" | head -2
echo ""

# OpenWrt version if available
echo "8. OpenWrt version:"
if [ -f /etc/openwrt_release ]; then
    cat /etc/openwrt_release
else
    echo "Not OpenWrt or version file not found"
fi
echo ""

# Endianness test
echo "9. Endianness:"
echo -n "System is: "
if echo -n I | od -o | head -1 | grep -q "000000 000111"; then
    echo "Little Endian (correct for MIPSEL)"
else
    echo "Big Endian (wrong - need MIPS not MIPSEL)"
fi
echo ""

echo "==============================="
echo "Please share this output to get the correct build!"