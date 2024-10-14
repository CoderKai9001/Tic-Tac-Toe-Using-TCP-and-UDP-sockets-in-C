#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT        8081
#define MAX_CLIENTS 2
#define BUFF_LEN    4096

#define GREEN "\033[0;32m"
#define BLUE  "\033[0;34m"
#define WHITE "\033[0;37m"
#define RED   "\033[0;31m"
#define RESET "\033[0m"

int turn_ptr = 0;
int replay[] = {0,0};

void printLocalIP() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, host);
        }
    }

    freeifaddrs(ifaddr);
}

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

//! ret_vals : {fd:success, -1:disconnect, -2:invalid_input_err, -3:wait_err, -4:wrong_turn_err, -5:played_position_err, -6:player_rejects_replay}
int handleClient(int clientfd, int* clients, char* game_state) { 
    // buffers used for read and write to clients socks
    char* read_buffer = (char*)malloc(sizeof(char)*BUFF_LEN);
    char* write_buffer = (char*)malloc(sizeof(char)*BUFF_LEN);
    // read from client
    int read_ret = recv(clientfd, read_buffer, BUFF_LEN, 0);
    if(read_ret == 0) { // closing client sock
        return -1;
    }
    read_buffer[strcspn(read_buffer, "\n")] = 0;
    //! incorrect turn of play
    if(((clients[0] == clientfd && turn_ptr != 0) || (clients[1] == clientfd && turn_ptr != 1)) && replay[0] == 0 && replay[1] == 0) {
        printf("not the client's turn[fd:%d]\n", clientfd);
        sprintf(write_buffer, RED"ERR : wait for your turn boy..."RESET);
        send(clientfd, write_buffer, strlen(write_buffer)+1, 0);
        free(read_buffer);
        free(write_buffer);
        return -4;
    }   
    if(checkFormat(read_buffer) == -1 && replay[0] == 0 && replay[1] == 0) { //! invalid format of input
        printf("data recieved from client in incorrect format!\n");
        sprintf(write_buffer, RED"ERR : Invalid Format!"RESET);
        send(clientfd, write_buffer, strlen(write_buffer)+1, 0);
        free(read_buffer);
        free(write_buffer);
        return -2;
    }
    if(clients[0] == 0 || clients[1] == 0) {
        printf("client sending data before both connections are made\n");
        sprintf(write_buffer, RED"ERR : wait for other client to connect!"RESET);
        send(clientfd, write_buffer, strlen(write_buffer)+1, 0);
        free(read_buffer);
        free(write_buffer);
        return -3;
    }
    printf("message recieved by client: %s\n", read_buffer);
    // send ack to client
        // sprintf(write_buffer, GREEN"message recieved is [%s]"RESET, read_buffer);
    if(clients[0] == clientfd && replay[0] == 1) {
        if(read_buffer[0] == 'Y' || read_buffer[0] == 'y') replay[0] = 2;
        else if(read_buffer[0] == 'N' || read_buffer[0] == 'n') replay[0] = -1;
    }
    if(clients[1] == clientfd && replay[1] == 1) {
        if(read_buffer[0] == 'Y' || read_buffer[0] == 'y') replay[1] = 2;
        else if(read_buffer[0] == 'N' || read_buffer[0] == 'n') replay[1] = -1;
    }
    if(replay[0] == 2 && replay[1] == 2) { //? they want a replay
        replay[0] = 0;
        replay[1] = 0;
        sprintf(write_buffer, GREEN"MSG : Match restarted! :) It's player1's turn."RESET);
        send(clients[1], write_buffer, BUFF_LEN, 0);
        send(clients[0], write_buffer, BUFF_LEN, 0);
        turn_ptr = 1;
    } else if (replay[0] == -1 || replay[1] == -1) {//? they don't want replay
        printf("no replay!\n");
        sprintf(write_buffer, RED"MSG : A player rejected remach :("RESET);
        send(clients[0], write_buffer, BUFF_LEN, 0);
        send(clients[1], write_buffer, BUFF_LEN, 0);
        free(write_buffer);
        free(read_buffer);
        return -6;
    } else if (replay[0] == 0 && replay[1] == 0) {
        printf(RED"[not in replay modify mode!!]\n"RESET);
        int position = ((read_buffer[0]-48)-1)*3 + (read_buffer[2]-48)-1;
        if(game_state[position] != '0') {
            sprintf(write_buffer, RED"ERR : position already played!"RESET);
            send(clientfd, write_buffer, strlen(write_buffer)+1, 0);
            free(read_buffer);
            free(write_buffer);
            return -5;
        }
        if(turn_ptr==0) game_state[position] = '1';
        else game_state[position] = '2';
        send(clientfd, game_state, strlen(game_state)+1, 0);
        if(clients[0] != clientfd) send(clients[0], game_state, strlen(game_state)+1, 0);
        else if(clients[1] != clientfd) send(clients[1], game_state, strlen(game_state)+1, 0);
        int win_state = checkForWin(game_state);
        //? send game completion message to clients and ask for replay
        if (win_state == 1) { // p1 wins
            sprintf(write_buffer, GREEN"MSG : Player1 wins!"RESET);
            usleep(10);
            send(clients[0], write_buffer, BUFF_LEN, 0);
            usleep(10);
            send(clients[1], write_buffer, BUFF_LEN, 0);
            usleep(10);
            strcpy(game_state, "000000000");
            sprintf(write_buffer, BLUE"MSG : Play again?[Y/N]"RESET);
            replay[0] = 1;
            replay[1] = 1;
            send(clients[0], write_buffer, BUFF_LEN, 0);
            usleep(10);
            send(clients[1], write_buffer, BUFF_LEN, 0);
            usleep(10);
        } else if (win_state == 2) { // p2 wins
            sprintf(write_buffer, GREEN"MSG : Player2 wins!"RESET);
            usleep(10);
            send(clients[0], write_buffer, BUFF_LEN, 0);
            usleep(10);
            send(clients[1], write_buffer, BUFF_LEN, 0);
            usleep(10);
            strcpy(game_state, "000000000");
            replay[0] = 1;
            replay[1] = 1;
            sprintf(write_buffer, BLUE"Play again?[Y/N]"RESET);
            send(clients[0], write_buffer, BUFF_LEN, 0);
            usleep(10);
            send(clients[1], write_buffer, BUFF_LEN, 0);
            usleep(10);
        } else if (win_state == 0 && strchr(game_state, '0') == NULL) { // draw
            sprintf(write_buffer, GREEN"MSG : DRAW!"RESET);
            send(clients[0], write_buffer, BUFF_LEN, 0);
            send(clients[1], write_buffer, BUFF_LEN, 0);
            strcpy(game_state, "000000000");
            replay[0] = 1;
            replay[1] = 1;
            sprintf(write_buffer, BLUE"Play again?[Y/N]"RESET);
            send(clients[0], write_buffer, BUFF_LEN, 0);
            send(clients[1], write_buffer, BUFF_LEN, 0);
        }
    }
    // free
    free(read_buffer);
    free(write_buffer);
    turn_ptr = 1-turn_ptr;
    return clientfd;
}

