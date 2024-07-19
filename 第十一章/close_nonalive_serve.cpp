/**
 * 定期检查客户端连接有没有需要销毁的
 * 还是通过系统发送的alarm信号去检查用户定时器
*/
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
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unordered_map>

#include "list_timer.h"

const int TIME_OUT = 5;
const int MAX_EVENTS = 1024;
static int pipefd[2];
static int epfd;
static class sort_timer_list *timer_list = new sort_timer_list();
std::unordered_map<int, client_data_ptr>mp;

void setnonblocking(const int& fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
void addfd(const int& fd)
{
    setnonblocking(fd);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}
void sig_handler(int sig)
{
    // 把信号发送给主循环
    int save_errno = errno;
    send(pipefd[1], (char*)&sig, 1, 0);
    std::cout << "alarm sig\n";
    errno = save_errno;
}
void addsig(const int& sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigemptyset(&sa.sa_mask);

    assert(sigaction(sig, &sa, NULL) != -1);
}
void cb_func(client_data* data)
{
    // 客户超时连接回调函数
    epoll_ctl(epfd, EPOLL_CTL_DEL, data->sock, 0);
    std::cout << "client " << ntohs(data->address.sin_port) << " timeout/disconnected...\n";
    close(data->sock);
    delete data;
}
int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char *ip = argv[1];
    const int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) != -1);
    assert(listen(listenfd, 5) != -1);

    // 使用epoll进行监听
    epfd = epoll_create(5);
    struct epoll_event events[MAX_EVENTS];
    addfd(listenfd);

    // 使用管道进行信号转发给主循环
    socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    addfd(pipefd[0]);

    // 注册定时信号，进程中止信号
    addsig(SIGALRM);
    addsig(SIGTERM);
    alarm(TIME_OUT);

    while(1)
    {
        int ret = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if(ret < 0)
        {
            if(errno != EINTR)
            {
                puts("epoll wait error");
                break;
            }
        }

        for(int i=0;i<ret;i++)
        {
            if(events[i].data.fd == listenfd && events[i].events == EPOLLIN)
            {
                // 有新的客户连接
                client_data *clnt_data = new client_data();
                socklen_t len = sizeof(clnt_data->address);
                clnt_data->sock = accept(listenfd, (sockaddr*)&clnt_data->address, &len);
                assert(clnt_data->sock != -1);
                addfd(clnt_data->sock);
                util_timer *timer = new util_timer();
                timer->expire = time(NULL) + 3 * TIME_OUT;
                timer->user_data = clnt_data;
                timer->cb_func = cb_func;
                clnt_data->timer = timer;
                std::cout << "client " << ntohs(clnt_data->address.sin_port) << " connected...\n";
                timer_list->add_timer(timer);
                mp.insert(std::pair<int, client_data_ptr>(clnt_data->sock, clnt_data));
            }
            else if(events[i].data.fd == pipefd[0])
            {
                // 要检查非活动的客户端并关闭
                char buf[BUF_SIZE];
                int len = recv(pipefd[0], buf, BUF_SIZE, 0);
                if(len <= 0) continue;
                for(int j=0;j<len;j++)
                {
                    switch(buf[j])
                    {
                        case SIGALRM:
                            timer_list->tick();
                            alarm(TIME_OUT);
                            break;
                        case SIGTERM:
                            return 0;
                            break;
                    }
                }
            }
            else
            {
                // 客户端发送信息
                client_data_ptr clnt_data = mp[events[i].data.fd];
                if(!clnt_data) continue;
                memset(clnt_data->buf, 0, sizeof(clnt_data->buf));
                int len = recv(clnt_data->sock, clnt_data->buf, BUF_SIZE, 0);
                if(len < 0)
                {
                    if(errno != EAGAIN)
                    {
                        // 读发生错误
                        cb_func(clnt_data);
                        if(clnt_data->timer)
                        timer_list->delete_timer(clnt_data->timer);
                    }
                }
                else if(len == 0)
                {
                    // 对方关闭连接
                    cb_func(clnt_data);
                    if(clnt_data->timer)
                    timer_list->delete_timer(clnt_data->timer);
                }
                else
                {
                    std::cout << "recv data from client " << ntohs(clnt_data->address.sin_port) << " is : " << clnt_data->buf << std::endl;
                    clnt_data->timer->expire = time(NULL) + 3 * TIME_OUT;
                    timer_list->adjust_timer(clnt_data->timer);
                }
            }
        }
    }
    mp.clear();
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete timer_list;
    return 0;
}