#include "server_utils.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @file server_utils.c
 * @brief Implementation of helper functions for the Chat Server.
 *
 * Contains logic for client/room management, command handling,
 * message broadcasting, and history maintenance.
 */

/* --- Global Definitions --- */
Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

const char *USER_COLORS[10] = {
    COLOR_USER_1, COLOR_USER_2, COLOR_USER_3, COLOR_USER_4, COLOR_USER_5,
    COLOR_USER_6, COLOR_USER_7, COLOR_USER_8, COLOR_USER_9, COLOR_USER_10
};

/* --- Initialization --- */

/**
 * @brief Initializes the global clients array.
 * Sets file descriptors to -1 and clears usernames.
 */
void init_clients(void) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].username[0] = '\0';
        clients[i].current_room[0] = '\0';
        clients[i].last_activity = 0;
        clients[i].last_typing_sent = 0;
    }
}

/**
 * @brief Initializes the global rooms array.
 * Sets up the default "lobby" room and marks others as inactive.
 */
void init_rooms(void) {
    int i;
    for (i = 0; i < MAX_ROOMS; i++) {
        rooms[i].active = 0;
        rooms[i].name[0] = '\0';
        rooms[i].history.count = 0;
        rooms[i].history.head = 0;
    }
    /* Create default "lobby" */
    strcpy(rooms[0].name, "lobby");
    rooms[0].active = 1;
}

/* --- Helpers --- */

/**
 * @brief Generates a current timestamp string.
 *
 * @param buffer Buffer to store the timestamp string.
 * @param size Size of the buffer.
 */
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%H:%M:%S", t);
}

/**
 * @brief Finds a client index by their socket file descriptor.
 *
 * @param fd The file descriptor to search for.
 * @return Index in clients array, or -1 if not found.
 */
int find_client_by_fd(int fd) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

/**
 * @brief Finds a client index by their username.
 *
 * @param username The username string to search for.
 * @return Index in clients array, or -1 if not found.
 */
