#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

#define THREAD_NUM 100

void* connect_to_server(void* arg) {
    int *port = (int*)arg;
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(*port);
    // 假设服务器地址为本地地址
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_fd);
        return NULL;
    }

    printf("Client connected to server\n");
    // 可以发送或接收数据...

    close(client_fd);
    return NULL;
}

int main(int argc, const char* argv[]) {
    pthread_t threads[THREAD_NUM];
    int port = atoi(argv[1]);

    // 创建100个线程连接服务器
    for (int i = 0; i < THREAD_NUM; ++i) {
        if (pthread_create(&threads[i], NULL, connect_to_server, (void*)&port) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    // 等待所有线程结束
    for (int i = 0; i < THREAD_NUM; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}