#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "protocol.h"
#include "server_utils.h"
#include "colors.h"

/**
 * @file server.c
 * @brief Main entry point for the Chat Server.
 *
 * This file handles the TCP socket initialization, the main event loop using select(),
 * accepting new connections, and routing data between clients and the server logic.
 */

/**
 * @brief Global flag to control the main loop execution.
 * Modified by the signal handler to initiate graceful shutdown.
 */
volatile sig_atomic_t running = 1;

/**
 * @brief Handles system signals (like SIGINT/Ctrl+C).
 *
 * @param sig The signal number.
 */
void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

/**
 * @brief Parse command line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param port Pointer to store port number.
 * @return 0 on success, -1 on failure.
 */
int parse_arguments(int argc, char *argv[], int *port) {
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            *port = atoi(argv[i + 1]);
            i++;
        }
    }

    if (*port == 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return -1;
    }

    return 0;
}

/**
 * @brief Create and configure server socket.
 *
 * @param port Port number to bind to.
 * @return Socket file descriptor on success, -1 on failure.
 */
int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;

    /* Create Server Socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    /* Set socket options to reuse address */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }

    /* Bind socket to port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    /* Start listening */
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    printf("Chat server started on port %d\n", port);
    printf("Waiting for connections...\n");

    return server_fd;
}

/**
 * @brief Setup file descriptor set for select().
 *
 * @param readfds Pointer to fd_set to configure.
 * @param server_fd Server socket file descriptor.
 * @return Maximum file descriptor number.
 */
int setup_fd_set(fd_set *readfds, int server_fd) {
    int max_fd = server_fd;
    int i;

    FD_ZERO(readfds);
    FD_SET(server_fd, readfds);

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) {
            FD_SET(clients[i].fd, readfds);
            if (clients[i].fd > max_fd) {
                max_fd = clients[i].fd;
            }
        }
    }

    return max_fd;
}

/**
 * @brief Handle maintenance tasks (periodic cleanup).
 */
void handle_maintenance(void) {
    check_inactive_clients();
    cleanup_empty_rooms();
}

/**
 * @brief Handle new client connection.
 *
 * @param server_fd Server socket file descriptor.
 */
void handle_new_connection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

    if (client_fd < 0) {
        return;
    }

    int added = 0;
    int j;
    char msg[BUFFER_SIZE];

    for (j = 0; j < MAX_CLIENTS; j++) {
        if (clients[j].fd == -1) {
            clients[j].fd = client_fd;
            clients[j].addr = client_addr;
            clients[j].last_activity = time(NULL);
            added = 1;

            snprintf(msg, sizeof(msg),
                     COLOR_SERVER "[SERVER] Connected to chat server. Set your username with /name <username>" COLOR_RESET "\n");
            send_message(client_fd, msg);

            printf("New connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
            break;
        }
    }

    if (!added) {
        char msg[] = COLOR_ERROR "[ERROR] Server is full." COLOR_RESET "\n";
        send(client_fd, msg, strlen(msg), 0);
        close(client_fd);
    }
}

/**
 * @brief Handle messages from all connected clients.
 *
 * @param readfds Pointer to fd_set with ready file descriptors.
 */
void handle_client_messages(fd_set *readfds) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && FD_ISSET(clients[i].fd, readfds)) {
            char buffer[BUFFER_SIZE];
            int bytes = recv(clients[i].fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) {
                handle_disconnect(i);
            } else {
                buffer[bytes] = '\0';
                handle_client_message(i, buffer);
            }
        }
    }
}

/**
 * @brief Main server event loop.
 *
 * @param server_fd Server socket file descriptor.
 */
void run_server_loop(int server_fd) {
    int loop_count = 0;

    while (running) {
        fd_set readfds;
        int max_fd;
        struct timeval tv = {1, 0};
        int activity;

        max_fd = setup_fd_set(&readfds, server_fd);

        /* Wait for activity on sockets */
        activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        /* Timeout handling (Maintenance tasks) */
        if (activity <= 0) {
            loop_count++;
            /* Check inactive clients and clean rooms every ~10 seconds */
            if (loop_count >= 10) {
                handle_maintenance();
                loop_count = 0;
            }
            continue;
        }

        /* Handle New Connection */
        if (FD_ISSET(server_fd, &readfds)) {
            handle_new_connection(server_fd);
        }

        /* Handle Client Messages */
        handle_client_messages(&readfds);
    }
}

/**
 * @brief Cleanup and shutdown server.
 *
 * @param server_fd Server socket file descriptor.
 */
void shutdown_server(int server_fd) {
    int i;

    printf("\nShutting down server...\n");

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) {
            close(clients[i].fd);
        }
    }

    close(server_fd);
}

/**
 * @brief Main server function.
 *
 * Initializes the server socket, binds to the specified port, and enters
 * the main event loop to handle client connections and messages.
 *
 * @param argc Argument count.
 * @param argv Argument vector (expects -p <port>).
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    int port = 0;
    int server_fd;

    /* Parse command line arguments */
    if (parse_arguments(argc, argv, &port) < 0) {
        return 1;
    }

    /* Setup signal handlers */
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    /* Initialize internal structures */
    init_clients();
    init_rooms();

    /* Create and configure server socket */
    server_fd = create_server_socket(port);
    if (server_fd < 0) {
        return 1;
    }

    /* Run main server loop */
    run_server_loop(server_fd);

    /* Cleanup and shutdown */
    shutdown_server(server_fd);

    return 0;
}