int find_client_by_username(const char *username) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && strcmp(clients[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Finds a room index by its name.
 *
 * @param name The name of the room.
 * @return Index in rooms array, or -1 if not found/inactive.
 */
int find_room(const char *name) {
    int i;
    for (i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active && strcmp(rooms[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Creates a new room if space is available.
 *
 * @param name The name of the new room.
 * @return Index of the new room, or -1 if full or exists.
 */
int create_room(const char *name) {
    int i;
    if (find_room(name) >= 0) return -1;

    for (i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].active) {
            strncpy(rooms[i].name, name, MAX_ROOMNAME - 1);
            rooms[i].name[MAX_ROOMNAME - 1] = '\0';
            rooms[i].active = 1;
            return i;
        }
    }
    return -1;
}

/* --- Networking --- */

/**
 * @brief Reliable send function to ensure all bytes are transmitted.
 *
 * @param fd Socket file descriptor.
 * @param buf Buffer containing data to send.
 * @param len Length of data to send.
 * @return 0 on success, -1 on failure.
 */
int send_all(int fd, const char *buf, size_t len) {
    size_t total_sent = 0;
    size_t bytes_left = len;
    ssize_t n;

    while (total_sent < len) {
        n = send(fd, buf + total_sent, bytes_left, 0);
        if (n == -1) {
            return -1;
        }
        total_sent += n;
        bytes_left -= n;
    }
    return 0;
}

/**
 * @brief Helper to send a null-terminated string to a client.
 *
 * @param client_fd Client socket file descriptor.
 * @param msg The message string.
 */
void send_message(int client_fd, const char *msg) {
    size_t len = strlen(msg);
    if (send_all(client_fd, msg, len) == -1) {
        perror("send_all failed");
    }
}

/**
 * @brief Broadcasts a message to all users in a specific room.
 *
 * @param room Name of the room.
 * @param msg Message content.
 * @param exclude_fd File descriptor to exclude from broadcast (e.g., sender), or -1.
 */
void broadcast_to_room(const char *room, const char *msg, int exclude_fd) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && clients[i].fd != exclude_fd &&
            strcmp(clients[i].current_room, room) == 0) {
            send_message(clients[i].fd, msg);
        }
    }
}

/* --- History --- */

/**
 * @brief Adds a message to the circular history buffer of a room.
 *
 * @param room_name Name of the room.
 * @param message The message string to store.
 */
void add_message_to_history(const char *room_name, const char *message) {
    int room_idx = find_room(room_name);
    MessageHistory *hist;

    if (room_idx < 0) return;

    hist = &rooms[room_idx].history;

    strncpy(hist->messages[hist->head], message, BUFFER_SIZE - 1);
    hist->messages[hist->head][BUFFER_SIZE - 1] = '\0';

    hist->head = (hist->head + 1) % MAX_HISTORY;
    if (hist->count < MAX_HISTORY) {
        hist->count++;
    }
}

/**
 * @brief Sends the recent chat history of a room to a specific client.
 *
 * @param client_idx Index of the client receiving history.
 * @param room_name Name of the room.
 */
void send_room_history(int client_idx, const char *room_name) {
    int room_idx = find_room(room_name);
    MessageHistory *hist;
    int start, i;

    if (room_idx < 0) return;

    hist = &rooms[room_idx].history;

    if (hist->count == 0) {
        return;
    }

    send_message(clients[client_idx].fd, COLOR_SYSTEM "[SERVER] --- Recent messages ---" COLOR_RESET "\n");

    start = (hist->head - hist->count + MAX_HISTORY) % MAX_HISTORY;
    for (i = 0; i < hist->count; i++) {
        int idx = (start + i) % MAX_HISTORY;
        send_message(clients[client_idx].fd, hist->messages[idx]);
    }

    send_message(clients[client_idx].fd, COLOR_SYSTEM "[SERVER] --- End of history ---" COLOR_RESET "\n");
}

/* --- Handlers --- */

/**
 * @brief Updates the last activity timestamp for a client.
 *
 * @param client_idx Index of the client.
 */
void update_client_activity(int client_idx) {
    clients[client_idx].last_activity = time(NULL);
}

/**
 * @brief Handles the /name command to set or change username.
 *
 * @param client_idx Index of the client.
 * @param username The requested username.
 */
void handle_setname(int client_idx, const char *username) {
    char msg[BUFFER_SIZE];
    char timestamp[32];

    if (strlen(username) == 0 || strlen(username) >= MAX_USERNAME) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Invalid username length." COLOR_RESET "\n");
        return;
    }

    if (find_client_by_username(username) >= 0) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Username already taken." COLOR_RESET "\n");
        return;
    }

    strncpy(clients[client_idx].username, username, MAX_USERNAME - 1);
    clients[client_idx].username[MAX_USERNAME - 1] = '\0';

    strncpy(clients[client_idx].current_room, "lobby", MAX_ROOMNAME - 1);
    clients[client_idx].current_room[MAX_ROOMNAME - 1] = '\0';

    update_client_activity(client_idx);

    snprintf(msg, sizeof(msg), COLOR_SERVER "[SERVER] Welcome, %s%s%s! You are in 'lobby'. Type /help for commands." COLOR_RESET "\n",
             get_user_color(username), username, COLOR_SERVER);
    send_message(clients[client_idx].fd, msg);

    send_room_history(client_idx, "lobby");

    get_timestamp(timestamp, sizeof(timestamp));
    snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET COLOR_ACTION " *** %s%s%s joined the lobby ***" COLOR_RESET "\n",
             timestamp, get_user_color(username), username, COLOR_ACTION);
    broadcast_to_room("lobby", msg, clients[client_idx].fd);
    add_message_to_history("lobby", msg);
}

/**
 * @brief Handles the /join command to enter a room.
 *
 * @param client_idx Index of the client.
 * @param room_name Name of the room to join.
 */
void handle_join(int client_idx, const char *room_name) {
    char old_room[MAX_ROOMNAME];
    char msg[BUFFER_SIZE];
    char timestamp[32];
    const char *user_color;

    if (strlen(clients[client_idx].username) == 0) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Set username first with /name <username>" COLOR_RESET "\n");
        return;
    }

    if (strlen(room_name) == 0 || strlen(room_name) >= MAX_ROOMNAME) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Invalid room name." COLOR_RESET "\n");
        return;
    }

    update_client_activity(client_idx);

    if (find_room(room_name) < 0) {
        if (create_room(room_name) < 0) {
            send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Cannot create room (server full)." COLOR_RESET "\n");
            return;
        }
    }

    /* Notify old room */
    strncpy(old_room, clients[client_idx].current_room, MAX_ROOMNAME - 1);
    old_room[MAX_ROOMNAME - 1] = '\0';

    get_timestamp(timestamp, sizeof(timestamp));
    user_color = get_user_color(clients[client_idx].username);

    snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET COLOR_ACTION " *** %s%s%s left the room ***" COLOR_RESET "\n",
             timestamp, user_color, clients[client_idx].username, COLOR_ACTION);
    broadcast_to_room(old_room, msg, clients[client_idx].fd);

    strncpy(clients[client_idx].current_room, room_name, MAX_ROOMNAME - 1);
    clients[client_idx].current_room[MAX_ROOMNAME - 1] = '\0';

    snprintf(msg, sizeof(msg), COLOR_SERVER "[SERVER] You joined room '%s'" COLOR_RESET "\n", room_name);
    send_message(clients[client_idx].fd, msg);

    send_room_history(client_idx, room_name);

    snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET COLOR_ACTION " *** %s%s%s joined the room ***" COLOR_RESET "\n",
             timestamp, user_color, clients[client_idx].username, COLOR_ACTION);
    broadcast_to_room(room_name, msg, clients[client_idx].fd);
    add_message_to_history(room_name, msg);

    cleanup_empty_rooms();
}

