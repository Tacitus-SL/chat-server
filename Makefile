CC := gcc
CFLAGS := -Iinclude -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS :=

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
DEPS_DIR := deps
TEST_DIR := tests

# Source files
SERVER_SRC := $(SRC_DIR)/server.c
CLIENT_SRC := $(SRC_DIR)/client.c
UTILS_SRC := $(SRC_DIR)/server_utils.c
UNIT_TEST_SRC := $(TEST_DIR)/unit_tests.c
HEADERS := $(wildcard $(INC_DIR)/*.h) $(wildcard $(SRC_DIR)/*.h)

# Object files
SERVER_OBJ := $(BUILD_DIR)/server.o $(BUILD_DIR)/server_utils.o
CLIENT_OBJ := $(BUILD_DIR)/client.o
UNIT_TEST_OBJ := $(BUILD_DIR)/unit_tests.o $(BUILD_DIR)/server_utils.o

# Dependency files
SERVER_DEP := $(DEPS_DIR)/server.d $(DEPS_DIR)/server_utils.d
CLIENT_DEP := $(DEPS_DIR)/client.d
UNIT_TEST_DEP := $(DEPS_DIR)/unit_tests.d

# Target executables
SERVER := $(BUILD_DIR)/server
CLIENT := $(BUILD_DIR)/client
UNIT_TEST_BIN := $(BUILD_DIR)/unit_tests

# Installation directory
INSTALL_DIR := /usr/local/bin

# Colors for output
GREEN := \033[0;32m
YELLOW := \033[1;33m
RED := \033[0;31m
BLUE := \033[38;5;33m
NC := \033[0m

# Default target
.DEFAULT_GOAL := all

# Phony targets
.PHONY: all clean test unit-tests integration-tests install uninstall help dirs

# Main targets
all: dirs $(SERVER) $(CLIENT)
	@echo "$(GREEN)✓ Build complete!$(NC)"

# Create necessary directories
dirs:
	@mkdir -p $(BUILD_DIR) $(DEPS_DIR)

# --- BUILD RULES ---

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

# Compile .c -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -MF $(DEPS_DIR)/$*.d

# --- TESTING RULES ---

test: unit-tests integration-tests

# 1. Unit Tests
unit-tests: $(UNIT_TEST_BIN)
	@echo "$(BLUE)Running Unit Tests...$(NC)"
	@./$(UNIT_TEST_BIN)
	@echo ""

# 2. Integration Tests
integration-tests: $(SERVER)
	@chmod +x tests/run_tests.sh
	@bash tests/run_tests.sh
	@echo ""

$(UNIT_TEST_BIN): $(UNIT_TEST_OBJ)
	@echo "$(YELLOW)Linking unit tests...$(NC)"
	$(CC) $(UNIT_TEST_OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/unit_tests.o: $(UNIT_TEST_SRC) | dirs
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -MF $(DEPS_DIR)/unit_tests.d

-include $(SERVER_DEP) $(CLIENT_DEP) $(UNIT_TEST_DEP)

# --- CLEAN ---

clean:
	@echo "$(YELLOW)Cleaning build artifacts...$(NC)"
	rm -rf $(BUILD_DIR) $(DEPS_DIR)
	@echo "$(GREEN)✓ Clean complete!$(NC)"

# --- INSTALL / UNINSTALL ---

install: all
	@echo "$(YELLOW)Installing system-wide to $(INSTALL_DIR)...$(NC)"
	install -m 755 $(SERVER) $(INSTALL_DIR)/chat-server
	install -m 755 $(CLIENT) $(INSTALL_DIR)/chat-client
	@echo "$(GREEN)✓ Installation complete!$(NC)"
	@echo "  Run server: chat-server -p <port>"
	@echo "  Run client: chat-client -p <port> -a <address>"

uninstall:
	@echo "$(YELLOW)Uninstalling from $(INSTALL_DIR)...$(NC)"
	rm -f $(INSTALL_DIR)/chat-server
	rm -f $(INSTALL_DIR)/chat-client
	@echo "$(GREEN)✓ Uninstall complete!$(NC)"

# --- HELP ---

help:
	@echo "$(BLUE)Chat Application Makefile$(NC)"
	@echo ""
	@echo "Available commands:"
	@echo "  $(YELLOW)make$(NC)                 - Build project"
	@echo "  $(YELLOW)make clean$(NC)           - Remove build files"
	@echo "  $(YELLOW)make test$(NC)            - Run ALL tests (Unit + Integration)"
	@echo ""
	@echo "Installation:"
	@echo "  $(YELLOW)sudo make install$(NC)    - Install system-wide"
	@echo "  $(YELLOW)sudo make uninstall$(NC)  - Uninstall"