#!/bin/bash

# Цвета
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}Chat Application - Quick Local Test${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""

# Проверка наличия файлов
echo -e "${YELLOW}Checking files...${NC}"

if [ ! -f "build/server" ]; then
    echo -e "${RED}✗ Server not found. Run 'make' first.${NC}"
    exit 1
fi

if [ ! -f "build/client" ]; then
    echo -e "${RED}✗ Client not found. Run 'make' first.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Server found${NC}"
echo -e "${GREEN}✓ Client found${NC}"
echo ""

# Запуск сервера
echo -e "${YELLOW}Starting server on port 8080...${NC}"
./build/server -p 8080 &
SERVER_PID=$!
sleep 1

# Проверка что сервер запустился
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}✗ Server failed to start${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Server running (PID: $SERVER_PID)${NC}"
echo ""

# Инструкции
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Server is running!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Open new terminals and run clients:"
echo ""
echo -e "${YELLOW}Terminal 2:${NC}"
echo "  ./build/client -p 8080 -a 127.0.0.1"
echo "  Then type: /name Alice"
echo "  Then type: Hello!"
echo ""
echo -e "${YELLOW}Terminal 3:${NC}"
echo "  ./build/client -p 8080 -a 127.0.0.1"
echo "  Then type: /name Bob"
echo "  Then type: Hi Alice!"
echo ""
echo -e "${YELLOW}Test commands:${NC}"
echo "  /help      - Show all commands"
echo "  /rooms     - List rooms"
echo "  /join test - Join 'test' room"
echo "  /users     - List users in room"
echo "  /msg Bob hello - Send private message"
echo ""
echo -e "${RED}Press Ctrl+C to stop the server${NC}"
echo ""

# Ждём сигнала остановки
trap "echo ''; echo 'Stopping server...'; kill $SERVER_PID 2>/dev/null; exit 0" INT TERM

# Держим скрипт запущенным
wait $SERVER_PID