/**
 * @brief Handles the /leave command.
 *
 * @param client_idx Index of the client.
 */
void handle_leave(int client_idx) {
    if (strlen(clients[client_idx].username) == 0) return;

    update_client_activity(client_idx);

    if (strcmp(clients[client_idx].current_room, "lobby") == 0) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] You are already in lobby." COLOR_RESET "\n");
        return;
    }

    /* Simulate joining lobby */
    handle_join(client_idx, "lobby");
}

/**
 * @brief Counts the number of users in a specific room.
 *
 * @param room_name Name of the room.
 * @return Number of users.
 */
int count_users_in_room(const char *room_name) {
    int count = 0;
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && strcmp(clients[i].current_room, room_name) == 0) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Handles the /rooms command to list available rooms.
 *
 * @param client_idx Index of the client.
 */
void handle_list_rooms(int client_idx) {
    char msg[BUFFER_SIZE];
    int i;
    int count;
    char line[256];

    update_client_activity(client_idx);

    msg[0] = '\0';
    strncat(msg, COLOR_SERVER "[SERVER] Available rooms:" COLOR_RESET "\n", BUFFER_SIZE - 1);

    for (i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active) {
            count = count_users_in_room(rooms[i].name);
            snprintf(line, sizeof(line), COLOR_INFO "  - %.31s" COLOR_RESET " (%d users)\n", rooms[i].name, count);

            if (strlen(msg) + strlen(line) < BUFFER_SIZE) {
                strcat(msg, line);
            }
        }
    }
    send_message(clients[client_idx].fd, msg);
}

/**
 * @brief Checks for inactive clients and disconnects them.
 */
void check_inactive_clients(void) {
    time_t now = time(NULL);
    const time_t timeout = 300;
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && clients[i].last_activity > 0) {
            if (now - clients[i].last_activity > timeout) {
                printf("Client timeout: %s (inactive for %ld s)\n",
                       clients[i].username[0] ? clients[i].username : "unnamed",
                       (long)(now - clients[i].last_activity));

                send_message(clients[i].fd, "[SERVER] Disconnected due to inactivity.\n");
                handle_disconnect(i);
            }
        }
    }
}

/**
 * @brief Deactivates empty rooms (except lobby).
 */
void cleanup_empty_rooms(void) {
    int i;
    for (i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].active) {
            if (strcmp(rooms[i].name, "lobby") == 0) continue;

            if (count_users_in_room(rooms[i].name) == 0) {
                printf("Cleaning up empty room: '%s'\n", rooms[i].name);
                rooms[i].active = 0;
                rooms[i].name[0] = '\0';
                rooms[i].history.count = 0;
                rooms[i].history.head = 0;
            }
        }
    }
}

/**
 * @brief Simple hash function for string to integer.
 *
 * @param str The string to hash.
 * @return The hash value.
 */
unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/**
 * @brief Gets a consistent color for a username based on hash.
 *
 * @param username The username.
 * @return String containing ANSI color code.
 */
const char *get_user_color(const char *username) {
    unsigned int hash = hash_string(username);
    return USER_COLORS[hash % USER_COLORS_COUNT];
}

/**
 * @brief Handles the /users command to list users in the current room.
 *
 * @param client_idx Index of the client.
 */
