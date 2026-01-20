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
#include "colors.h"

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
 * @brief Parse command line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param port Pointer to store port number.
 * @param ip_addr Pointer to store IP address.
 * @return 0 on success, -1 on failure.
 */
int parse_arguments(int argc, char *argv[], int *port, char **ip_addr) {
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            *port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            *ip_addr = argv[i + 1];
            i++;
        }
    }

    if (*port == 0 || *ip_addr == NULL) {
        fprintf(stderr, "Usage: %s -p <port> -a <ip_address>\n", argv[0]);
        return -1;
    }

    return 0;
}

/**
 * @brief Create and configure a socket.
 *
 * @return Socket file descriptor on success, -1 on failure.
 */
int create_socket(void) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
    }
    return sock_fd;
}

/**
 * @brief Connect to the chat server.
 *
 * @param sock_fd Socket file descriptor.
 * @param ip_addr Server IP address.
 * @param port Server port.
 * @return 0 on success, -1 on failure.
 */
int connect_to_server(int sock_fd, const char *ip_addr, int port) {
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", ip_addr);
        return -1;
    }

    printf("Connecting to %s:%d...\n", ip_addr, port);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return -1;
    }

    printf("Connected to chat server!\n");
    printf("---------------------------------------\n");

    return 0;
}

/**
 * @brief Handle incoming message from server.
 *
 * @param sock_fd Socket file descriptor.
 * @return 0 to continue, -1 to disconnect.
 */
int handle_server_message(int sock_fd) {
    char buffer[BUFFER_SIZE];
    int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
        printf("\n[DISCONNECTED] Connection to server lost.\n");
        return -1;
    }

    buffer[bytes] = '\0';
    printf("\r%s", buffer);

    if (buffer[bytes - 1] != '\n') {
        printf("\n");
    }

    printf("> ");
    fflush(stdout);

    return 0;
}

/**
 * @brief Clear remaining input from stdin after overflow.
 */
void clear_input_buffer(void) {
    int c;
    while (1) {
        c = getchar();
        if (c == '\n' || c == EOF) {
            break;
        }
    }
}

/**
 * @brief Handle user input from keyboard.
 *
 * @param sock_fd Socket file descriptor.
 * @return 0 to continue, -1 on error.
 */
int handle_user_input(int sock_fd) {
    char buffer[BUFFER_SIZE];

    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }

    size_t len = strlen(buffer);

    /* Check for buffer overflow */
    if (len > 0 && buffer[len - 1] != '\n') {
        printf(COLOR_ERROR "\n[WARNING] Message too long! Maximum length is %d characters.\n" COLOR_RESET,
               BUFFER_SIZE - 2);
        printf("> ");
        fflush(stdout);
        clear_input_buffer();
        return 0;
    }

    /* Remove trailing newline */
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    /* Only send non-empty messages */
    if (strlen(buffer) > 0) {
        strcat(buffer, "\n");
        if (send(sock_fd, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Main event loop for the chat client.
 *
 * @param sock_fd Socket file descriptor.
 */
void run_event_loop(int sock_fd) {
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

        if (activity <= 0) {
            continue;
        }

        /* Handle incoming message from server */
        if (FD_ISSET(sock_fd, &readfds)) {
            if (handle_server_message(sock_fd) < 0) {
                break;
            }
        }

        /* Handle user input from keyboard */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (handle_user_input(sock_fd) < 0) {
                break;
            }
        }
    }
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

    /* Parse command line arguments */
    if (parse_arguments(argc, argv, &port, &ip_addr) < 0) {
        return 1;
    }

    /* Setup signal handling */
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    /* Create socket */
    sock_fd = create_socket();
    if (sock_fd < 0) {
        return 1;
    }

    /* Connect to server */
    if (connect_to_server(sock_fd, ip_addr, port) < 0) {
        close(sock_fd);
        return 1;
    }

    /* Run main event loop */
    run_event_loop(sock_fd);

    close(sock_fd);
    return 0;
}