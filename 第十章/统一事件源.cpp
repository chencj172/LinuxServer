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
#include <signal.h>
#include <sys/epoll.h>

// 把事件当作事件进行处理

const int MAX_EVENT = 1024;
const int BUF_SIZE = 1024;

static int pipefd[2];
void setnonblocking(const int& fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
void addfd(const int& epfd, const int& fd)
{
    setnonblocking(fd);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}

// 信号处理函数很快执行完毕，就不会屏蔽之后再传来的信号
void sig_handler(int sig)
{
    puts("chufa");
    int save_error = errno;
    // 将信号传给主循环
    int msg = sig;
    // 信号就是一字节大小
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_error;
}

void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    // sigfillset(&sa.sa_mask);
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    // std::cout << sa.sa_mask.__val << std::endl;
    // sigemptyset(&sa.sa_mask);
    // sigaddset(&sa.sa_mask, SIGALRM);
    sigaction(sig, &sa, NULL);
}

int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
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
    epoll_event events[MAX_EVENT];
    int epfd = epoll_create(5);
    addfd(epfd, listenfd);

    // 创建管道
    // assert(pipe(pipefd) != -1);
    socketpair(PF_UNIX,  SOCK_STREAM, 0, pipefd);  //使用这个注册管道的读事件
    setnonblocking(pipefd[1]);
    addfd(epfd, pipefd[0]);

    // 设置屏蔽信号
    sigset_t array;
    sigemptyset(&array);
    sigaddset(&array, SIGALRM);
    sigprocmask(SIG_BLOCK, &array, NULL);
    
    // 注册信号
    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    // addsig(SIGINT);
    addsig(SIGALRM);
    alarm(2);

    while(1)
    {
        int ret = epoll_wait(epfd, events, MAX_EVENT, -1);
        // errno为EINTR说明信号被触发，中断当前的系统调用
        if(ret < 0 && errno != EINTR)
        {
            puts("epoll_wair error");
            break;
        }

        for(int i=0;i<ret;i++)
        {
            if(events[i].data.fd == pipefd[0])
            {
                char buf[BUF_SIZE];
                int len = recv(pipefd[0], buf, BUF_SIZE, 0);
                if(len <= 0)
                continue;
                else
                {
                    for(int j=0;j<len;j++)
                    {
                        // 在这处理相关信号
                        int sig_val = (int)buf[j];
                        switch (sig_val)
                        {
                        case SIGALRM:
                            std::cout << "alrm\n";
                            break;
                        
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }

    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}