void handle_list_users(int client_idx) {
    char msg[BUFFER_SIZE];
    int i;
    char line[256];
    const char *user_color;

    update_client_activity(client_idx);

    snprintf(msg, sizeof(msg), COLOR_SERVER "[SERVER] Users in '%s':" COLOR_RESET "\n", clients[client_idx].current_room);

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && strlen(clients[i].username) > 0 &&
            strcmp(clients[i].current_room, clients[client_idx].current_room) == 0) {

            user_color = get_user_color(clients[i].username);
            snprintf(line, sizeof(line), "  - %s%.31s" COLOR_RESET "\n", user_color, clients[i].username);

            if (strlen(msg) + strlen(line) < BUFFER_SIZE) {
                strcat(msg, line);
            }
        }
    }
    send_message(clients[client_idx].fd, msg);
}

/**
 * @brief Handles private messaging (/msg).
 *
 * @param client_idx Index of the sender.
 * @param target Username of the recipient.
 * @param content Message content.
 */
void handle_private_message(int client_idx, const char *target, const char *content) {
    int target_idx;
    char msg[BUFFER_SIZE];
    char timestamp[32];
    const char *sender_color;
    const char *target_color;

    update_client_activity(client_idx);

    target_idx = find_client_by_username(target);

    if (target_idx < 0) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] User not found." COLOR_RESET "\n");
        return;
    }

    get_timestamp(timestamp, sizeof(timestamp));
    sender_color = get_user_color(clients[client_idx].username);
    target_color = get_user_color(target);

    snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET COLOR_PM " [PM from %s%s" COLOR_PM "]: " COLOR_RESET "%s\n",
             timestamp, sender_color, clients[client_idx].username, content);
    send_message(clients[target_idx].fd, msg);

    snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET COLOR_PM " [PM to %s%s" COLOR_PM "]: " COLOR_RESET "%s\n",
             timestamp, target_color, target, content);
    send_message(clients[client_idx].fd, msg);
}

/**
 * @brief Handles standard chat messages (broadcast to room).
 *
 * @param client_idx Index of the sender.
 * @param content Message content.
 */
void handle_chat_message(int client_idx, const char *content) {
    char msg[BUFFER_SIZE];
    char timestamp[32];
    const char *user_color;
    int i;

    if (strlen(clients[client_idx].username) == 0) {
        send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Set username first with /name <username>" COLOR_RESET "\n");
        return;
    }

    update_client_activity(client_idx);

    get_timestamp(timestamp, sizeof(timestamp));
    user_color = get_user_color(clients[client_idx].username);

    snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET " %s%s" COLOR_RESET ": %s\n",
             timestamp, user_color, clients[client_idx].username, content);

    add_message_to_history(clients[client_idx].current_room, msg);

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 &&
            strcmp(clients[i].current_room, clients[client_idx].current_room) == 0) {
            send_message(clients[i].fd, msg);
        }
    }
}

/**
 * @brief Sends the help menu to the client.
 *
 * @param client_idx Index of the client.
 */
void handle_help(int client_idx) {
    char msg[BUFFER_SIZE];
    msg[0] = '\0';
    strncat(msg, COLOR_SERVER "[SERVER] Available commands:" COLOR_RESET "\n", BUFFER_SIZE - 1);
    strncat(msg, COLOR_INFO "  /name <username>        " COLOR_RESET "- Set your username\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /join <room>            " COLOR_RESET "- Join or create a room\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /leave                  " COLOR_RESET "- Leave current room (go to lobby)\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /rooms                  " COLOR_RESET "- List all rooms\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /users                  " COLOR_RESET "- List users in current room\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /msg <user> <message>   " COLOR_RESET "- Send private message\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /quit                   " COLOR_RESET "- Exit the chat\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /ping                   " COLOR_RESET "- Check server responsiveness\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /typing                 " COLOR_RESET "- Send typing notification\n", BUFFER_SIZE - strlen(msg) - 1);
    strncat(msg, COLOR_INFO "  /help                   " COLOR_RESET "- Show this help\n", BUFFER_SIZE - strlen(msg) - 1);
    send_message(clients[client_idx].fd, msg);
}

/**
 * @brief Handles client disconnection request.
 *
 * @param client_idx Index of the client.
 */
void handle_quit(int client_idx) {
    send_message(clients[client_idx].fd, COLOR_SERVER "[SERVER] Goodbye! Disconnecting..." COLOR_RESET "\n");
    handle_disconnect(client_idx);
}

/**
 * @brief Handles server ping request (health check).
 *
 * @param client_idx Index of the client.
 */
void handle_ping(int client_idx) {
    char msg[BUFFER_SIZE];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    snprintf(msg, sizeof(msg), COLOR_SUCCESS "[SERVER] PONG [%s]" COLOR_RESET "\n", timestamp);
    send_message(clients[client_idx].fd, msg);
}

