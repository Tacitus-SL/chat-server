#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_USERNAME 32
#define MAX_ROOMNAME 32
#define MAX_MESSAGE 512
#define MAX_CLIENTS 100
#define MAX_ROOMS 50
#define BUFFER_SIZE 4096
#define MAX_HISTORY 10  // Количество сообщений в истории на комнату

// Типы сообщений
typedef enum {
    MSG_SETNAME,      // Установить имя пользователя
    MSG_CHAT,         // Обычное сообщение в комнату
    MSG_PRIVATE,      // Личное сообщение
    MSG_JOIN,         // Присоединиться к комнате
    MSG_LEAVE,        // Покинуть комнату
    MSG_CREATE,       // Создать комнату
    MSG_LIST_ROOMS,   // Список комнат
    MSG_LIST_USERS,   // Список пользователей в комнате
    MSG_HELP,         // Помощь
    MSG_QUIT,         // Выход из чата
    MSG_SERVER_INFO,  // Информация от сервера
    MSG_ERROR         // Ошибка
} MessageType;

// Структура сообщения
typedef struct {
    MessageType type;
    char username[MAX_USERNAME];
    char room[MAX_ROOMNAME];
    char target[MAX_USERNAME];  // Для личных сообщений
    char content[MAX_MESSAGE];
    char timestamp[32];
} Message;

// Структура для истории сообщений
typedef struct {
    char messages[MAX_HISTORY][BUFFER_SIZE];
    int count;
    int head;  // Индекс для циклического буфера
} MessageHistory;

#endif // PROTOCOL_H