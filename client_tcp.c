#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#define PORT     8081
#define BUFF_LEN 4096

#define GREEN "\033[0;32m"
#define BLUE  "\033[0;34m"
#define WHITE "\033[0;37m"
#define RED   "\033[0;31m"
#define RESET "\033[0m"

void showBoard(char* game_state) {
    for(int i = 0; i < 3; i++) {
        printf("\t");
        for(int j = 3*i; j < 3*(i+1); j++) {
            if(game_state[j] == '0')  printf("(%d,%d)", i+1, j-3*i+1);
            else if(game_state[j] == '1') printf("  X  ");
            else if(game_state[j] == '2') printf("  O  ");
            if(j != 3*(i+1)-1)  printf("|");
        }
        if(i != 2) printf("\n\t-----|-----|-----");
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    // *buffers used
    char* rec_msg = (char*)malloc(sizeof(char)*BUFF_LEN);
    char* snd_msg = (char*)malloc(sizeof(char)*BUFF_LEN);

    // *create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // *set socket address
    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &sock_addr.sin_addr);
    socklen_t sock_addr_length = sizeof(sock_addr);

    // *connect
    connect(sockfd, (struct sockaddr*) &sock_addr, sock_addr_length);

    fd_set current_sockets;
    int max_fd = STDIN_FILENO > sockfd ? STDERR_FILENO : sockfd;
    for(;;) {
        // *initialise the sockets.
        FD_ZERO(&current_sockets);

        // *add the sockets to the set.
        FD_SET(STDIN_FILENO, &current_sockets);
        FD_SET(sockfd, &current_sockets);

        if(select(max_fd+1, &current_sockets, NULL, NULL, NULL) < 0) {
            printf("select error\n");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(STDIN_FILENO, &current_sockets)) {
            read(STDIN_FILENO, snd_msg, BUFF_LEN);
            send(sockfd, snd_msg, strlen(snd_msg)+1, 0);
        }
        if(FD_ISSET(sockfd, &current_sockets)) {
            recv(sockfd, rec_msg, BUFF_LEN, 0);
            printf("ack: %s\n", rec_msg);
            if(!(strncmp(rec_msg+7, "ERR", 3) == 0 || strncmp(rec_msg+7, "MSG", 3) == 0 || strncmp(rec_msg+7, "TERM", 4) == 0)) showBoard(rec_msg); // +7 accounts for color.
            if(strncmp(rec_msg+7, "TERM", 4) == 0) {
                close(sockfd);
                return 0;
            }
        }
    }
    return 0;
}