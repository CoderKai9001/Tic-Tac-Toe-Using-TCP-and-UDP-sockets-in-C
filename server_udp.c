#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define PORT     8080
#define BUFF_LEN 4096

#define GREEN "\033[0;32m"
#define BLUE  "\033[0;34m"
#define WHITE "\033[0;37m"
#define RED   "\033[0;31m"
#define RESET "\033[0m"

int turn_ptr = 0;
int replay[] = {0,0};

//! return 0 if neither won, 1 if p1 wins, 0 if p2 wins.
int checkForWin(char* game_state) {
    if(game_state[0] == game_state[1] && game_state[1] == game_state[2]) {
        if(game_state[0] == '1') return 1;
        else if(game_state[0] == '2') return 2;
        else return 0;
    } else if(game_state[0] == game_state[3] && game_state[3] == game_state[6]) {
        if(game_state[0] == '1') return 1;
        else if(game_state[0] == '2') return 2;
        else return 0;
    } else if(game_state[0] == game_state[4] && game_state[4] == game_state[8]) {
        if(game_state[0] == '1') return 1;
        else if(game_state[0] == '2') return 2;
        else return 0;
    } else if(game_state[2] == game_state[4] && game_state[4] == game_state[6]) {
        if(game_state[2] == '1') return 1;
        else if(game_state[2] == '2') return 2;
        else return 0;
    } else if(game_state[2] == game_state[5] && game_state[5] == game_state[8]) {
        if(game_state[2] == '1') return 1;
        else if(game_state[2] == '2') return 2;
        else return 0;
    } else if(game_state[4] == game_state[1] && game_state[1] == game_state[7]) {
        if(game_state[4] == '1') return 1;
        else if(game_state[4] == '2') return 2;
        else return 0;
    } else if(game_state[4] == game_state[3] && game_state[3] == game_state[5]) {
        if(game_state[4] == '1') return 1;
        else if(game_state[4] == '2') return 2;
        else return 0;
    } else if(game_state[6] == game_state[7] && game_state[7] == game_state[8]) {
        if(game_state[6] == '1') return 1;
        else if(game_state[6] == '2') return 2;
        else return 0;
    } else return 0;
}

int checkFormat(char* str) {
    if(strlen(str) != 3) return -1;
    else if(str[0] > 51 || str[0] < 49 || str[2] > 51 || str[2] < 49 || str[1] != 32) return -1;
    else return 1;
}

