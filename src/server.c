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
 * @brief Main server function.
 *
 * @param argc Argument count.
 * @param argv Argument vector (expects -p <port>).
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    int port = 0;
    int i;
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    int loop_count = 0;

    /* Parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        }
    }

    if (port == 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return 1;
    }

    /* Setup signal handlers */
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    /* Initialize internal structures */
    init_clients();
    init_rooms();

    /* Create Server Socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Set socket options to reuse address (prevents "Address already in use" errors) */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    /* Bind socket to port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    /* Start listening */
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Chat server started on port %d\n", port);
    printf("Waiting for connections...\n");

    /* Main Server Loop */
    while (running) {
        fd_set readfds;
        int max_fd = server_fd;
        struct timeval tv = {1, 0};
        int activity;

        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) {
                    max_fd = clients[i].fd;
                }
            }
        }

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
                check_inactive_clients();
                cleanup_empty_rooms();
                loop_count = 0;
            }
            continue;
        }

        /* Handle New Connection */
        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

            if (client_fd >= 0) {
                int added = 0;
                int j;
                for (j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].fd == -1) {
                        clients[j].fd = client_fd;
                        clients[j].addr = client_addr;
                        clients[j].last_activity = time(NULL);
                        added = 1;

                        char msg[BUFFER_SIZE];
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
        }

        /* Handle Client Messages */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0 && FD_ISSET(clients[i].fd, &readfds)) {
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

    /* Cleanup and Shutdown */
    printf("\nShutting down server...\n");
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0) {
            close(clients[i].fd);
        }
    }
    close(server_fd);
    return 0;
}