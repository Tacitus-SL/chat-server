#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * @file protocol.h
 * @brief Defines constants, structures, and message types for the chat protocol.
 */

#define MAX_USERNAME    32      /**< Maximum length of a username */
#define MAX_ROOMNAME    32      /**< Maximum length of a room name */
#define MAX_MESSAGE     512     /**< Maximum length of a single message */
#define MAX_CLIENTS     100     /**< Maximum number of concurrent clients */
#define MAX_ROOMS       50      /**< Maximum number of active rooms */
#define BUFFER_SIZE     4096    /**< Network buffer size */
#define MAX_HISTORY     10      /**< Number of messages stored in history per room */

/**
 * @brief Enumeration of supported message types.
 *
 * Defines the intent of the message being processed by the server or client.
 */
typedef enum {
    MSG_SETNAME,      /**< Request to set or change username */
    MSG_CHAT,         /**< Standard broadcast message to a room */
    MSG_PRIVATE,      /**< Private message to a specific user */
    MSG_JOIN,         /**< Request to join a room */
    MSG_LEAVE,        /**< Request to leave the current room */
    MSG_CREATE,       /**< Request to create a new room */
    MSG_LIST_ROOMS,   /**< Request list of active rooms */
    MSG_LIST_USERS,   /**< Request list of users in current room */
    MSG_HELP,         /**< Request help/command list */
    MSG_QUIT,         /**< Client disconnect request */
    MSG_SERVER_INFO,  /**< Informational message from server */
    MSG_ERROR         /**< Error message from server */
} MessageType;

/**
 * @brief Represents a structured message in the chat system.
 */
typedef struct {
    MessageType type;           /**< Type of the message */
    char username[MAX_USERNAME];/**< Sender's username */
    char room[MAX_ROOMNAME];    /**< Context room name */
    char target[MAX_USERNAME];  /**< Target username (for private messages) */
    char content[MAX_MESSAGE];  /**< The actual message text */
    char timestamp[32];         /**< Formatted timestamp string */
} Message;

/**
 * @brief Circular buffer for storing chat history.
 */
typedef struct {
    char messages[MAX_HISTORY][BUFFER_SIZE]; /**< Array of stored messages */
    int count;                               /**< Current number of stored messages */
    int head;                                /**< Index for the circular buffer head */
} MessageHistory;

#endif /* PROTOCOL_H */