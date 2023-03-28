#include "heltool-u2t.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_UDP_PACKET_SIZE 65507

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <udp_port> <tcp_address> <tcp_port>\n", argv[0]);
        return 1;
    }

    // create UDP socket
    int udp_socket;
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(atoi(argv[1]));

    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    if (bind(udp_socket, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    printf("Listening on UDP port %s\n", argv[1]);

    // create TCP socket and connect to destination host and port
    int tcp_socket;
    struct sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = inet_addr(argv[2]);
    tcp_addr.sin_port = htons(atoi(argv[3]));

    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    if (connect(tcp_socket, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    printf("Connected to %s:%s via TCP\n", argv[2], argv[3]);

    // forward UDP packets to TCP socket
    char buffer[MAX_UDP_PACKET_SIZE];
    ssize_t len;

    while (1) {
        len = recvfrom(udp_socket, buffer, sizeof(buffer), 0, NULL, NULL);

        if (len < 0) {
            perror("recvfrom failed");
            break;
        }

        if (send(tcp_socket, buffer, len, 0) < 0) {
            perror("send failed");
            break;
        }
    }

    close(tcp_socket);
    close(udp_socket);

    return 0;
}

/*
int main(int argc, char *argv[]) {
    int udp_socket, tcp_socket, new_socket;
    struct sockaddr_in udp_addr, tcp_addr, cli_addr;
    int cli_len = sizeof(cli_addr);
    char buffer[1024];

    // create UDP socket
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(UDP_PORT);

    // bind UDP socket
    if (bind(udp_socket, (const struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // create TCP socket
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(TCP_PORT);

    // bind TCP socket
    if (bind(tcp_socket, (const struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for TCP connections
    if (listen(tcp_socket, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Waiting for UDP packet...\n");

        // receive UDP packet
        int len = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &cli_len);

        if (len < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        printf("Received UDP packet from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        // create TCP connection to destination host and port
        if ((new_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = cli_addr.sin_addr.s_addr;
        dest_addr.sin_port = htons(atoi(buffer));

        if (connect(new_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }

        printf("Connected to %s:%s via TCP\n", inet_ntoa(dest_addr.sin_addr), buffer);

        // forward UDP packet to TCP socket
        if (send(new_socket, buffer, strlen(buffer), 0) < 0) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }

        // forward data from TCP socket to UDP socket
        while (1) {
            len = recv(new_socket, buffer, sizeof(buffer), 0);

            if (len < 0) {
                perror("recv failed");
                exit(EXIT_FAILURE);
            }

            if (len == 0) {
                printf("TCP connection closed by peer\n");
                break;
            }

            if (sendto(udp_socket, buffer, len, 0, (struct sockaddr *)&cli_addr, cli_len) < 0) {
                perror("sendto failed");
                exit(EXIT_FAILURE);
            }
        }

        close(new_socket);
    }

    close(tcp_socket);
    close(udp_socket);

    return 0;
}
*/
