# Cloudflare DNS Management Tools
# Usage: make [target]

# Compiler settings
CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -I/opt/homebrew/opt/openssl@3/include
LIBS=-L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
LIBDIR=lib
TESTDIR=tests

# Library files
LIB_SOURCES=$(LIBDIR)/json.c $(LIBDIR)/cloudflare_utils.c $(LIBDIR)/socket_http.c $(LIBDIR)/publicip.c $(LIBDIR)/getip.c $(LIBDIR)/setip.c
LIB_HEADERS=$(LIBDIR)/json.h $(LIBDIR)/cloudflare_utils.h $(LIBDIR)/socket_http.h $(LIBDIR)/publicip.h $(LIBDIR)/getip.h $(LIBDIR)/setip.h

# Main programs
PROGRAMS=tools/getip tools/setip tools/publicip cloudflare-renew

# Test programs
TESTS=test_json_comprehensive test_recursive_search test_serialization test_roundtrip_simple

.PHONY: all clean tests programs help format lint check-format install-tools

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

cloudflare-renew: cloudflare_renew.c $(LIB_SOURCES) $(LIB_HEADERS)
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
	@echo ""
	@echo "Code quality targets:"
	@echo "  make format             # Format all source code"
	@echo "  make check-format       # Check if code is properly formatted"
	@echo "  make lint               # Run all linters (cppcheck, clang-tidy)"
	@echo "  make install-tools      # Install required development tools"

# Code formatting and linting targets

# Source files to format/lint (excluding external dependencies)
SOURCE_FILES = cloudflare_renew.c \
               tools/*.c \
               lib/*.c lib/*.h \
               tests/*.c

# Format all source code using clang-format
format:
	@echo "🎨 Formatting source code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name "*.c" -o -name "*.h" | grep -E "(lib|tools|tests|cloudflare_renew)" | xargs clang-format -i; \
		echo "✅ Code formatting complete"; \
	else \
		echo "❌ clang-format not found. Run 'make install-tools' first."; \
		exit 1; \
	fi

# Check if code is properly formatted
check-format:
	@echo "🔍 Checking code formatting..."
	@if command -v clang-format >/dev/null 2>&1; then \
		UNFORMATTED=$$(find . -name "*.c" -o -name "*.h" | grep -E "(lib|tools|tests|cloudflare_renew)" | xargs clang-format --dry-run --Werror 2>&1 | wc -l); \
		if [ $$UNFORMATTED -eq 0 ]; then \
			echo "✅ All code is properly formatted"; \
		else \
			echo "❌ Code formatting issues found. Run 'make format' to fix."; \
			find . -name "*.c" -o -name "*.h" | grep -E "(lib|tools|tests|cloudflare_renew)" | xargs clang-format --dry-run --Werror; \
			exit 1; \
		fi; \
	else \
		echo "❌ clang-format not found. Run 'make install-tools' first."; \
		exit 1; \
	fi

# Run static analysis and linting
lint: lint-cppcheck lint-clang-tidy

# Run cppcheck static analysis
lint-cppcheck:
	@echo "🔍 Running cppcheck static analysis..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all \
			--std=c99 \
			--platform=unix64 \
			--template=gcc \
			--verbose \
			--force \
			--error-exitcode=1 \
			--inline-suppr \
			--suppress=missingIncludeSystem \
			--suppress=unusedFunction:tests/* \
			--suppress=checkersReport \
			--suppress=normalCheckLevelMaxBranches \
			--suppress=unmatchedSuppression \
			--suppress=unreadVariable \
			-I lib/ \
			-I . \
			cloudflare_renew.c lib/ tools/ tests/; \
		echo "✅ cppcheck analysis complete"; \
	else \
		echo "❌ cppcheck not found. Run 'make install-tools' first."; \
		exit 1; \
	fi

# Run clang-tidy analysis
lint-clang-tidy:
	@echo "🔍 Running clang-tidy analysis..."
	@if command -v clang-tidy >/dev/null 2>&1; then \
		set -e; \
		for file in $$(find . -name "*.c" | grep -E "(lib|tools|cloudflare_renew)"); do \
			echo "Analyzing $$file..."; \
			clang-tidy $$file -- $(CFLAGS) -I. -I./lib; \
		done; \
		echo "✅ clang-tidy analysis complete"; \
	else \
		echo "❌ clang-tidy not found. Run 'make install-tools' first."; \
		exit 1; \
	fi

# Install development tools (macOS/Linux)
install-tools:
	@echo "🛠️  Installing development tools..."
	@if [[ "$$OSTYPE" == "darwin"* ]]; then \
		echo "Installing tools via Homebrew..."; \
		brew install llvm cppcheck || echo "Please install Homebrew first: https://brew.sh"; \
		echo "Note: You may need to add LLVM to your PATH:"; \
		echo "  export PATH=\"/opt/homebrew/opt/llvm/bin:\$$PATH\""; \
	elif command -v apt-get >/dev/null 2>&1; then \
		echo "Installing tools via apt-get..."; \
		sudo apt-get update && sudo apt-get install -y clang-format clang-tidy cppcheck; \
	elif command -v yum >/dev/null 2>&1; then \
		echo "Installing tools via yum..."; \
		sudo yum install -y clang-tools-extra cppcheck; \
	elif command -v dnf >/dev/null 2>&1; then \
		echo "Installing tools via dnf..."; \
		sudo dnf install -y clang-tools-extra cppcheck; \
	else \
		echo "❌ Unsupported package manager. Please install manually:"; \
		echo "  - clang-format"; \
		echo "  - clang-tidy"; \
		echo "  - cppcheck"; \
		exit 1; \
	fi
	@echo "✅ Development tools installation complete"
