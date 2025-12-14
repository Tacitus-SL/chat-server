#include "server_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Определение глобальных массивов
Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

void init_clients(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].username[0] = '\0';
        clients[i].current_room[0] = '\0';
    }
}

void init_rooms(void) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        rooms[i].active = 0;
        rooms[i].name[0] = '\0';
        rooms[i].history.count = 0;
        rooms[i].history.head = 0;
    }
    // Создаём комнату "lobby" по умолчанию
    strcpy(rooms[0].name, "lobby");
    rooms[0].active = 1;
}

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%H:%M:%S", t);
}

int find_client_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

int find_client_by_username(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && strcmp(clients[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

int find_room(const char *name) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && strcmp(rooms[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int create_room(const char *name) {
    if (find_room(name) >= 0) return -1;

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].active) {
            strcpy(rooms[i].name, name);
            rooms[i].active = 1;
            return i;
        }
    }
    return -1;
}

void send_message(int client_fd, const char *msg) {
    send(client_fd, msg, strlen(msg), 0);
}

void broadcast_to_room(const char *room, const char *msg, int exclude_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && clients[i].fd != exclude_fd &&
            strcmp(clients[i].current_room, room) == 0) {
            send_message(clients[i].fd, msg);
        }
    }
}

void add_message_to_history(const char *room_name, const char *message) {
    int room_idx = find_room(room_name);
    if (room_idx < 0) return;

    MessageHistory *hist = &rooms[room_idx].history;

    // Копируем сообщение в циклический буфер
    strncpy(hist->messages[hist->head], message, BUFFER_SIZE - 1);
    hist->messages[hist->head][BUFFER_SIZE - 1] = '\0';

    hist->head = (hist->head + 1) % MAX_HISTORY;
    if (hist->count < MAX_HISTORY) {
        hist->count++;
    }
}

void send_room_history(int client_idx, const char *room_name) {
    int room_idx = find_room(room_name);
    if (room_idx < 0) return;

    MessageHistory *hist = &rooms[room_idx].history;

    if (hist->count == 0) {
        return; // Нет истории
    }

    send_message(clients[client_idx].fd, "[SERVER] --- Recent messages ---\n");

    // Отправляем сообщения в правильном порядке
    int start = (hist->head - hist->count + MAX_HISTORY) % MAX_HISTORY;
    for (int i = 0; i < hist->count; i++) {
        int idx = (start + i) % MAX_HISTORY;
        send_message(clients[client_idx].fd, hist->messages[idx]);
    }

    send_message(clients[client_idx].fd, "[SERVER] --- End of history ---\n");
}

void handle_setname(int client_idx, const char *username) {
    if (strlen(username) == 0 || strlen(username) >= MAX_USERNAME) {
        send_message(clients[client_idx].fd, "[ERROR] Invalid username length.\n");
        return;
    }

    if (find_client_by_username(username) >= 0) {
        send_message(clients[client_idx].fd, "[ERROR] Username already taken.\n");
        return;
    }

    strcpy(clients[client_idx].username, username);
    strcpy(clients[client_idx].current_room, "lobby");

    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "[SERVER] Welcome, %s! You are in 'lobby'. Type /help for commands.\n", username);
    send_message(clients[client_idx].fd, msg);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    snprintf(msg, sizeof(msg), "[%s] *** %s joined the lobby ***\n", timestamp, username);
    broadcast_to_room("lobby", msg, clients[client_idx].fd);
}

void handle_join(int client_idx, const char *room_name) {
    if (strlen(clients[client_idx].username) == 0) {
        send_message(clients[client_idx].fd, "[ERROR] Set username first with /name <username>\n");
        return;
    }

    if (strlen(room_name) == 0 || strlen(room_name) >= MAX_ROOMNAME) {
        send_message(clients[client_idx].fd, "[ERROR] Invalid room name.\n");
        return;
    }

    if (find_room(room_name) < 0) {
        if (create_room(room_name) < 0) {
            send_message(clients[client_idx].fd, "[ERROR] Cannot create room (server full).\n");
            return;
        }
    }

    char old_room[MAX_ROOMNAME];
    strcpy(old_room, clients[client_idx].current_room);

    char msg[BUFFER_SIZE];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    snprintf(msg, sizeof(msg), "[%s] *** %s left the room ***\n",
             timestamp, clients[client_idx].username);
    broadcast_to_room(old_room, msg, clients[client_idx].fd);

    strcpy(clients[client_idx].current_room, room_name);

    snprintf(msg, sizeof(msg), "[SERVER] You joined room '%s'\n", room_name);
    send_message(clients[client_idx].fd, msg);

    // Показываем историю сообщений
    send_room_history(client_idx, room_name);

    snprintf(msg, sizeof(msg), "[%s] *** %s joined the room ***\n",
             timestamp, clients[client_idx].username);
    broadcast_to_room(room_name, msg, clients[client_idx].fd);
    add_message_to_history(room_name, msg);
}

void handle_leave(int client_idx) {
    if (strlen(clients[client_idx].username) == 0) return;

    char msg[BUFFER_SIZE];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(msg, sizeof(msg), "[%s] *** %s left the room ***\n",
             timestamp, clients[client_idx].username);
    broadcast_to_room(clients[client_idx].current_room, msg, clients[client_idx].fd);

    strcpy(clients[client_idx].current_room, "lobby");
    send_message(clients[client_idx].fd, "[SERVER] You are back in lobby.\n");

    snprintf(msg, sizeof(msg), "[%s] *** %s joined the lobby ***\n",
             timestamp, clients[client_idx].username);
    broadcast_to_room("lobby", msg, clients[client_idx].fd);
}

