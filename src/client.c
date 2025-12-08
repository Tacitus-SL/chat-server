#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "protocol.h"

volatile sig_atomic_t running = 1;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    int port = 0;
    char *ip_addr = NULL;

    for (int i = 1; i < argc; i++) {
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

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", ip_addr);
        close(sock_fd);
        return 1;
    }

    printf("Connecting to %s:%d...\n", ip_addr, port);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to chat server!\n");
    printf("---------------------------------------\n");

    setvbuf(stdout, NULL, _IOLBF, 0);

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock_fd, &readfds);

        int max_fd = sock_fd > STDIN_FILENO ? sock_fd : STDIN_FILENO;
        struct timeval tv = {1, 0};

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (activity <= 0) continue;

        // Получение сообщений от сервера
        if (FD_ISSET(sock_fd, &readfds)) {
            char buffer[BUFFER_SIZE];
            int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) {
                printf("\n[DISCONNECTED] Connection to server lost.\n");
                break;
            }

            buffer[bytes] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }

        // Отправка сообщений на сервер
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char buffer[BUFFER_SIZE];

            if (!fgets(buffer, sizeof(buffer), stdin)) {
                continue;
            }

            // Отправляем сообщение на сервер
            if (send(sock_fd, buffer, strlen(buffer), 0) < 0) {
                perror("send");
                break;
            }
        }
    }

    printf("\nDisconnecting...\n");
    close(sock_fd);
    return 0;
}