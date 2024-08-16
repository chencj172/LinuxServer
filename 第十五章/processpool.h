#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H
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
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <memory>

// 描述子进程的类，m_pid是子进程的id，m_pipefd是父子进程通信的管道
class process
{
public:
    process(): m_pid(-1){}
    process(int pid): m_pid(pid){}
public:
    pid_t m_pid;
    int m_pipefd[2];
};

// 进程池类，使用单例模式
template<typename T>
class processpool
{
private:
    processpool(int listenfd, int process_number = 0);

public:
    processpool<T>& operator=(const processpool<T>& pool) = delete;
    // 不创建对象就可以访问静态方法
    static processpool<T>* create(int listenfd, int process_number = 0)
    {
        if(!m_instance)
        {
            m_instance = new processpool<T>(listenfd, process_number);
            // m_instance = std::make_shared<processpool<T>>(new processpool<T>(listenfd, process_number));
        }
        return m_instance;
    }
    ~processpool<T>()
    {
        delete [] m_sub_process;
    }
    // 启动进程池
    void run();

private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();
private:
    // 进程池允许最大子进程数量
    static const int MAX_PROCESS_NUMBER = 16;
    // 子进程最多处理的客户数量
    static const int USER_PER_PROCESS = 65535;
    // epoll最多能处理的事件数
    static const int MAX_EVENT_NUMBER = 10000;
    // 进程池中的进程总数
    int m_process_number;
    // 子进程在池中的序号
    int m_idx;
    // 每个进程都有一个epoll内核事件表
    int m_epollfd;
    // 监听socket
    int m_listenfd;
    // 子进程通过m_stop决定是否停止运行
    int m_stop;
    // 所有子进程的描述信息
    process* m_sub_process;
    // 类的每个对象都共享这个实例，这样get实例返回的都是同一个
    // static std::shared_ptr<processpool<T>* > m_instance;
    static processpool<T>* m_instance;
};
template <typename T>
processpool<T>* processpool<T>::m_instance = nullptr;

// 用于处理信号的管道，以实现统一事件源
static int sig_pipefd[2];

static int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

static void addfd(int epfd, int fd)
{
    setnonblocking(fd);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}

static void removefd(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(*handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if(restart) sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

// 进程池的构造函数
template <typename T>
processpool<T>::processpool(int listenfd, int process_number)
    :m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)
{
    // std::cout << process_number << std::endl;
    assert((process_number > 0) && (process_number < MAX_PROCESS_NUMBER));

    m_sub_process = new process[process_number];
    assert(m_sub_process);

    // 创建process_number个子进程，并创建与父进程之间的管道
    for(int i=0;i<process_number;i++)
    {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);

        if(m_sub_process[i].m_pid > 0)
        {
            close(m_sub_process[i].m_pipefd[0]);
            continue;
        }
        else
        {
            close(m_sub_process[i].m_pipefd[1]);
            m_idx = i;
            break;
        }
    }
}

// 统一事件源
template <typename T>
void processpool<T>::setup_sig_pipe()
{
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

template <typename T>
void processpool<T>::run()
{
    // std::cout << m_idx << "   " << m_instance << std::endl;
    if(m_idx != -1)
    {
        run_child();
        return ;
    }
    run_parent();
}

template <typename T>
void processpool<T>::run_child()
{
    setup_sig_pipe();

    int pipefd = m_sub_process[m_idx].m_pipefd[0];
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T* user = new T[USER_PER_PROCESS];
    assert(user);
    int number = 0;
    int ret = -1;

    while(!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        
        if(number < 0 && errno != EINTR)
        {
            printf("epoll failure\n");
            break;
        }

        for(int i=0;i<number;i++)
        {
            int sockfd = events[i].data.fd;

            if(sockfd == pipefd && events[i].events == EPOLLIN)
            {
                std::cout << "new  connected...\n";
                // 处理新的客户端连接
                int clientfd;
                int ret = recv(pipefd, (char*)&clientfd, sizeof(clientfd), 0);
                if((ret < 0 && errno != EAGAIN) || ret == 0)
                {
                    continue;
                }
                else
                {
                    sockaddr_in clnt_addr;
                    socklen_t len = sizeof(clnt_addr);
                    int clnt_fd = accept(m_listenfd, (sockaddr*)&clnt_addr, &len);
                    assert(clnt_fd != -1);
                    addfd(m_epollfd, clnt_fd);
                    user[clnt_fd].init(m_epollfd, clnt_fd, clnt_addr);
                }
            }
            else if(sockfd == sig_pipefd[0] && events[i].events == EPOLLIN)
            {
                // 处理子进程接受到的信号
                int sig;
                char signals[1024];
                int ret = recv(sockfd, signals, sizeof(signals), 0);
                if(ret < 0) continue;
                else
                {
                    for(int i=0;i<ret;i++)
                    {
                        switch(signals[i])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while(waitpid(-1, &stat, WNOHANG) > 0) continue;
                                break;
                            }
                            case SIGTERM: break;
                            case SIGINT:
                            {
                                m_stop = true;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else if(events[i].events == EPOLLIN)
            {
                user[sockfd].process();
            }
            else 
            {
                continue;
            }
        }
    }

    delete [] user;
    user = nullptr;
    close(pipefd);
    close(m_epollfd);
}

template <typename T>
void processpool<T>::run_parent()
{
    setup_sig_pipe();

    addfd(m_epollfd, m_listenfd);
    epoll_event events[MAX_EVENT_NUMBER];
    int number;
    int new_conn = 1;
    int sub_process_counter = 0;

    while(!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if((number < 0) && errno != EINTR)
        {
            printf("epoll failure\n");
            break;
        }

        for(int i=0;i<number;i++)
        {
            int sockfd = events[i].data.fd;

            if(sockfd == m_listenfd && events[i].events == EPOLLIN)
            {
                // 分配子进程接受连接的请求
                int index = sub_process_counter;
                while(m_sub_process[index].m_pid == -1)
                {   
                    index = (index + 1) % MAX_PROCESS_NUMBER;
                } 
                if(m_sub_process[index].m_pid == -1)
                {
                    m_stop = true;
                    break;
                }
                send(m_sub_process[index].m_pipefd[1], (char*)&new_conn, sizeof(new_conn), 0);
            }
            else if(sockfd == sig_pipefd[0] && events[i].events == EPOLLIN)
            {
                int sig;
                char signals[1024];
                int ret = recv(sockfd, signals, sizeof(signals), 0);
                if(ret < 0) continue;
                else
                {
                    for(int i=0;i<ret;i++)
                    {
                        switch(signals[i])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                                {
                                    for(int j=0;j<m_process_number;j++)
                                    {
                                        if(m_sub_process[i].m_pid == pid)
                                        {
                                            printf("child %d join \n", j);
                                            close(m_sub_process[j].m_pipefd[0]);
                                            m_sub_process[j].m_pid = -1;
                                        }
                                    }
                                }

                                // 所有子进程退出,父进程也退出
                                m_stop = true;
                                for(int i=0;i<m_process_number;i++)
                                {
                                    if(m_sub_process[i].m_pid != -1)
                                    m_stop = false;
                                }
                                break;
                            }
                            case SIGTERM: break;
                            case SIGINT:
                            {
                                m_stop = true;
                                printf("kill all the child now\n");
                                for(int i=0;i<m_process_number;i++)
                                {
                                    int pid = m_sub_process[i].m_pid;
                                    if(pid != -1)
                                    kill(pid, SIGTERM);
                                }
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else 
            {
                continue;
            }
        }
    }

    close(m_epollfd);
}
#endif