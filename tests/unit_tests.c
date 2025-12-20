#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "protocol.h"
#include "server_utils.h"

/**
 * @file unit_tests.c
 * @brief Unit testing suite for the Chat Application server logic.
 *
 * This file tests the core logic of the server (room management, user handling,
 * history) without requiring a real network connection. It directly manipulates
 * the global state arrays `clients` and `rooms`.
 */

/* --- Output Colors --- */
#define GREEN   "\033[0;32m"
#define RED     "\033[0;31m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define NC      "\033[0m"

/* --- External Globals --- */
/* Accessing global state defined in server_utils.c */
extern Client clients[MAX_CLIENTS];
extern Room rooms[MAX_ROOMS];

/* --- Test Counters --- */
int tests_passed = 0;
int tests_failed = 0;

/**
 * @brief Prints the result of a single test case.
 *
 * @param test_name Description of the test.
 * @param passed 1 if test passed, 0 if failed.
 */
void test_result(const char *test_name, int passed) {
    if (passed) {
        printf(GREEN "✓ " NC "%s\n", test_name);
        tests_passed++;
    } else {
        printf(RED "✗ " NC "%s\n", test_name);
        tests_failed++;
    }
}

/**
 * @brief Resets the global server state before each test.
 */
void setup() {
    init_clients();
    init_rooms();
}

/* ========================================== */
/* PART 1: PROTOCOL TESTS                     */
/* ========================================== */

void test_protocol_constants() {
    test_result("MAX_USERNAME is positive", MAX_USERNAME > 0);
    test_result("MAX_ROOMNAME is positive", MAX_ROOMNAME > 0);
    test_result("MAX_MESSAGE is positive", MAX_MESSAGE > 0);
    test_result("MAX_CLIENTS is reasonable", MAX_CLIENTS >= 10);
    test_result("MAX_ROOMS is reasonable", MAX_ROOMS >= 10);
    test_result("BUFFER_SIZE is sufficient", BUFFER_SIZE >= MAX_MESSAGE);
}

void test_string_limits() {
    char username[MAX_USERNAME];
    memset(username, 'A', MAX_USERNAME - 1);
    username[MAX_USERNAME - 1] = '\0';
    test_result("Username max length check", strlen(username) == MAX_USERNAME - 1);
}

/* ========================================== */
/* PART 2: SERVER LOGIC TESTS                 */
/* ========================================== */

void test_init_state() {
    int clients_empty = 1;
    int i;
    setup();

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) clients_empty = 0;
    }
    test_result("Clients array initialized empty", clients_empty);
    test_result("Lobby room exists by default", strcmp(rooms[0].name, "lobby") == 0);
    test_result("Lobby is active", rooms[0].active == 1);
}

void test_setname_logic() {
    setup();

    /* Simulate a connected client at index 0 */
    clients[0].fd = 999; /* Fake socket */

    /* Attempt to set name. Note: This may print socket errors to console, which is expected. */
    handle_setname(0, "Alice");

    test_result("Handle_setname sets username", strcmp(clients[0].username, "Alice") == 0);
    test_result("User added to lobby automatically", strcmp(clients[0].current_room, "lobby") == 0);

    /* Attempt to take the same name with another client */
    clients[1].fd = 888;
    handle_setname(1, "Alice");

    test_result("Cannot take occupied username", strcmp(clients[1].username, "Alice") != 0);
}

void test_room_management() {
    int room_idx;
    int found_idx;
    int dup_idx;
    setup();

    /* 1. Create room */
    room_idx = create_room("gaming");
    test_result("Create_room returns valid index", room_idx != -1);
    test_result("Room name set correctly", strcmp(rooms[room_idx].name, "gaming") == 0);
    test_result("Room set to active", rooms[room_idx].active == 1);

    /* 2. Find room */
    found_idx = find_room("gaming");
    test_result("Find_room finds existing room", found_idx == room_idx);

    /* 3. Duplicate room */
    dup_idx = create_room("gaming");
    test_result("Cannot create duplicate room", dup_idx == -1);
}

void test_join_leave_logic() {
    setup();

    /* Preparation: Alice is in Lobby */
    clients[0].fd = 777;
    strcpy(clients[0].username, "Alice");
    strcpy(clients[0].current_room, "lobby");

    /* Create room and join */
    create_room("tech");
    handle_join(0, "tech");

    test_result("User moved to new room", strcmp(clients[0].current_room, "tech") == 0);

    /* Leave */
    handle_leave(0);
    test_result("User returned to lobby after leave", strcmp(clients[0].current_room, "lobby") == 0);
}

void test_history_logic() {
    int lobby_idx = 0; /* Lobby is always 0 */
    setup();

    add_message_to_history("lobby", "Message 1");
    add_message_to_history("lobby", "Message 2");

    test_result("History count incremented", rooms[lobby_idx].history.count == 2);
    test_result("Message 1 saved correctly", strcmp(rooms[lobby_idx].history.messages[0], "Message 1") == 0);
    test_result("Message 2 saved correctly", strcmp(rooms[lobby_idx].history.messages[1], "Message 2") == 0);
}

void test_find_client() {
    setup();
    clients[5].fd = 123;
    strcpy(clients[5].username, "Bob");

    test_result("Find client by FD", find_client_by_fd(123) == 5);
    test_result("Find client by Username", find_client_by_username("Bob") == 5);
    test_result("Find non-existent client returns -1", find_client_by_username("Ghost") == -1);
}

/* ========================================== */
/* MAIN ENTRY POINT                           */
/* ========================================== */

int main() {
    printf("\n");
    printf("================================\n");
    printf("  Chat Application Unit Tests   \n");
    printf("================================\n");

    printf(YELLOW "--- Protocol Constants Tests ---\n" NC);
    test_protocol_constants();
    test_string_limits();
    printf("\n");

    printf(YELLOW "--- Server Initialization Tests ---\n" NC);
    test_init_state();
    printf("\n");

    printf(YELLOW "--- User Management Tests ---\n" NC);
    test_setname_logic();
    test_find_client();
    printf("\n");

    printf(YELLOW "--- Room Management Tests ---\n" NC);
    test_room_management();
    test_join_leave_logic();
    printf("\n");

    printf(YELLOW "--- History Tests ---\n" NC);
    test_history_logic();
    printf("\n");

    /* Final Results */
    printf("================================\n");
    printf("Test Results\n");
    printf("================================\n");
    printf(GREEN "Passed: %d\n" NC, tests_passed);

    if (tests_failed > 0) {
        printf(RED "Failed: %d\n" NC, tests_failed);
        printf("\n");
        printf(RED "Some tests failed. Check your logic!\n" NC);
        return 1;
    } else {
        printf(RED "Failed: %d\n" NC, tests_failed);
        printf("\n");
        printf(GREEN "✓ All tests passed successfully!\n" NC);
        printf("\n");
        return 0;
    }
}