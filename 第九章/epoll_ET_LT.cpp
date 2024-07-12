#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <assert.h>
#include <fcntl.h>

const int MAX_EVENT_NUMBER = 1024;
const int BUF_SIZE = 10;
char buf[BUF_SIZE];

void setnonblock(const int& sock)
{
    int old_option = fcntl(sock, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sock, F_SETFL, new_option);
}

int acceptclient(const int& listenfd)
{
    sockaddr_in clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);
    int clnt_sock = accept(listenfd, (sockaddr*)&clnt_addr, &clnt_len);
    assert(clnt_sock != -1);
    std::cout << "connected...\n";

    return clnt_sock;
}
void addfd(const int &epfd, const int& sock, bool flag)
{
    struct epoll_event event;
    event.data.fd = sock;
    event.events = EPOLLIN;
    if(flag)
    {
        // 边沿模式要设置套接字为非阻塞
        setnonblock(sock);
        event.events |= EPOLLET;
    }

    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event);
}
// 水平触发模式
void epoll_LT(const int& epfd, const struct epoll_event* events, const int& num, const int& listenfd)
{
    for(int i=0;i<num;i++)
    {
        if(events[i].data.fd == listenfd)
        {
            int clnt_sock = acceptclient(listenfd);
            // 添加到epfd中
            addfd(epfd, clnt_sock, false);
        }
        else if(events[i].events & EPOLLIN)
        {
            std::cout << "event trigger once\n";
            memset(buf, 0, sizeof(buf));
            int len = recv(events[i].data.fd, buf, BUF_SIZE - 1, 0);
            // 主动或者被动退出
            if(strcmp(buf, "exit") == 0 || len == 0)
            {
                std::cout << "disconnected...\n";
                close(events[i].data.fd);
                return;
            }
            std::cout << "get " << len << " bytes of content : " << buf << std::endl;
        }
    }
}

// 边沿触发方式
void epoll_ET(const int& epfd, const struct epoll_event* events, const int& num, const int& listenfd)
{
    for(int i=0;i<num;i++)
    {
        if(events[i].data.fd == listenfd)
        {
            int clnt_sock = acceptclient(listenfd);
            // 添加到epfd中
            addfd(epfd, clnt_sock, true);
        }
        else if(events[i].events & EPOLLIN)
        {
            std::cout << "event trigger once\n";
            // 必须要循环从缓冲区读取
            while(1)
            {
                memset(buf, 0, sizeof(buf));
                int len = recv(events[i].data.fd, buf, BUF_SIZE - 1, 0);
                if(len < 0)
                {
                    // 缓冲区没有数据可读
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        break;
                    }
                }
                else
                {
                    if(strcmp(buf, "exit") == 0 || len == 0)
                    {
                        std::cout << "disconnected...\n";
                        close(events[i].data.fd);
                        return ;
                    }
                    std::cout << "get " << len << " bytes of content : " << buf << std::endl;
                }
            }
        }
    }
}

int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(listenfd, 5) != -1);

    // 生成epoll注册表，并注册listenfd
    auto epfd = epoll_create(MAX_EVENT_NUMBER);
    struct epoll_event event, *events;
    events = (epoll_event*)malloc(sizeof(epoll_event) * MAX_EVENT_NUMBER);
    event.data.fd = listenfd;
    event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);

    while(1)
    {
        int ret = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        std::cout << ret << std::endl;
        // epoll_LT(epfd, events, ret, listenfd);
        epoll_ET(epfd, events, ret, listenfd);
    }
    close(listenfd);
    return 0;
}