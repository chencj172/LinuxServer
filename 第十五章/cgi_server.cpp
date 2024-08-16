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
#include <sys/wait.h>

#include "processpool.h"

class cgi_conn
{
public:
    cgi_conn(){}
    ~cgi_conn(){}

    void init(int epollfd, int sockfd, sockaddr_in& client_addr)
    {
        epoll_fd = epollfd;
        clnt_fd = sockfd;
        m_address = client_addr;

        memset(buf, 0, sizeof(buf));
        m_read_idx = 0;
    }

    void process()
    {
        int idx = 0;
        int ret = -1;
        while(true)
        {
            idx = m_read_idx;
            ret = recv(clnt_fd, buf, BUFFER_SIZE - 1, 0);
            if(ret < 0)
            {
                if(errno != EAGAIN)
                {
                    removefd(epoll_fd, clnt_fd);
                }
                break;
            }
            else if(ret == 0)
            {
                removefd(epoll_fd, clnt_fd);
                break;
            }
            else
            {
                m_read_idx += ret;
                std::cout << "user ocntent is : " << buf;
            }
        }
    }

private:
    static const int BUFFER_SIZE = 1024;
    static int epoll_fd;
    int clnt_fd;
    char buf[BUFFER_SIZE];
    sockaddr_in m_address;
    int m_read_idx;
};
int cgi_conn::epoll_fd = -1;

int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    serv_addr.sin_family = AF_INET;
    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(listenfd, 5) != -1);
    
    // 获取进程池实例
    auto m_instance = processpool<cgi_conn>::create(listenfd, 5);
    // 创建了n个进程,下面的就会执行n+1次
    if(m_instance)
    {
        m_instance->run();

        delete m_instance;
    }

    close(listenfd);
    return 0;
}