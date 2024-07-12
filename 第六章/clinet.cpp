#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void error_headling(const char *);

int main(int argc, const char* argv[])
{
    int clnt_sock;
    struct sockaddr_in serv_addr;
    char message[1024];
    int str_len = 0;

    if(argc != 3)
    {
        std::cout << "Usage : " << argv[0] << "<port>\n";
        exit(1);
    }

    // 创建socket
    clnt_sock = socket(PF_INET, SOCK_STREAM, 0);
    std::cout << "clint : " << clnt_sock << std::endl;

    if(clnt_sock == -1)
        error_headling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family =  AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // 服务器发送连接请求
    if(connect(clnt_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_headling("connect() error");
    else 
        std::cout << "Connected...\n";

    memset(message, 0, sizeof(message));
    // sendfile
    // while (1) {
    //     std::cout << "===\n";
    //     ssize_t received = recv(clnt_sock, message, sizeof(message) - 1, 0);
    //     if (received == -1) {
    //         perror("recv failed");
    //         break;
    //     } else if (received == 0) {
    //         // 连接关闭
    //         break;
    //     }
    //     // 写入到文件或显示在终端
    //     // write(STDOUT_FILENO, message, received);
    //     std::cout << message << std::endl;
    // }
    
    // splice
    write(clnt_sock, "ab", strlen("12"));
    write(clnt_sock, "ab", strlen("12"));
    read(clnt_sock, message, 4);
    std::cout << "message from server : " << message << std::endl;
    close(clnt_sock);

    return 0;
}

void error_headling(const char *message)
{
    perror("error: ");
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
