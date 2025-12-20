#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include "protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

/**
 * @file server_utils.h
 * @brief Server utility functions and data structures.
 *
 * Contains declarations for client management, room logic, message handling,
 * and helper functions used by the main server loop.
 */

/**
 * @brief Represents a connected client.
 */
typedef struct {
    int fd;                         /**< Socket file descriptor */
    char username[MAX_USERNAME];    /**< Client's display name */
    char current_room[MAX_ROOMNAME];/**< Name of the room the client is currently in */
    struct sockaddr_in addr;        /**< Client's network address information */
    time_t last_activity;           /**< Timestamp of last action for timeout handling */
    time_t last_typing_sent;        /**< Timestamp of last "typing..." notification */
} Client;

/**
 * @brief Represents a chat room.
 */
typedef struct {
    char name[MAX_ROOMNAME];        /**< Name of the room */
    int active;                     /**< Flag: 1 if active, 0 if empty/unused */
    MessageHistory history;         /**< Rolling history of recent messages */
} Room;

/* --- Global State Arrays --- */
extern Client clients[MAX_CLIENTS];
extern Room rooms[MAX_ROOMS];

/* --- Initialization Functions --- */

/**
 * @brief Initializes the clients array, setting default values.
 */
void init_clients(void);

/**
 * @brief Initializes the rooms array and creates the default lobby.
 */
void init_rooms(void);

/* --- Time Utilities --- */

/**
 * @brief Generates a current timestamp string.
 *
 * @param buffer Buffer to store the timestamp.
 * @param size Size of the buffer.
 */
void get_timestamp(char *buffer, size_t size);

/* --- Lookup Functions --- */

/**
 * @brief Finds a client index by their socket file descriptor.
 *
 * @param fd The socket file descriptor.
 * @return Index in `clients` array, or -1 if not found.
 */
int find_client_by_fd(int fd);

/**
 * @brief Finds a client index by their username.
 *
 * @param username The username to search for.
 * @return Index in `clients` array, or -1 if not found.
 */
int find_client_by_username(const char *username);

/**
 * @brief Finds a room index by name.
 *
 * @param name The name of the room.
 * @return Index in `rooms` array, or -1 if not found.
 */
int find_room(const char *name);

/**
 * @brief Finds an empty slot and marks a new room as active.
 *
 * @param name Name of the new room.
 * @return Index of the new room, or -1 if max rooms reached or name exists.
 */
int create_room(const char *name);

/* --- Network / Messaging Functions --- */

/**
 * @brief Ensures all bytes of a buffer are sent over the socket.
 *
 * @param fd Socket file descriptor.
 * @param buf Data buffer.
 * @param len Length of data to send.
 * @return 0 on success, -1 on failure.
 */
int send_all(int fd, const char *buf, size_t len);

/**
 * @brief Wrapper to send a string message to a client.
 *
 * @param client_fd Client socket descriptor.
 * @param msg Null-terminated string to send.
 */
void send_message(int client_fd, const char *msg);

/**
 * @brief Broadcasts a message to all users in a specific room.
 *
 * @param room Name of the target room.
 * @param msg The message to broadcast.
 * @param exclude_fd File descriptor to exclude from broadcast (e.g., sender), or -1.
 */
void broadcast_to_room(const char *room, const char *msg, int exclude_fd);

/* --- Command Handlers --- */

/**
 * @brief Handles the /name command.
 * @param client_idx Index of the client.
 * @param username The requested username.
 */
void handle_setname(int client_idx, const char *username);

/**
 * @brief Handles the /join command.
 * @param client_idx Index of the client.
 * @param room_name The target room name.
 */
void handle_join(int client_idx, const char *room_name);

/**
 * @brief Handles the /leave command (returns user to lobby).
 * @param client_idx Index of the client.
 */
void handle_leave(int client_idx);

/**
 * @brief Handles the /rooms command.
 * @param client_idx Index of the client.
 */
void handle_list_rooms(int client_idx);

/**
 * @brief Handles the /users command.
 * @param client_idx Index of the client.
 */
void handle_list_users(int client_idx);

/**
 * @brief Handles private messaging (/msg).
 * @param client_idx Index of the sender.
 * @param target Username of the recipient.
 * @param content Message content.
 */
void handle_private_message(int client_idx, const char *target, const char *content);

/**
 * @brief Handles standard chat messages (broadcast).
 * @param client_idx Index of the sender.
 * @param content Message content.
 */
void handle_chat_message(int client_idx, const char *content);

/**
 * @brief Handles the /help command.
 * @param client_idx Index of the client.
 */
void handle_help(int client_idx);

/**
 * @brief Handles the /quit command.
 * @param client_idx Index of the client.
 */
void handle_quit(int client_idx);

/**
 * @brief Handles typing notifications (/typing).
 * @param client_idx Index of the client who is typing.
 */
void handle_typing(int client_idx);

/**
 * @brief Parses and routes raw input from a client.
 * @param client_idx Index of the client.
 * @param buffer Raw string buffer received from socket.
 */
void handle_client_message(int client_idx, char *buffer);

/**
 * @brief Handles client disconnection (cleanup).
 * @param client_idx Index of the client.
 */
void handle_disconnect(int client_idx);

/* --- History Management --- */

/**
 * @brief Adds a message to a room's history buffer.
 * @param room_name Target room.
 * @param message Message string.
 */
void add_message_to_history(const char *room_name, const char *message);

/**
 * @brief Sends stored history to a client (usually upon join).
 * @param client_idx Index of the client.
 * @param room_name Room to retrieve history from.
 */
void send_room_history(int client_idx, const char *room_name);

/* --- Activity & Cleanup --- */

/**
 * @brief Updates the last_activity timestamp for a client.
 * @param client_idx Index of the client.
 */
void update_client_activity(int client_idx);

/**
 * @brief Checks for inactive clients and disconnects them if timed out.
 */
void check_inactive_clients(void);

/**
 * @brief Counts number of users currently in a room.
 * @param room_name Name of the room.
 * @return Number of users.
 */
int count_users_in_room(const char *room_name);

/**
 * @brief Removes rooms that have zero users (except lobby).
 */
void cleanup_empty_rooms(void);

/* --- Utility --- */

/**
 * @brief Deterministically gets a color code based on username.
 * @param username The username string.
 * @return ANSI color code string.
 */
const char *get_user_color(const char *username);

/**
 * @brief Simple hash function for strings.
 * @param str Input string.
 * @return Hash value.
 */
unsigned int hash_string(const char *str);

#endif /* SERVER_UTILS_H */