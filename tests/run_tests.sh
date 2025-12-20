#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[38;5;33m'
NC='\033[0m'

SERVER_BIN="./build/server"
PORT=9999

echo -e "${BLUE}Running Integration Test: Server Ping...${NC}"

# 1. Check if the server binary exists
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Error: Server binary not found at $SERVER_BIN${NC}"
    echo "Please run 'make' first."
    exit 1
fi

# 2. Start the server in the background (&) and suppress standard output
$SERVER_BIN -p $PORT > /dev/null 2>&1 &
SERVER_PID=$!

sleep 1

# 3. Send /ping command via netcat (nc)
RESPONSE=$(echo "/ping" | nc localhost $PORT -w 1 2>/dev/null)

# 4. Cleanup: Kill the server process
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

# 5. Validate the response
if [[ "$RESPONSE" == *"PONG"* ]]; then
    echo -e "${GREEN}✓ SUCCESS: Server replied PONG${NC}"
    exit 0
else
    echo -e "${RED}✗ FAILURE: Server did not reply correctly${NC}"
    echo "Expected 'PONG', got: '$RESPONSE'"
    exit 1
fi