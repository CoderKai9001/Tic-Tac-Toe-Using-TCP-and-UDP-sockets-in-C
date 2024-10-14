#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#define BUFF_LEN 4096
#define PORT     8080

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

    if(argc != 2) {
        printf(RED"args err\n"RESET);
        exit(EXIT_FAILURE);
    }

    const char* server_ip = argv[1];

    // *buffers used
    char* snd_msg = (char*)malloc(sizeof(char)*BUFF_LEN);
    char* rec_msg = (char*)malloc(sizeof(char)*BUFF_LEN);

    //* create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    //* set address to the socket
    struct sockaddr_in socket_address, server_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &socket_address.sin_addr);
    printf(RED"%u"RESET ,socket_address.sin_addr.s_addr);
    socklen_t socket_address_len = sizeof(socket_address);
    socklen_t server_address_len = sizeof(server_address);

    bind(sockfd, (struct sockaddr*) &socket_address, socket_address_len);
    bzero(snd_msg, BUFF_LEN);


    fd_set current_fds;

    int max_fd = (STDIN_FILENO > sockfd) ? STDIN_FILENO : sockfd;
    sprintf(snd_msg, "%u|%d|%d", socket_address.sin_addr.s_addr, socket_address.sin_port, socket_address.sin_family);
    printf("%s\n", snd_msg);
    sendto(sockfd, snd_msg, strlen(snd_msg)+1, 0, (struct sockaddr*) &socket_address, socket_address_len);

    while(1) {
        //* clear the descriptor set.
        FD_ZERO(&current_fds);

        //* add stdin and server socket to the descr set. 
        FD_SET(STDIN_FILENO, &current_fds);
        FD_SET(sockfd, &current_fds);

        //* select active fds
        if(select(max_fd+1, &current_fds, NULL, NULL, NULL) < 0) {
            printf(RED"select error!\n"RESET);
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(STDIN_FILENO, &current_fds)) { //? stdin active
            bzero(snd_msg, BUFF_LEN);
            read(STDIN_FILENO, snd_msg, BUFF_LEN);
            sendto(sockfd, snd_msg, strlen(snd_msg)+1, 0, (struct sockaddr*) &socket_address, socket_address_len);
            // printf("[["BLUE"sent: %s"RESET"]]\n", snd_msg);
        }
        if(FD_ISSET(sockfd, &current_fds)) { //? server-socket active 
            bzero(rec_msg, BUFF_LEN);
            recvfrom(sockfd, rec_msg, BUFF_LEN, 0, (struct sockaddr*) &server_address, &server_address_len);
            if(!(strncmp(rec_msg+7, "ERR", 3) == 0 || strncmp(rec_msg+7, "MSG", 3) == 0 || strncmp(rec_msg+7, "TERM", 4) == 0)) showBoard(rec_msg); // +7 accounts for color.
            else printf("[["GREEN"recieved: %s"RESET"]]\n", rec_msg); 
            if(strncmp(rec_msg+7, "TERM", 4) == 0) {
                close(sockfd);
                return 0;
            }
        }
    }

    return 0;
}