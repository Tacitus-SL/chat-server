#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "protocol.h"

/**
 * @file client.c
 * @brief TCP Client implementation for the chat application.
 *
 * Handles server connection, sending user input, and displaying incoming messages
 * while maintaining a clean console interface.
 */

/**
 * @brief Global flag to control the main loop execution.
 * Toggled to 0 when SIGINT (Ctrl+C) is received.
 */
volatile sig_atomic_t running = 1;

/**
 * @brief Signal handler for graceful shutdown.
 *
 * @param sig The signal number (e.g., SIGINT).
 */
void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

/**
 * @brief Main entry point for the chat client.
 *
 * @param argc Argument count.
 * @param argv Argument vector (expects -p <port> and -a <address>).
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    int port = 0;
    char *ip_addr = NULL;
    int sock_fd;
    struct sockaddr_in server_addr;
    int i;

    /* Parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            ip_addr = argv[i + 1];
            i++;
        }
    }

    if (port == 0 || ip_addr == NULL) {
        fprintf(stderr, "Usage: %s -p <port> -a <ip_address>\n", argv[0]);
        return 1;
    }

    /* Setup signal handling */
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    /* Create socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", ip_addr);
        close(sock_fd);
        return 1;
    }

    printf("Connecting to %s:%d...\n", ip_addr, port);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to chat server!\n");
    printf("---------------------------------------\n");

    /* Main event loop */
    while (running) {
        fd_set readfds;
        int max_fd;
        int activity;
        struct timeval tv = {1, 0};

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock_fd, &readfds);

        max_fd = (sock_fd > STDIN_FILENO) ? sock_fd : STDIN_FILENO;

        activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (activity <= 0) continue;

        /* --- Incoming Message from Server --- */
        if (FD_ISSET(sock_fd, &readfds)) {
            char buffer[BUFFER_SIZE];
            int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) {
                printf("\n[DISCONNECTED] Connection to server lost.\n");
                break;
            }
            buffer[bytes] = '\0';

            printf("\r%s", buffer);

            if (buffer[bytes - 1] != '\n') {
                printf("\n");
            }

            printf("> ");
            fflush(stdout);
        }

        /* --- User Input from Keyboard --- */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char buffer[BUFFER_SIZE];
            if (!fgets(buffer, sizeof(buffer), stdin)) {
                continue;
            }

            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }

            /* Only send non-empty messages */
            if (strlen(buffer) > 0) {
                strcat(buffer, "\n");
                if (send(sock_fd, buffer, strlen(buffer), 0) < 0) {
                    perror("send");
                    break;
                }
            }
        }
    }

    close(sock_fd);
    return 0;
}