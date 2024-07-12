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

int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int sock;
    sockaddr_in serv_addr;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock  != -1);

    bzero(&serv_addr, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    assert(connect(sock, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    char buf[1024], is_oob;
    // send(sock, "1212", strlen("1212"), 0);

    while(1)
    {
        std::cout << "please input : ";
        memset(buf, 0, sizeof(buf));
        std::cin >> buf >> is_oob;
        if(is_oob == 'T')
        {
            // 发送带外数据
            send(sock, buf, strlen(buf), MSG_OOB);
        }
        else 
        {
            // 发送正常数据
            send(sock, buf, strlen(buf), 0);
            if(strcmp("exit", buf) == 0) 
            {
                std::cout << "disconnected...\n";
                break;
            }
        }
    }
    close(sock);
    return 0;
}