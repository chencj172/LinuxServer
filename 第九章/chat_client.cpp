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
#include <poll.h>
#include <sys/uio.h>
#include <fcntl.h>

char buf[1024];
int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(connect(sockfd, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    puts("connected...");
    // 使用poll监听标准输入和该文件描述符
    pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    // 创建管道用于使用splice函数发送数据
    int pipefd[2];
    assert(pipe(pipefd) != -1);

    while(1)
    {
        // 开始监听
        int ret = poll(fds, 2, -1);

        if(ret < 0)
        {
            puts("poll error");
            break;
        }
        else if(ret > 0)
        {
            // 只有三种事件会发生，因为上面一共只监听三个事件
            if(fds[0].revents & POLLIN)
            {
                // 标准输入的数据发送给服务端
                // 使用splice函数，先写入管道，在从管道输出
                splice(0, NULL, pipefd[1], NULL, 1024, SPLICE_F_MORE | SPLICE_F_MOVE);
                splice(pipefd[0], NULL, sockfd, NULL, 1024, SPLICE_F_MORE | SPLICE_F_MOVE);
            }
            if(fds[1].revents & POLLIN)
            {
                // 输出到标准输出
                memset(buf, 0, sizeof(buf));
                recv(sockfd, buf, sizeof(buf), 0);
                std::cout << "message from server : " << buf << std::endl;
            }
            if(fds[1].revents & POLLRDHUP)
            {
                puts("disconnected...");
                break;
            }
        }
    }
    close(sockfd);
    return 0;
}