#!/bin/bash

# Цвета
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[38;5;33m'
NC='\033[0m'

SERVER_BIN="./build/server"
PORT=9999 # Используем другой порт, чтобы не мешать, если сервер уже запущен вручную

echo -e "${BLUE}Running Integration Test: Server Ping...${NC}"

# 1. Проверяем наличие файла
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Error: Server binary not found!${NC}"
    exit 1
fi

# 2. Запускаем сервер тихо (в фоновом режиме)
$SERVER_BIN -p $PORT > /dev/null 2>&1 &
SERVER_PID=$!

# Даем серверу секунду на запуск
sleep 1

# 3. Отправляем команду /ping через nc (netcat) и ловим ответ
# Флаг -w 1 означает "ждать ответа не больше 1 секунды"
RESPONSE=$(echo "/ping" | nc localhost $PORT -w 1)

# 4. Сразу убиваем сервер, тест окончен
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

# 5. Проверяем, ответил ли сервер "PONG"
if [[ "$RESPONSE" == *"PONG"* ]]; then
    echo -e "${GREEN}✓ SUCCESS: Server replied PONG${NC}"
    exit 0
else
    echo -e "${RED}✗ FAILURE: Server did not reply correctly${NC}"
    echo "Got: '$RESPONSE'"
    exit 1
fi