int handleClient(struct sockaddr_in client_address1, struct sockaddr_in client_address2, char* game_state, int sockfd) {
    //* temp client to store the address of the client from which the message is received
    struct sockaddr_in temp_client;
    socklen_t temp_client_length = sizeof(temp_client);

    //* buffers used 
    char* read_buffer = (char*)malloc(sizeof(char)*BUFF_LEN);
    char* write_buffer = (char*)malloc(sizeof(char)*BUFF_LEN);

    int Bytes_read = recvfrom(sockfd, read_buffer, BUFF_LEN, 0, (struct sockaddr*)&temp_client, &temp_client_length);
    int current_client = (ntohs(client_address1.sin_port) == ntohs(temp_client.sin_port)) ? 1 : (ntohs(client_address2.sin_port) == ntohs(temp_client.sin_port)) ? 2 : 3 ;
    read_buffer[strcspn(read_buffer, "\n")] = 0;
    //! incorrect turn of play (-4)
    if(((current_client == 1 && turn_ptr != 0) || (current_client == 2 && turn_ptr != 1)) && replay[0] == 0 && replay[1] == 0) {
        printf("client %d played early\n", current_client);
        if(current_client == 1) {
            sprintf(write_buffer, RED"ERR : wait for your turn boy..."RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
        } else {
            sprintf(write_buffer, RED"ERR : wait for your turn boy..."RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        }
        free(read_buffer);
        free(write_buffer);
        return -4;
    }
    //! invalid format of input (-2)
    if(checkFormat(read_buffer) == -1 && replay[0] == 0 && replay[1] == 0) {
        printf("invalid format of input from client %d\n", current_client);
        if(current_client == 1) {
            sprintf(write_buffer, RED"ERR : invalid input format..."RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
        } else {
            sprintf(write_buffer, RED"ERR : invalid input format..."RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        }
        free(read_buffer);
        free(write_buffer);
        return -2;
    }
    printf("client message: %s\n", read_buffer);
    if(current_client == 1 && replay[0] == 1) {
        if(read_buffer[0] == 'Y' || read_buffer[0] == 'y') replay[0] = 2;
        else if(read_buffer[0] == 'N' || read_buffer[0] == 'n') replay[0] = -1;
    }
    if(current_client == 2 && replay[1] == 1) {
        if(read_buffer[0] == 'Y' || read_buffer[0] == 'y') replay[1] = 2;
        else if(read_buffer[0] == 'N' || read_buffer[0] == 'n') replay[1] = -1;
    }
    if(replay[0] == 2 && replay[1] == 2) { //? they want a replay
        replay[0] = 0;
        replay[1] = 0;
        sprintf(write_buffer, GREEN"MSG : Match restarted! :) It's player1's turn."RESET);
        sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
        sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        turn_ptr = 1;
    } else if (replay[0] == -1 || replay[1] == -1) {//? they don't want a replay (-6)
        printf("no replay!\n");
        sprintf(write_buffer, RED"TERM : A player rejected remach :("RESET);
        sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
        sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        free(write_buffer);
        free(read_buffer);
        return -6;
    } else if(replay[0] == 0 && replay[1] == 0) {
        printf(RED"[not in replay mode!!]\n"RESET);
        int position = ((read_buffer[0]-48)-1)*3 + (read_buffer[2]-48)-1;
        if(game_state[position] != '0') {
            sprintf(write_buffer, RED"ERR : position already played!"RESET);
            if(current_client == 1) {
                sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            } else {
                sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
            }
            free(read_buffer);
            free(write_buffer);
            return -5;
        }
        if(turn_ptr == 0) game_state[position] = '1';
        else game_state[position] = '2';
        sendto(sockfd, game_state, strlen(game_state)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
        sendto(sockfd, game_state, strlen(game_state)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        int win_state = checkForWin(game_state);
        //? send game completion message to clients and ask for replay
        if(win_state == 1) { // p1 wins
            sprintf(write_buffer, GREEN"MSG : Player1 wins!"RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
            strcpy(game_state, "000000000");
            sprintf(write_buffer, BLUE"MSG : Play again?[Y/N]"RESET);
            replay[0] = 1;
            replay[1] = 1;
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        } else if (win_state == 2) { // p2 wins
            sprintf(write_buffer, GREEN"MSG : Player2 wins!"RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
            strcpy(game_state, "000000000");
            replay[0] = 1;
            replay[1] = 1;
            sprintf(write_buffer, BLUE"Play again?[Y/N]"RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        } else if (win_state == 0 && strchr(game_state, '0') == NULL) { // draw
            sprintf(write_buffer, GREEN"MSG : It's a draw!"RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
            strcpy(game_state, "000000000");
            replay[0] = 1;
            replay[1] = 1;
            sprintf(write_buffer, BLUE"Play again?[Y/N]"RESET);
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address1, sizeof(client_address1));
            sendto(sockfd, write_buffer, strlen(write_buffer)+1, 0, (struct sockaddr*)&client_address2, sizeof(client_address2));
        }
    }
    free(read_buffer);
    free(write_buffer);
    turn_ptr = 1-turn_ptr;
    return 0;
}

int main() {

    // *buffers used
    char* snd_msg = (char*)malloc(sizeof(char)*BUFF_LEN);
    char* rec_msg = (char*)malloc(sizeof(char)*BUFF_LEN);

    ///* define game state.
    char* game_state = (char*)malloc(sizeof(char)*11);
    strcpy(game_state, "000000000");

    //* create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    //* set address to the socket
    struct sockaddr_in socket_address, client_address1, client_address2, temp_client;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(PORT);
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t socket_address_len = sizeof(socket_address);
    socklen_t client_address1_length = sizeof(client_address1);
    socklen_t client_address2_length = sizeof(client_address2);
    socklen_t temp_client_length = sizeof(temp_client);
    int first_flag = 0, second_flag = 0;

    //* bind the address to socket
    bind(sockfd, (struct sockaddr*) &socket_address, socket_address_len);
    printf(BLUE"Server is running and waiting for clients...\n"RESET);

    // Accept first client
    recvfrom(sockfd, rec_msg, BUFF_LEN, 0, (struct sockaddr*)&temp_client, &temp_client_length);
    client_address1 = temp_client;
    first_flag = 1;
    bzero(rec_msg, BUFF_LEN);
    printf("client1 connected! Address: %s:%d\n", inet_ntoa(client_address1.sin_addr), ntohs(client_address1.sin_port));

    // Accept second client
    while (1) {
        recvfrom(sockfd, rec_msg, BUFF_LEN, 0, (struct sockaddr*)&temp_client, &temp_client_length);
        if (temp_client.sin_addr.s_addr != client_address1.sin_addr.s_addr || temp_client.sin_port != client_address1.sin_port) {
            client_address2 = temp_client;
            second_flag = 1;
            bzero(rec_msg, BUFF_LEN);
            printf("client2 connected! Address: %s:%d\n", inet_ntoa(client_address2.sin_addr), ntohs(client_address2.sin_port));
            break;
        } else {
            bzero(snd_msg, BUFF_LEN);
            sprintf(snd_msg, "Wait for client2 to connect...");
            sendto(sockfd, snd_msg, strlen(snd_msg)+1, 0, (struct sockaddr*)&temp_client, temp_client_length);
        }
    }

    printf("both conns made! c1addr:%s:%d, c2addr:%s:%d\n", inet_ntoa(client_address1.sin_addr), ntohs(client_address1.sin_port), inet_ntoa(client_address2.sin_addr), ntohs(client_address2.sin_port));
    sprintf(snd_msg, GREEN"MSG : you are player 1, you are assigned X"RESET);
    sendto(sockfd, snd_msg, BUFF_LEN, 0, (struct sockaddr*)&client_address1, client_address1_length);
    sprintf(snd_msg, GREEN"MSG : you are player 2, you are assigned O"RESET);
    sendto(sockfd, snd_msg, BUFF_LEN, 0, (struct sockaddr*)&client_address2, client_address2_length);
    while (1) {
        int action = handleClient(client_address1, client_address2, game_state, sockfd);
        if(action == -6) break;
    }

    close(sockfd);
    free(snd_msg);
    free(rec_msg);
    return 0;
}