int main() {

    printLocalIP();

    // game state
    char* game_state = (char*)malloc(sizeof(char)*11);
    strcpy(game_state, "000000000");

    // buffers used
    char* rec_msg = (char*)malloc(sizeof(char)*BUFF_LEN);
    char* snd_msg = (char*)malloc(sizeof(char)*BUFF_LEN);

    int clients[] = {0,0};
    int last_client_added = 0;

    // create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // set address to socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    socklen_t server_address_length = sizeof(server_addr);

    // bind the socket to the address
    bind(server_socket, (struct sockaddr*) &server_addr, server_address_length);

    // listen from the socket for any connections
    listen(server_socket, MAX_CLIENTS);

    // server accepts client in a loop , gets and sends data.
    fd_set current_sockets;

    for(;;) {
        // initialise the fd_set
        FD_ZERO(&current_sockets);
        // add server sock to set
        FD_SET(server_socket, &current_sockets);

        // add currently active client socks to set
        for(int i = 0; i < MAX_CLIENTS; i++)
            if(clients[i] > 0) FD_SET(clients[i], &current_sockets);

        // detect the sockets which are active
        if(select(FD_SETSIZE, &current_sockets, NULL, NULL, NULL) < 0) {
            printf("select error\n");
            exit(EXIT_FAILURE);
        }

        // check for new connections and data recieved.
        for(int i = 0; i < FD_SETSIZE; i++) {
            if(FD_ISSET(i, &current_sockets)) {
                if(i == server_socket) { // new connection
                    for(int j = 0; j < MAX_CLIENTS; j++) {
                        if(clients[j] == 0) {
                            printf(GREEN"new client [%d] connected!\n"RESET, j);
                            clients[j] = accept(server_socket, (struct sockaddr*) &server_addr, &server_address_length);
                            char sym = j == 0 ? 'X' : 'O';
                            sprintf(snd_msg, GREEN"MSG : you are player %d, you are assigned %c"RESET, j+1, sym);
                            send(clients[j], snd_msg, BUFF_LEN, 0);
                            break;
                        }
                    }
                } else { // incoming data / closing sockets
                    printf("handling client with sockfd[%d]\n", i);
                    int ret_stat = handleClient(i, clients, game_state);
                    // closing socks
                    if (ret_stat == -1) {
                        for(int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j] == i) {
                                close(i);
                                clients[j] = 0;
                                break;
                            }
                        }
                    }
                    // end game if -6
                    if(ret_stat == -6) {
                        sprintf(snd_msg, RED"TERM : exiting now..."RESET);
                        send(clients[0], snd_msg, BUFF_LEN, 0);
                        send(clients[1], snd_msg, BUFF_LEN, 0);
                        close(clients[0]);
                        close(clients[1]);
                        usleep(10);
                        close(server_socket);
                        printf(GREEN"game has ended!\n"RESET);
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}