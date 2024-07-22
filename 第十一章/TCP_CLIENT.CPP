#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>

// int client_socket[1000];
void* client_thread(void* arg) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return NULL;
    }

    int *port = (int*)arg;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(*port); // 服务器端口
    // 服务器IP，这里使用本地回环地址
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_socket);
        return NULL;
    }
    // sleep(5);
    sleep(20);
    close(client_socket);
    return NULL;
}

int main(int argc, const char* argv[]) {
    const int num_clients = 500;
    pthread_t threads[num_clients];

    int port = atoi(argv[1]);

    for (int i = 0; i < num_clients; ++i) {
        if (pthread_create(&threads[i], NULL, client_thread, (void*)&port) != 0) {
            perror("pthread_create");
            return 1;
        }
        std::cout << "current index : " << i << std::endl;
    }
    
    for (int i = 0; i < num_clients; ++i) {
        pthread_join(threads[i], NULL);
    }
    // while(1);

    return 0;
}