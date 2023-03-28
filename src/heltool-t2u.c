#include "heltool-t2u.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <tcp_port> <udp_address> <udp_port>\n", argv[0]);
        return 1;
    }

    // create TCP socket and listen for incoming connections
    int tcp_socket, tcp_client_socket;
    struct sockaddr_in tcp_addr, tcp_client_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_addr.sin_port = htons(atoi(argv[1]));

    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    if (bind(tcp_socket, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(tcp_socket, 1) < 0) {
        perror("listen failed");
        return 1;
    }

    printf("Listening on TCP port %s\n", argv[1]);

    // create UDP socket and configure destination address
    int udp_socket;
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = inet_addr(argv[2]);
    udp_addr.sin_port = htons(atoi(argv[3]));

    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    printf("Forwarding to UDP address %s:%s\n", argv[2], argv[3]);

    // accept incoming TCP connections and forward packets to UDP socket
    char buffer[MAX_UDP_PACKET_SIZE];
    ssize_t len;

    while (1) {
        if ((tcp_client_socket = accept(tcp_socket, (struct sockaddr *)&tcp_client_addr, (socklen_t*)&len)) < 0) {
            perror("accept failed");
            break;
        }

        printf("Accepted incoming TCP connection from %s:%d\n", inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port));

        while ((len = recv(tcp_client_socket, buffer, sizeof(buffer), 0)) > 0) {
            if (sendto(udp_socket, buffer, len, 0, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
                perror("sendto failed");
                break;
            }
        }

        if (len < 0) {
            perror("recv failed");
            break;
        }

        printf("Closed incoming TCP connection from %s:%d\n", inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port));
        close(tcp_client_socket);
    }

    close(udp_socket);
    close(tcp_socket);

    return 0;
}
