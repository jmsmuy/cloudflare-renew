# Cloudflare DNS Management Tools
# Usage: make [target]

# Compiler settings
CC=gcc
CFLAGS=-Wall -Wextra -std=c99
LIBS=-lcurl
LIBDIR=lib
TESTDIR=tests

# Library files
LIB_SOURCES=$(LIBDIR)/json.c $(LIBDIR)/cloudflare_utils.c $(LIBDIR)/http_utils.c $(LIBDIR)/publicip.c $(LIBDIR)/getip.c $(LIBDIR)/setip.c
LIB_HEADERS=$(LIBDIR)/json.h $(LIBDIR)/cloudflare_utils.h $(LIBDIR)/http_utils.h $(LIBDIR)/publicip.h $(LIBDIR)/getip.h $(LIBDIR)/setip.h

# Main programs
PROGRAMS=tools/getip tools/setip tools/publicip cloudflare_renew

# Test programs
TESTS=test_json_comprehensive test_recursive_search test_serialization test_roundtrip_simple

.PHONY: all clean tests programs help mips mips-docker mips-native

# Default target
all: programs

# Build all programs
programs: $(PROGRAMS)

# Build main programs
tools/getip: tools/getip.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ tools/getip.c $(LIB_SOURCES) $(LIBS)

tools/setip: tools/setip.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ tools/setip.c $(LIB_SOURCES) $(LIBS)

tools/publicip: tools/publicip.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ tools/publicip.c $(LIB_SOURCES) $(LIBS)

cloudflare_renew: cloudflare_renew.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ cloudflare_renew.c $(LIB_SOURCES) $(LIBS)

# Build tests
tests: $(addprefix $(TESTDIR)/, $(TESTS))

$(TESTDIR)/test_json_comprehensive: $(TESTDIR)/test_json_comprehensive.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBDIR)/json.c -I.

$(TESTDIR)/test_recursive_search: $(TESTDIR)/test_recursive_search.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBDIR)/json.c -I.

$(TESTDIR)/test_serialization: $(TESTDIR)/test_serialization.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBDIR)/json.c -I.

$(TESTDIR)/test_roundtrip_simple: $(TESTDIR)/test_roundtrip_simple.c $(LIB_SOURCES) $(LIB_HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBDIR)/json.c -I.

# Run all tests
test: tests
	@echo "Running all tests..."
	@for test in $(addprefix $(TESTDIR)/, $(TESTS)); do \
		echo "Running $$test..."; \
		./$$test; \
		echo ""; \
	done

# Clean up
clean:
	rm -f $(PROGRAMS)
	rm -f $(addprefix $(TESTDIR)/, $(TESTS))
	rm -f cloudflare_renew-mips

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build all programs (default)"
	@echo "  programs  - Build getip and setip"
	@echo "  tests     - Build all test programs"
	@echo "  test      - Build and run all tests"
	@echo "  clean     - Remove all built files"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build programs"
	@echo "  make tests              # Build tests"
	@echo "  make test               # Build and run tests"
	@echo "  make clean              # Clean up"
	@echo "  ./tools/getip cloudflare.conf cloudflare.token [domain] # Get current IP from Cloudflare"
	@echo "  ./getip-all.sh          # Get IP addresses for all configured domains"
	@echo "  ./tools/setip cloudflare.conf cloudflare.token 1.2.3.4 [domain] # Set IP in Cloudflare"
	@echo "  ./setip-all.sh 1.2.3.4 # Set IP for all configured domains"
	@echo "  ./tools/publicip        # Get current public IP from ipinfo.io"
	@echo "  ./cloudflare_renew      # Automatically update all DNS records if IP changed"
	@echo ""
	@echo "Cross-compilation targets:"
	@echo "  make mips               # Build cloudflare_renew for MIPS (uses Docker)"
	@echo "  make mips-native        # Build cloudflare_renew for MIPS (requires native toolchain)"

# MIPS cross-compilation targets
mips: mips-docker

# Docker-based MIPS cross-compilation using Debian Bookworm (recommended)
mips-docker:
	@echo "🏗️  Building cloudflare_renew for MIPS using Debian Bookworm Docker..."
	@docker run --rm -v $(PWD):/workspace -w /workspace \
		--platform linux/amd64 \
		debian:bullseye bash -c " \
		export DEBIAN_FRONTEND=noninteractive && \
		echo '📦 Setting up repositories and architecture...' && \
		apt-get update -qq && \
		dpkg --add-architecture mips64el && \
		echo 'deb http://deb.debian.org/debian bullseye main' > /etc/apt/sources.list && \
		echo 'deb-src http://deb.debian.org/debian bullseye main' >> /etc/apt/sources.list && \
		echo 'deb http://deb.debian.org/debian bullseye-updates main' >> /etc/apt/sources.list && \
		apt-get update -qq && \
		echo '🔍 Checking available MIPS packages...' && \
		apt-cache search libcurl || echo 'No MIPS curl packages found' && \
		apt-cache search openssl | grep mips64el || echo 'No MIPS64EL SSL packages found' && \
		echo '📦 Installing cross-compilation tools...' && \
		apt-get install -y -qq gcc-mips64el-linux-gnuabi64 build-essential pkg-config && \
		echo '📦 Installing available development libraries...' && \
		apt-get install -y -qq \
			libcurl4-openssl-dev:mips64el \
			libssl-dev:mips64el \
			zlib1g-dev:mips64el && \
		echo '🔧 Setting up cross-compilation environment...' && \
		export CC=mips64el-linux-gnuabi64-gcc && \
		export PKG_CONFIG_PATH=/usr/lib/mips64el-linux-gnuabi64/pkgconfig && \
		export CPPFLAGS='-I/usr/include/mips64el-linux-gnuabi64' && \
		export LDFLAGS='-L/usr/lib/mips64el-linux-gnuabi64' && \
		echo '🔧 Compiling cloudflare_renew for MIPS64EL...' && \
		\$$CC -static -Wall -Wextra -std=c99 -o cloudflare_renew-mips \
			\$$CPPFLAGS \
			cloudflare_renew.c \
			lib/json.c lib/cloudflare_utils.c lib/http_utils.c \
			lib/publicip.c lib/getip.c lib/setip.c \
			\$$LDFLAGS -lcurl -lssl -lcrypto -lz -lpthread \
		"
	@echo "✅ MIPS binary created: cloudflare_renew-mips"
	@file cloudflare_renew-mips 2>/dev/null || echo "File command not available"

# Native MIPS cross-compilation (requires toolchain)
mips-native:
	@echo "🏗️  Building cloudflare_renew for MIPS using native toolchain..."
	@if ! command -v mips-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "❌ MIPS cross-compiler not found. Install with:"; \
		echo "   brew install crosstool-ng"; \
		echo "   ct-ng mips-unknown-linux-gnu"; \
		echo "   ct-ng build"; \
		echo "Or use 'make mips-docker' instead."; \
		exit 1; \
	fi
	@export CC=mips-linux-gnu-gcc && \
	export LDFLAGS="-static" && \
	$$CC $(CFLAGS) $$LDFLAGS -o cloudflare_renew-mips \
		cloudflare_renew.c $(LIB_SOURCES) \
		-lcurl -lssl -lcrypto -lz -pthread -ldl
	@echo "✅ MIPS binary created: cloudflare_renew-mips"
	@file cloudflare_renew-mips 2>/dev/null || echo "File command not available"

# Clean MIPS binaries
clean-mips:
	rm -f cloudflare_renew-mips
