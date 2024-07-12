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
#include <pthread.h>

const int MAX_EVENT_NUMBER = 1024;
const int BUF_SIZE = 10;
char buf[BUF_SIZE];

typedef struct
{
    int epfd;
    int sock;
}fds;

void setnonblock(const int &sock)
{
    int old_option = fcntl(sock, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sock, F_SETFL, new_option);
}
void addfd(const int &epfd, const int &sock)
{
    struct epoll_event event;
    event.data.fd = sock;
    // 这样设置以后一个线程就只负责一个套接字
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    // 设置成非阻塞套接字
    setnonblock(sock);
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event);
}

void reset_oneshot(const int& epfd, const int& sock)
{
    struct epoll_event event;
    event.data.fd = sock;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &event);
}

void* handle_data(void* arg)
{
    fds* tmp = (fds*)arg;
    std::cout << "sock : " << tmp->sock << std::endl;
    while(1)
    {
        memset(buf, 0, sizeof(buf));
        int ret = recv(tmp->sock, buf, BUF_SIZE - 1, 0);
        if(ret < 0)
        {
            // 数据接收完毕
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 重新注册sock上的事件，内核才能检查到EPOLLIN事件的发生
                reset_oneshot(tmp->epfd, tmp->sock);
                break;
            }
            else 
            {
                perror("error ");
                exit(-1);
            }
        }
        else if(ret > 0)
        {
            std::cout << "msg from server : " << buf << std::endl;
            // 模拟处理数据
            puts("start process data");
            sleep(5);
            puts("ending");
        }
        else 
        {
            puts("disconnected...");
            close(tmp->sock);
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

    sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);
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
        for(int i=0;i<ret;i++)
        {
            auto fd = events[i].data.fd;
            if(fd == listenfd)
            {
                int sock = accept(listenfd, (sockaddr*)&clnt_addr, &clnt_len);
                assert(sock != -1);
                // 添加到注册表
                addfd(epfd, sock);
                std::cout << "connected...\n";
            }
            else
            {
                fds tmp;
                tmp.epfd = epfd;
                tmp.sock = fd;
                // 开一个线程处理
                pthread_t pid;
                pthread_create(&pid, NULL, handle_data, (void*)&tmp);
                // std::cout << pid << std::endl;
                pthread_detach(pid);
            }
        }
    }
    close(listenfd);
    return 0;
}