void handle_list_rooms(int client_idx) {
    char msg[BUFFER_SIZE];
    strcpy(msg, "[SERVER] Available rooms:\n");

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active) {
            int count = 0;
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].fd > 0 && strcmp(clients[j].current_room, rooms[i].name) == 0) {
                    count++;
                }
            }
            char line[256];
            snprintf(line, sizeof(line), "  - %.31s (%d users)\n", rooms[i].name, count);
            strcat(msg, line);
        }
    }
    send_message(clients[client_idx].fd, msg);
}

void handle_list_users(int client_idx) {
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "[SERVER] Users in '%s':\n", clients[client_idx].current_room);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && strlen(clients[i].username) > 0 &&
            strcmp(clients[i].current_room, clients[client_idx].current_room) == 0) {
            char line[256];
            snprintf(line, sizeof(line), "  - %.31s\n", clients[i].username);
            strcat(msg, line);
        }
    }
    send_message(clients[client_idx].fd, msg);
}

void handle_private_message(int client_idx, const char *target, const char *content) {
    int target_idx = find_client_by_username(target);

    if (target_idx < 0) {
        send_message(clients[client_idx].fd, "[ERROR] User not found.\n");
        return;
    }

    char msg[BUFFER_SIZE];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(msg, sizeof(msg), "[%s] [PM from %s]: %s\n",
             timestamp, clients[client_idx].username, content);
    send_message(clients[target_idx].fd, msg);

    snprintf(msg, sizeof(msg), "[%s] [PM to %s]: %s\n",
             timestamp, target, content);
    send_message(clients[client_idx].fd, msg);
}

void handle_chat_message(int client_idx, const char *content) {
    if (strlen(clients[client_idx].username) == 0) {
        send_message(clients[client_idx].fd, "[ERROR] Set username first with /name <username>\n");
        return;
    }

    char msg[BUFFER_SIZE];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(msg, sizeof(msg), "[%s] %s: %s\n",
             timestamp, clients[client_idx].username, content);

    // Сохраняем в историю
    add_message_to_history(clients[client_idx].current_room, msg);

    // Отправляем всем в комнате, включая отправителя
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 &&
            strcmp(clients[i].current_room, clients[client_idx].current_room) == 0) {
            send_message(clients[i].fd, msg);
        }
    }
}

void handle_help(int client_idx) {
    char msg[BUFFER_SIZE];
    strcpy(msg, "[SERVER] Available commands:\n");
    strcat(msg, "  /name <username>        - Set your username\n");
    strcat(msg, "  /join <room>            - Join or create a room\n");
    strcat(msg, "  /leave                  - Leave current room (go to lobby)\n");
    strcat(msg, "  /rooms                  - List all rooms\n");
    strcat(msg, "  /users                  - List users in current room\n");
    strcat(msg, "  /msg <user> <message>   - Send private message\n");
    strcat(msg, "  /quit                   - Exit the chat\n");
    strcat(msg, "  /help                   - Show this help\n");
    send_message(clients[client_idx].fd, msg);
}

void handle_quit(int client_idx) {
    send_message(clients[client_idx].fd, "[SERVER] Goodbye! Disconnecting...\n");
    handle_disconnect(client_idx);
}

void handle_client_message(int client_idx, char *buffer) {
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

    if (buffer[0] == '/') {
        char *cmd = strtok(buffer, " ");

        if (strcmp(cmd, "/name") == 0) {
            char *username = strtok(NULL, " ");
            if (username) handle_setname(client_idx, username);
            else send_message(clients[client_idx].fd, "[ERROR] Usage: /name <username>\n");
        }
        else if (strcmp(cmd, "/join") == 0) {
            char *room = strtok(NULL, " ");
            if (room) handle_join(client_idx, room);
            else send_message(clients[client_idx].fd, "[ERROR] Usage: /join <room>\n");
        }
        else if (strcmp(cmd, "/leave") == 0) {
            handle_leave(client_idx);
        }
        else if (strcmp(cmd, "/rooms") == 0) {
            handle_list_rooms(client_idx);
        }
        else if (strcmp(cmd, "/users") == 0) {
            handle_list_users(client_idx);
        }
        else if (strcmp(cmd, "/msg") == 0) {
            char *target = strtok(NULL, " ");
            char *msg = strtok(NULL, "");
            if (target && msg) handle_private_message(client_idx, target, msg);
            else send_message(clients[client_idx].fd, "[ERROR] Usage: /msg <user> <message>\n");
        }
        else if (strcmp(cmd, "/help") == 0) {
            handle_help(client_idx);
        }
        else if (strcmp(cmd, "/quit") == 0) {
            handle_quit(client_idx);
        }
        else {
            send_message(clients[client_idx].fd, "[ERROR] Unknown command. Type /help for help.\n");
        }
    } else {
        handle_chat_message(client_idx, buffer);
    }
}

void handle_disconnect(int client_idx) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clients[client_idx].addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    int port = ntohs(clients[client_idx].addr.sin_port);

    if (strlen(clients[client_idx].username) > 0) {
        char msg[BUFFER_SIZE];
        char timestamp[32];
        get_timestamp(timestamp, sizeof(timestamp));

        snprintf(msg, sizeof(msg), "[%s] *** %s disconnected ***\n",
                 timestamp, clients[client_idx].username);
        broadcast_to_room(clients[client_idx].current_room, msg, -1);

        // Логирование на сервере
        printf("Lost connection from %s:%d (user: %s)\n", ip_str, port, clients[client_idx].username);
    } else {
        printf("Lost connection from %s:%d (no username set)\n", ip_str, port);
    }

    close(clients[client_idx].fd);
    clients[client_idx].fd = -1;
    clients[client_idx].username[0] = '\0';
    clients[client_idx].current_room[0] = '\0';
}