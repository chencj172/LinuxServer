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
#include <assert.h>
#include <sys/epoll.h>

const int MAX_EVENT = 1024;
const int TCP_BUFSIZE = 512;
const int UDP_BUFSIZE = 1024;

void setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
void addfd(int epfd, int fd)
{
    setnonblocking(fd);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}
int main(int argc, const char* argv[])
{
    assert(argc == 3);
    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    // 绑定TCP套接字
    int tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
    assert(tcp_socket != -1);
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(bind(tcp_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) != -1);
    assert(listen(tcp_socket, 5) != -1);

    // 绑定UDP套接字
    int udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    assert(bind(udp_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) != -1);

    struct epoll_event events[MAX_EVENT];
    int epfd = epoll_create(5);
    addfd(epfd, tcp_socket);
    addfd(epfd, udp_socket);
    
    while(1)
    {
        int ret = epoll_wait(epfd, events, MAX_EVENT, -1);
        assert(ret > 0);
        for(int i=0;i<ret;i++)
        {
            if(events[i].data.fd == tcp_socket && events[i].events == EPOLLIN)
            {
                // 有新的tcp客户端连接
                sockaddr_in clnt_addr;
                socklen_t len = sizeof(clnt_addr);
                int clnt_sock = accept(tcp_socket, (sockaddr*)&clnt_addr, &len);
                assert(clnt_sock != -1);
                addfd(epfd, clnt_sock);
                puts("connected...");
            }
            else if(events[i].data.fd == udp_socket && events[i].events == EPOLLIN)
            {
                // udp客户端
                sockaddr_in clnt_addr;
                socklen_t len = sizeof(clnt_addr);
                char udp_buf[UDP_BUFSIZE];
                memset(udp_buf, 0, sizeof(udp_buf));
                int recvlen = recvfrom(udp_socket, udp_buf, UDP_BUFSIZE - 1, 0, (sockaddr*)&clnt_addr, &len);
                assert(recvlen >= 0);
                if(strcmp(udp_buf, "q") == 0)
                {
                    puts("udp client disconnected...");
                    continue;
                }
                if(recvlen == 0)
                puts("disconnected...");
                else
                {
                    // std::cout << "udp client port : " << ntohs(clnt_addr.sin_port) << "\n";
                    std::cout << "udp message : " << udp_buf << std::endl;

                    sendto(udp_socket, udp_buf, strlen(udp_buf), 0, (sockaddr*)&clnt_addr, sizeof(clnt_addr));
                }
            }
            else if(events[i].events == EPOLLIN)
            {
                char tcp_buf[TCP_BUFSIZE];
                while(1)
                {
                    memset(tcp_buf, 0, sizeof(tcp_buf));
                    int recvlen = recv(events[i].data.fd, tcp_buf, TCP_BUFSIZE, 0);
                    if(strcmp(tcp_buf, "q") == 0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, events + i);
                        puts("tcp client disconnected...");
                        break;
                    }
                    if(recvlen > 0)
                    {
                        std::cout << "tcp message : " << tcp_buf << std::endl;
                    }
                    else
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            send(events[i].data.fd, "accept", strlen("accept"), 0);
                            break;
                        }
                        close(events[i].data.fd);
                        break;
                    }
                }
            }
        }
    }

    close(tcp_socket);
    close(udp_socket);
    return 0;
}