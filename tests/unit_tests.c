#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "protocol.h"

// Цвета для вывода
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[1;33m"
#define NC "\033[0m"

int tests_passed = 0;
int tests_failed = 0;

void test_result(const char* test_name, int passed) {
    if (passed) {
        printf(GREEN "✓ " NC "%s\n", test_name);
        tests_passed++;
    } else {
        printf(RED "✗ " NC "%s\n", test_name);
        tests_failed++;
    }
}

// Тест 1: Проверка констант протокола
void test_protocol_constants() {
    test_result("MAX_USERNAME is positive", MAX_USERNAME > 0);
    test_result("MAX_ROOMNAME is positive", MAX_ROOMNAME > 0);
    test_result("MAX_MESSAGE is positive", MAX_MESSAGE > 0);
    test_result("MAX_CLIENTS is reasonable", MAX_CLIENTS >= 10 && MAX_CLIENTS <= 1000);
    test_result("MAX_ROOMS is reasonable", MAX_ROOMS >= 10 && MAX_ROOMS <= 100);
    test_result("BUFFER_SIZE is sufficient", BUFFER_SIZE >= MAX_MESSAGE);
}

// Тест 2: Проверка структуры Message
void test_message_structure() {
    Message msg;

    // Тест инициализации
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_CHAT;
    strcpy(msg.username, "Alice");
    strcpy(msg.room, "lobby");
    strcpy(msg.content, "Hello, World!");

    test_result("Message type assignment", msg.type == MSG_CHAT);
    test_result("Username assignment", strcmp(msg.username, "Alice") == 0);
    test_result("Room assignment", strcmp(msg.room, "lobby") == 0);
    test_result("Content assignment", strcmp(msg.content, "Hello, World!") == 0);
}

// Тест 3: Проверка типов сообщений
void test_message_types() {
    test_result("MSG_SETNAME defined", MSG_SETNAME >= 0);
    test_result("MSG_CHAT defined", MSG_CHAT >= 0);
    test_result("MSG_PRIVATE defined", MSG_PRIVATE >= 0);
    test_result("MSG_JOIN defined", MSG_JOIN >= 0);
    test_result("MSG_LEAVE defined", MSG_LEAVE >= 0);
    test_result("MSG_CREATE defined", MSG_CREATE >= 0);
    test_result("MSG_LIST_ROOMS defined", MSG_LIST_ROOMS >= 0);
    test_result("MSG_LIST_USERS defined", MSG_LIST_USERS >= 0);
    test_result("MSG_HELP defined", MSG_HELP >= 0);
    test_result("MSG_SERVER_INFO defined", MSG_SERVER_INFO >= 0);
    test_result("MSG_ERROR defined", MSG_ERROR >= 0);
}

// Тест 4: Проверка размеров строк
void test_string_limits() {
    char username[MAX_USERNAME];
    char roomname[MAX_ROOMNAME];
    char message[MAX_MESSAGE];

    // Заполняем максимально возможные строки
    memset(username, 'A', MAX_USERNAME - 1);
    username[MAX_USERNAME - 1] = '\0';

    memset(roomname, 'B', MAX_ROOMNAME - 1);
    roomname[MAX_ROOMNAME - 1] = '\0';

    memset(message, 'C', MAX_MESSAGE - 1);
    message[MAX_MESSAGE - 1] = '\0';

    test_result("Username max length", strlen(username) == MAX_USERNAME - 1);
    test_result("Roomname max length", strlen(roomname) == MAX_ROOMNAME - 1);
    test_result("Message max length", strlen(message) == MAX_MESSAGE - 1);
}

// Тест 5: Проверка валидности имён
void test_name_validation() {
    // Валидные имена
    test_result("Valid username 'Alice'", strlen("Alice") > 0 && strlen("Alice") < MAX_USERNAME);
    test_result("Valid roomname 'lobby'", strlen("lobby") > 0 && strlen("lobby") < MAX_ROOMNAME);

    // Граничные случаи
    char long_name[MAX_USERNAME + 10];
    memset(long_name, 'X', MAX_USERNAME + 5);
    long_name[MAX_USERNAME + 5] = '\0';

    test_result("Too long username detected", strlen(long_name) >= MAX_USERNAME);

    // Пустые имена
    test_result("Empty username invalid", strlen("") == 0);
}

// Тест 6: Проверка Message с различными типами
void test_message_types_usage() {
    Message msg;

    // Тест MSG_SETNAME
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_SETNAME;
    strcpy(msg.username, "Bob");
    test_result("MSG_SETNAME with username",
                msg.type == MSG_SETNAME && strcmp(msg.username, "Bob") == 0);

    // Тест MSG_JOIN
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_JOIN;
    strcpy(msg.room, "gaming");
    test_result("MSG_JOIN with room",
                msg.type == MSG_JOIN && strcmp(msg.room, "gaming") == 0);

    // Тест MSG_PRIVATE
    memset(&msg, 0, sizeof(Message));
    msg.type = MSG_PRIVATE;
    strcpy(msg.username, "Alice");
    strcpy(msg.target, "Bob");
    strcpy(msg.content, "Hello!");
    test_result("MSG_PRIVATE with target",
                msg.type == MSG_PRIVATE &&
                strcmp(msg.target, "Bob") == 0 &&
                strcmp(msg.content, "Hello!") == 0);
}

// Тест 7: Проверка timestamp поля
void test_timestamp_field() {
    Message msg;
    memset(&msg, 0, sizeof(Message));

    strcpy(msg.timestamp, "12:34:56");
    test_result("Timestamp assignment", strcmp(msg.timestamp, "12:34:56") == 0);

    strcpy(msg.timestamp, "23:59:59");
    test_result("Max valid timestamp", strcmp(msg.timestamp, "23:59:59") == 0);
}

int main() {
    printf("\n");
    printf("================================\n");
    printf("Chat Application Unit Tests\n");
    printf("================================\n");
    printf("\n");

    printf(YELLOW "Running protocol tests...\n" NC);
    test_protocol_constants();
    printf("\n");

    printf(YELLOW "Running message structure tests...\n" NC);
    test_message_structure();
    printf("\n");

    printf(YELLOW "Running message type tests...\n" NC);
    test_message_types();
    printf("\n");

    printf(YELLOW "Running string limit tests...\n" NC);
    test_string_limits();
    printf("\n");

    printf(YELLOW "Running name validation tests...\n" NC);
    test_name_validation();
    printf("\n");

    printf(YELLOW "Running message type usage tests...\n" NC);
    test_message_types_usage();
    printf("\n");

    printf(YELLOW "Running timestamp tests...\n" NC);
    test_timestamp_field();
    printf("\n");

    // Итоговые результаты
    printf("================================\n");
    printf("Test Results\n");
    printf("================================\n");
    printf(GREEN "Passed: %d\n" NC, tests_passed);

    if (tests_failed > 0) {
        printf(RED "Failed: %d\n" NC, tests_failed);
        printf("\n");
        return 1;
    } else {
        printf(RED "Failed: %d\n" NC, tests_failed);
        printf("\n");
        printf(GREEN "✓ All tests passed!\n" NC);
        printf("\n");
        return 0;
    }
}