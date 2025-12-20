#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include "protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>

// Структуры данных сервера
typedef struct {
    int fd;
    char username[MAX_USERNAME];
    char current_room[MAX_ROOMNAME];
    struct sockaddr_in addr;
    time_t last_activity;  // Время последней активности
	time_t last_typing_sent;
} Client;

typedef struct {
    char name[MAX_ROOMNAME];
    int active;
    MessageHistory history;  // История сообщений комнаты
} Room;

// Глобальные массивы (extern - объявление, определение будет в .c файле)
extern Client clients[MAX_CLIENTS];
extern Room rooms[MAX_ROOMS];

// Функции инициализации
void init_clients(void);
void init_rooms(void);

// Утилиты времени
void get_timestamp(char *buffer, size_t size);

// Поиск клиентов и комнат
int find_client_by_fd(int fd);
int find_client_by_username(const char *username);
int find_room(const char *name);
int create_room(const char *name);

// Отправка сообщений
int send_all(int fd, const char *buf, size_t len);
void send_message(int client_fd, const char *msg);
void broadcast_to_room(const char *room, const char *msg, int exclude_fd);

// Обработчики команд
void handle_setname(int client_idx, const char *username);
void handle_join(int client_idx, const char *room_name);
void handle_leave(int client_idx);
void handle_list_rooms(int client_idx);
void handle_list_users(int client_idx);
void handle_private_message(int client_idx, const char *target, const char *content);
void handle_chat_message(int client_idx, const char *content);
void handle_help(int client_idx);
void handle_quit(int client_idx);
void handle_client_message(int client_idx, char *buffer);
void handle_disconnect(int client_idx);

// Управление историей сообщений
void add_message_to_history(const char *room_name, const char *message);
void send_room_history(int client_idx, const char *room_name);

// Управление активностью клиентов
void update_client_activity(int client_idx);
void check_inactive_clients(void);

// Управление комнатами
int count_users_in_room(const char *room_name);
void cleanup_empty_rooms(void);

// Цвета пользователей
const char* get_user_color(const char *username);
unsigned int hash_string(const char *str);

#endif // SERVER_UTILS_H