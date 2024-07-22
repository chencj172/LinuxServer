#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <assert.h>
#include <pthread.h>

const int MAX_EVENTS = 1024;
static int epfd;
static int listenfd;
int index_pos = 0;

void setnonblocking(const int& fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
void addfdToepfd(int fd)
{
    setnonblocking(fd);
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLET |  EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD, fd, &event);
}
void* func(void *arg)
{
        sockaddr_in clnt_addr;
        socklen_t len = sizeof(clnt_addr);
        int sock = accept(listenfd, (sockaddr*)&clnt_addr, &len);
        assert(sock != -1);
        addfdToepfd(sock);

        return NULL;
}

int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char *ip = argv[1];
    const int port = atoi(argv[2]);

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);
    sockaddr_in serv_addr, clnt_addr;
    socklen_t len = sizeof(clnt_addr);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) != -1);
    assert(listen(listenfd, 1024) != -1);

    struct epoll_event events[MAX_EVENTS];
    epfd = epoll_create(100);
    addfdToepfd(listenfd);
    // struct epoll_event event;
    // event.data.fd = listenfd;
    // event.events = EPOLLIN;
    // epoll_ctl(epfd,EPOLL_CTL_ADD, listenfd, &event);
    
    while(1)
    {
        if(index_pos == 100) break;
        int ret = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for(int i=0;i<ret;i++)
        {
            if(events[i].data.fd == listenfd && events[i].events == EPOLLIN)
            {
                int sock;
                // 循环从监听队列里面取得连接，因为是ET模式
                // 要么循环配合边沿触发，要么就单纯使用水平触发
                while((sock = accept(listenfd, (sockaddr*)&clnt_addr, &len)) != -1)
                {
                    addfdToepfd(sock);
                    index_pos++;
                    std::cout << "index_pos : " << index_pos << std::endl;
                }
                // sock = accept(listenfd, (sockaddr*)&clnt_addr, &len);
                // addfdToepfd(sock);
                // index_pos++;
                // std::cout << "index_pos : " << index_pos << std::endl;
            }
            else if(events[i].events == EPOLLIN)
            {
                char buf[10];
                int fd = events[i].data.fd;
                int len = recv(fd, buf, 10, 0);
                if(len < 0)
                {
                    if(errno != EAGAIN)
                    {
                        // 读发生错误
                        puts("Error");
                        close(fd);
                    }
                }
                else if(len == 0)
                {
                    // 对方关闭连接
                    puts("disconnected...");
                    close(fd);
                }
                else
                {
                    
                }
            }
        }
    }

    close(listenfd);
    return 0;
}