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
#include <sys/select.h>
#include <fcntl.h>

int setnonblock(int sock)
{
    int old_option = fcntl(sock, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sock, F_SETFL, new_option);

    return old_option;
}

int unblock_connect(const char* ip, int port, int time)
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);
    // 套接字设置成非阻塞
    int old_option = setnonblock(sockfd);

    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    int res = connect(sockfd, (sockaddr*)&serv_addr, sizeof(sockaddr));
    if(res == 0)
    {
        // 连接建立完成，恢复之前的阻塞特性并返回
        std::cout << "connect with server imediately\n";
        fcntl(sockfd, F_SETFL, old_option);
        return sockfd;
    }
    else if(errno != EINPROGRESS)
    {
        // 对于非阻塞套接字来说，只有当errno是EINPROGRESS表示连接正在建立，也就是三次握手正在执行
        std::cout << "unblock connect not support\n";
        close(sockfd);
        return -1;
    }

    fd_set reads;
    fd_set writes;
    struct timeval timeout;
    // 设置阻塞超时时间
    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    FD_ZERO(&reads);
    FD_ZERO(&writes);
    FD_SET(sockfd, &writes);

    int ret = select(sockfd + 1, NULL, &writes, NULL, &timeout);
    if(ret <= 0)
    {
        // select出错或者是超时
        std::cout << "connect time out\n";
        close(sockfd);
        return -1;
    }
    else if(!FD_ISSET(sockfd, &writes))
    {
        // 写事件没有发生
        std::cout << "no events on sockfd found\n";
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
    {
        std::cout << "getsockopt error";
        close(sockfd);
        return -1;
    }
    if(error != 0)
    {
        std::cout << "connect failed after select with the error " << error << std::endl;
        close(sockfd);
        return -1;
    }

    std::cout << "connect successful...\n";
    fcntl(sockfd, F_SETFL, old_option);
    return sockfd;
}
int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int sockfd = unblock_connect(ip, port, 10);

    if(sockfd < 0) return 1;
    close(sockfd);

    return 0;
}