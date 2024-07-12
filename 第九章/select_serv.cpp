#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <assert.h>

// select能处理异常情况只有一种，就是收到带外数据
int main(int argc, char* argv[])
{
    assert(argc == 3);
    
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int listenfd;
    sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    bzero(&serv_addr, 0);
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(listenfd, 5) != -1);

    int clnt_sock = accept(listenfd, (sockaddr*)&clnt_addr, &clnt_len);
    assert(clnt_sock != -1);
    std::cout << "connected...\n";

    // 创建select中负责监听读和异常结构体
    fd_set read_set;
    fd_set exception_set;
    FD_ZERO(&read_set);
    FD_ZERO(&exception_set);
    char buf[1024];

    while(1)
    {
        // 每次都要设置监听的套接字
        FD_SET(clnt_sock, &read_set);
        FD_SET(clnt_sock, &exception_set);
        int res = select(clnt_sock + 1, &read_set, NULL, &exception_set, NULL);
        assert(res != -1);
        memset(buf, 0, sizeof(buf));
        if(FD_ISSET(clnt_sock, &read_set))
        {
            recv(clnt_sock, buf, sizeof(buf), 0);
            if(strcmp("exit", buf) == 0)
            {
                std::cout << "disconnected...\n";
                break;
            }
            std::cout << "normal message : " << buf << std::endl;
        }
        else if(FD_ISSET(clnt_sock, &exception_set))
        {
            recv(clnt_sock, buf , sizeof(buf), MSG_OOB);
            std::cout << "msg_oob message : " << buf << std::endl;
        }
    }

    close(listenfd);
    close(clnt_sock);
    return 0;
}