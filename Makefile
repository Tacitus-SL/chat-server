CC := gcc
CFLAGS := -Iinclude -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS :=

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
DEPS_DIR := deps
TEST_DIR := tests

# Source files (автоматический поиск)
SERVER_SRC := $(SRC_DIR)/server.c
CLIENT_SRC := $(SRC_DIR)/client.c
UTILS_SRC := $(SRC_DIR)/server_utils.c
HEADERS := $(wildcard $(INC_DIR)/*.h) $(wildcard $(SRC_DIR)/*.h)

# Object files
SERVER_OBJ := $(BUILD_DIR)/server.o $(BUILD_DIR)/server_utils.o
CLIENT_OBJ := $(BUILD_DIR)/client.o

# Dependency files
SERVER_DEP := $(DEPS_DIR)/server.d $(DEPS_DIR)/server_utils.d
CLIENT_DEP := $(DEPS_DIR)/client.d

# Dependencies (for manual reference)
$(SERVER_OBJ): $(SERVER_SRC) $(HEADERS)
$(CLIENT_OBJ): $(CLIENT_SRC) $(HEADERS)

# Target executables
SERVER := $(BUILD_DIR)/server
CLIENT := $(BUILD_DIR)/client

# Test files
UNIT_TEST_SRC := $(TEST_DIR)/unit_tests.c
UNIT_TEST_BIN := $(TEST_DIR)/unit_tests
INTEGRATION_TEST := $(TEST_DIR)/run_tests.sh

# Installation directory
INSTALL_DIR := /usr/local/bin

# Colors for output
GREEN := \033[0;32m
YELLOW := \033[1;33m
RED := \033[0;31m
NC := \033[0m# No Color

# Default target
.DEFAULT_GOAL := all

# Phony targets
.PHONY: all clean test unit-tests integration-tests install uninstall help dirs

# Main targets
all: dirs $(SERVER) $(CLIENT)
	@echo "$(GREEN)✓ Build complete!$(NC)"
	@echo "  Server: $(SERVER)"
	@echo "  Client: $(CLIENT)"

# Create necessary directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(DEPS_DIR)
	@mkdir -p $(TEST_DIR)

# Build server
$(SERVER): $(SERVER_OBJ)
	@echo "$(YELLOW)Linking server...$(NC)"
	$(CC) $(SERVER_OBJ) -o $@ $(LDFLAGS)
	@echo "$(GREEN)✓ Server compiled successfully!$(NC)"

# Build client
$(CLIENT): $(CLIENT_OBJ)
	@echo "$(YELLOW)Linking client...$(NC)"
	$(CC) $(CLIENT_OBJ) -o $@ $(LDFLAGS)
	@echo "$(GREEN)✓ Client compiled successfully!$(NC)"

# Compile .c → .o with automatic dependency generation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -MF $(DEPS_DIR)/$*.d

# Include automatically generated dependencies
-include $(SERVER_DEP) $(CLIENT_DEP)

# Clean build artifacts
clean:
	@echo "$(YELLOW)Cleaning build artifacts...$(NC)"
	rm -rf $(BUILD_DIR) $(DEPS_DIR) $(UNIT_TEST_BIN)
	@echo "$(GREEN)✓ Clean complete!$(NC)"

# Install executables to system
install: all
	@echo "$(YELLOW)Installing to $(INSTALL_DIR)...$(NC)"
	install -m 755 $(SERVER) $(INSTALL_DIR)/chat-server
	install -m 755 $(CLIENT) $(INSTALL_DIR)/chat-client
	@echo "$(GREEN)✓ Installation complete!$(NC)"
	@echo "  Usage: chat-server -p <port>"
	@echo "  Usage: chat-client -p <port> -a <address>"

# Uninstall from system
uninstall:
	@echo "$(YELLOW)Uninstalling...$(NC)"
	rm -f $(INSTALL_DIR)/chat-server
	rm -f $(INSTALL_DIR)/chat-client
	@echo "$(GREEN)✓ Uninstall complete!$(NC)"
	@echo "Note: Run 'hash -r' to clear bash command cache"

# Display help
help:
	@echo "$(GREEN)Chat Application Makefile$(NC)"
	@echo ""
	@echo "Available targets:"
	@echo "  $(YELLOW)make$(NC)              - Build server and client (default)"
	@echo "  $(YELLOW)make all$(NC)          - Same as 'make'"
	@echo "  $(YELLOW)make clean$(NC)        - Remove build artifacts"
	@echo "  $(YELLOW)make test$(NC)         - Run all tests"
	@echo "  $(YELLOW)make unit-tests$(NC)   - Run unit tests only"
	@echo "  $(YELLOW)make integration-tests$(NC) - Run integration tests only"
	@echo "  $(YELLOW)make install$(NC)      - Install to $(INSTALL_DIR)"
	@echo "  $(YELLOW)make uninstall$(NC)    - Remove from system"
	@echo "  $(YELLOW)make debug$(NC)        - Build with debug symbols"
	@echo "  $(YELLOW)make release$(NC)      - Build optimized release"
	@echo "  $(YELLOW)make analyze$(NC)      - Run static analysis (requires cppcheck)"
	@echo "  $(YELLOW)make format$(NC)       - Format code (requires clang-format)"
	@echo "  $(YELLOW)make valgrind$(NC)     - Instructions for memory leak check"
	@echo "  $(YELLOW)make help$(NC)         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  $(YELLOW)./$(SERVER) -p 8080$(NC)"
	@echo "  $(YELLOW)./$(CLIENT) -p 8080 -a 127.0.0.1$(NC)"

# Show build information
info:
	@echo "$(GREEN)Build Configuration:$(NC)"
	@echo "  CC:        $(CC)"
	@echo "  CFLAGS:    $(CFLAGS)"
	@echo "  LDFLAGS:   $(LDFLAGS)"
	@echo "  SRC_DIR:   $(SRC_DIR)"
	@echo "  BUILD_DIR: $(BUILD_DIR)"
	@echo "  DEPS_DIR:  $(DEPS_DIR)"
	@echo ""
	@echo "$(GREEN)Source Files:$(NC)"
	@echo "  Server:    $(SERVER_SRC)"
	@echo "  Client:    $(CLIENT_SRC)"
	@echo ""
	@echo "$(GREEN)Output Files:$(NC)"
	@echo "  Server:    $(SERVER)"
	@echo "  Client:    $(CLIENT)"