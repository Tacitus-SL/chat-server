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
# --- ДОБАВЛЕНО: Объектные файлы для тестов ---
# unit_tests.o (из tests/) и server_utils.o (из src/)
UNIT_TEST_OBJ := $(BUILD_DIR)/unit_tests.o $(BUILD_DIR)/server_utils.o
# --- КОНЕЦ ДОБАВЛЕНИЯ ---

# Dependency files
SERVER_DEP := $(DEPS_DIR)/server.d $(DEPS_DIR)/server_utils.d
CLIENT_DEP := $(DEPS_DIR)/client.d
# --- ДОБАВЛЕНО: Файл зависимостей для тестов ---
UNIT_TEST_DEP := $(DEPS_DIR)/unit_tests.d
# --- КОНЕЦ ДОБАВЛЕНИЯ ---

# Target executables
SERVER := $(BUILD_DIR)/server
CLIENT := $(BUILD_DIR)/client
UNIT_TEST_BIN := $(BUILD_DIR)/unit_tests # Собираем тесты тоже в build/

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
	@mkdir -p $(BUILD_DIR) $(DEPS_DIR)

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

# --- ДОБАВЛЕНО: Правила для сборки и запуска тестов ---

# Цель для запуска всех тестов
test: unit-tests

# Цель для сборки и запуска unit-тестов
unit-tests: $(UNIT_TEST_BIN)
	@echo "$(GREEN)--- Running Unit Tests ---$(NC)"
	@./$(UNIT_TEST_BIN)

# Правило для линковки исполняемого файла тестов
$(UNIT_TEST_BIN): $(UNIT_TEST_OBJ)
	@echo "$(YELLOW)Linking unit tests...$(NC)"
	$(CC) $(UNIT_TEST_OBJ) -o $@ $(LDFLAGS)
	@echo "$(GREEN)✓ Unit tests compiled successfully!$(NC)"

# --- КОНЕЦ ДОБАВЛЕНИЯ ---

# Compile .c → .o (из папки src/)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -MF $(DEPS_DIR)/$*.d

# --- ДОБАВЛЕНО: Специальное правило для компиляции unit_tests.c из папки tests/ ---
$(BUILD_DIR)/unit_tests.o: $(UNIT_TEST_SRC) | dirs
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -MF $(DEPS_DIR)/unit_tests.d
# --- КОНЕЦ ДОБАВЛЕНИЯ ---


# Include automatically generated dependencies
-include $(SERVER_DEP) $(CLIENT_DEP) $(UNIT_TEST_DEP)

# Clean build artifacts
clean:
	@echo "$(YELLOW)Cleaning build artifacts...$(NC)"
	rm -rf $(BUILD_DIR) $(DEPS_DIR)
	@echo "$(GREEN)✓ Clean complete!$(NC)"

# Install executables to system
install: all
	@echo "$(YELLOW)Installing to $(INSTALL_DIR)...$(NC)"
	install -m 755 $(SERVER) $(INSTALL_DIR)/chat-server
	install -m 755 $(CLIENT) $(INSTALL_DIR)/chat-client
	@echo "$(GREEN)✓ Installation complete!$(NC)"

# Uninstall from system
uninstall:
	@echo "$(YELLOW)Uninstalling...$(NC)"
	rm -f $(INSTALL_DIR)/chat-server
	rm -f $(INSTALL_DIR)/chat-client
	@echo "$(GREEN)✓ Uninstall complete!$(NC)"

# Display help (без изменений, уже есть unit-tests)
help:
	# ... (твой help-блок)