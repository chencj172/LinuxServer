#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>

const int BUF_SIZE = 512;
int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    int tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
    assert(tcp_socket != -1);
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    assert(connect(tcp_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) != -1);
    puts("connected...");

    char buf[BUF_SIZE];
    while(1)
    {
        memset(buf, 0, sizeof(buf));
        std::cout << "please input tcp data: ";
        std::cin >> buf;

        send(tcp_socket, buf, strlen(buf), 0);
        if(strcmp(buf, "q") == 0)
        break;

        recv(tcp_socket, buf, BUF_SIZE, 0);
        std::cout << "message : " << buf << std::endl;
    }

    close(tcp_socket);
    return 0;
}