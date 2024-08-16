#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"

const int MAX_FD = 65535;
const int  MAX_EVENT_NUMBER = 10000;

extern int addfd(int , int, bool);
extern int removefd(int , int);

void addsig(int sig, void(*handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart)
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
void show_error(int connfd, const char* info)
{
    std::cout << info << std::endl;
    send(connfd, info, sizeof(info), 0);
    close(connfd);
}
int main(int argc, const char* argv[])
{
    assert(argc == 3);

    const char* ip = argv[1];
    const int port = atoi(argv[2]);
    
    // 忽略SIGPIPE信号
    addsig(SIGPIPE, SIG_IGN);

    // 创建线程池
    threadpool<http_conn>* pool = NULL;
    try
    {
        pool = new threadpool<http_conn>;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    // 预先为每个可能的客户分配一个http_conn对象
    http_conn* users = new http_conn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);
    // 关闭套接字后看还有没有未发送的数据,有的话等待2秒
    struct linger tmp = {1, 2};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);
    
    assert(bind(listenfd, (sockaddr*)&address, sizeof(address)) != -1);
    assert(listen(listenfd, 5) != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epfd = epoll_create(5);
    assert(epfd != -1);
    addfd(epfd, listenfd, false);
    http_conn::m_epollfd = epfd;

    while(true)
    {
        int number = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        if(number < 0 && errno != EINTR)
        {
            std::cout << "epoll failure\n";
            break;
        }
        for(int i=0;i<number;i++)
        {
            int sockfd = events[i].data.fd;

            if(sockfd == listenfd)
            {
                sockaddr_in clnt_addr;
                socklen_t len = sizeof(clnt_addr);
                int connfd = accept(sockfd, (sockaddr*)&clnt_addr, &len);
                assert(connfd > 0);
                if(http_conn::m_user_count >= MAX_FD)
                {
                    show_error(connfd, "Internal server busy");
                    continue;
                }
                users[connfd].init(connfd, clnt_addr);
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }
            else if(events[i].events & EPOLLIN)
            {
                if(users[sockfd].read())
                {
                    pool->append(users + sockfd);
                }
                else 
                {
                    users[sockfd].close_conn();
                }
            }
            else if(events[i].events & EPOLLOUT)
            {
                if(!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
        }
    }
    
    close(epfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}