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

.PHONY: all clean tests programs help

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