/**
 * @brief Handles typing notifications.
 *
 * @param client_idx Index of the client.
 */
void handle_typing(int client_idx) {
    time_t now;
    char msg[BUFFER_SIZE];
    const char *user_color;

    if (strlen(clients[client_idx].username) == 0)
    {
        return;
    }

    update_client_activity(client_idx);

    now = time(NULL);
    if (now - clients[client_idx].last_typing_sent < 3) {
        return;
    }

    clients[client_idx].last_typing_sent = now;
    user_color = get_user_color(clients[client_idx].username);

    snprintf(msg, sizeof(msg),
             COLOR_INFO "\x1b[3m ... %s%s%s is typing ... \x1b[0m" COLOR_RESET "\n",
             user_color, clients[client_idx].username, COLOR_INFO);

    broadcast_to_room(clients[client_idx].current_room, msg, clients[client_idx].fd);
}

/**
 * @brief Processes incoming raw text from a client.
 * Parses commands or routes to chat handler.
 *
 * @param client_idx Index of the client.
 * @param buffer The raw message buffer.
 */
void handle_client_message(int client_idx, char *buffer) {
    size_t len = strlen(buffer);
    char *cmd;
    char *arg1;
    char *arg2;

    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

    if (buffer[0] == '/') {
        cmd = strtok(buffer, " ");

        if (strcmp(cmd, "/name") == 0) {
            arg1 = strtok(NULL, " ");
            if (arg1) {
                handle_setname(client_idx, arg1);
            } else {
                send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Usage: /name <username>" COLOR_RESET "\n");
            }
        } else if (strcmp(cmd, "/join") == 0) {
            arg1 = strtok(NULL, " ");
            if (arg1) {
                handle_join(client_idx, arg1);
            } else {
                send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Usage: /join <room>" COLOR_RESET "\n");
            }
        } else if (strcmp(cmd, "/leave") == 0) {
            handle_leave(client_idx);
        } else if (strcmp(cmd, "/rooms") == 0) {
            handle_list_rooms(client_idx);
        } else if (strcmp(cmd, "/users") == 0) {
            handle_list_users(client_idx);
        } else if (strcmp(cmd, "/msg") == 0) {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, "");
            if (arg1 && arg2) {
                handle_private_message(client_idx, arg1, arg2);
            } else {
                send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Usage: /msg <user> <message>" COLOR_RESET "\n");
            }
        } else if (strcmp(cmd, "/help") == 0) {
            handle_help(client_idx);
        } else if (strcmp(cmd, "/quit") == 0) {
            handle_quit(client_idx);
        } else if (strcmp(cmd, "/ping") == 0) {
            handle_ping(client_idx);
        } else if (strcmp(cmd, "/typing") == 0) {
            handle_typing(client_idx);
        } else {
            send_message(clients[client_idx].fd, COLOR_ERROR "[ERROR] Unknown command. Type /help for help." COLOR_RESET "\n");
        }
    } else {
        handle_chat_message(client_idx, buffer);
    }
}

/**
 * @brief Handles logic for when a client disconnects.
 * Notifies room and cleans up resources.
 *
 * @param client_idx Index of the client.
 */
void handle_disconnect(int client_idx) {
    char ip_str[INET_ADDRSTRLEN];
    int port;
    char msg[BUFFER_SIZE];
    char timestamp[32];
    const char *user_color;

    inet_ntop(AF_INET, &(clients[client_idx].addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    port = ntohs(clients[client_idx].addr.sin_port);

    if (strlen(clients[client_idx].username) > 0) {
        get_timestamp(timestamp, sizeof(timestamp));
        user_color = get_user_color(clients[client_idx].username);

        snprintf(msg, sizeof(msg), COLOR_TIMESTAMP "[%s]" COLOR_RESET COLOR_ACTION " *** %s%s%s disconnected ***" COLOR_RESET "\n",
                 timestamp, user_color, clients[client_idx].username, COLOR_ACTION);
        broadcast_to_room(clients[client_idx].current_room, msg, -1);

        printf("Lost connection from %s:%d (user: %s)\n", ip_str, port, clients[client_idx].username);
    } else {
        printf("Lost connection from %s:%d (no username set)\n", ip_str, port);
    }

    close(clients[client_idx].fd);
    clients[client_idx].fd = -1;
    clients[client_idx].username[0] = '\0';
    clients[client_idx].current_room[0] = '\0';

    cleanup_empty_rooms();
}