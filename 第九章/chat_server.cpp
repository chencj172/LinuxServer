#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <unordered_map>
#include <fcntl.h>
#include <assert.h>

const int user_limits = 5;
const int BUF_SIZE = 64;
int user_count = 0;

// 客户端信息: 地址结构，写缓冲区，读缓冲区
typedef struct 
{
    sockaddr_in address;
    char *writebuf;
    char readbuf[BUF_SIZE];
}client, *clientptr;
std::unordered_map<int, client>clntMap;

int setnonblock(int sock)
{
    int old_option = fcntl(sock, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(sock, F_SETFL, new_option);

    return old_option;
}

void init_pollfds(pollfd fds[], const int& listenfd)
{
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    for(int i=1;i<=user_limits;i++)
    {
        fds[i].fd = -1;
        fds[i].revents = 0;
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
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    assert(bind(listenfd, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    assert(listen(listenfd, 5) != -1);

    pollfd fds[user_limits + 1];
    init_pollfds(fds, listenfd);

    while(1)
    {
        int ret = poll(fds, user_count + 1, -1);
        if(ret < 0)
        {
            puts("poll failure");
            break;
        }
        for(int i=0;i<=user_count;i++)
        {
            if(fds[i].fd == listenfd && fds[i].revents & POLLIN)
            {
                client tmp;
                socklen_t length = sizeof(tmp.address);
                int connfd = accept(listenfd, (sockaddr*)&tmp.address, &length);
                if(connfd < 0)
                {
                    std::cout << "errno is " << errno << std::endl;
                    continue;
                }
                // 由客户请求连接
                if(user_limits == user_count)
                {
                    const char* msg = "too many users";
                    puts(msg);
                    send(connfd, msg, strlen(msg), 0);
                    close(connfd);
                    continue;
                }
                setnonblock(connfd);
                user_count++;
                fds[user_count].fd = connfd;
                fds[user_count].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_count].revents = 0;
                clntMap.insert(std::pair<int, client>(connfd, tmp));
                printf("comes a new user, now have %d users\n", user_count);
            }
            else if(fds[i].revents & POLLERR)
            {
                perror("ERROR");
                continue;
            }
            else if(fds[i].revents & POLLRDHUP)
            {
                // 断开连接
                clntMap.erase(fds[i].fd);
                close(fds[i].fd);
                fds[i]  = fds[user_count];
                i--;
                user_count--;
                puts("disconnected...");
            }
            else if(fds[i].revents & POLLIN)
            {
                client& readclnt = clntMap[fds[i].fd];
                memset(readclnt.readbuf, 0, sizeof(readclnt.readbuf));
                // 有数据可读
                int len = recv(fds[i].fd, readclnt.readbuf, BUF_SIZE, 0);
                if(len < 0)
                {
                    if(errno != EAGAIN)
                    {
                        clntMap.erase(fds[i].fd);
                        close(fds[i].fd);
                        fds[i]  = fds[user_count];
                        i--;
                        user_count--;
                        puts("recv error");
                    }
                }
                else if(len > 0)
                {
                    // 把该数据写到其他客户端
                    for(int j=1;j<=user_count;j++)
                    {
                        if(i == j) continue;
                        client& tmp = clntMap[fds[j].fd];
                        tmp.writebuf = readclnt.readbuf;
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                    }
                }
            }
            else if(fds[i].revents & POLLOUT)
            {
                // 可以向客户端写入数据
                client&writeclnt = clntMap[fds[i].fd];
                if(writeclnt.writebuf)
                {
                    send(fds[i].fd, writeclnt.writebuf, strlen(writeclnt.writebuf), 0);
                    writeclnt.writebuf = NULL;
                    // 写完之后重新注册事件
                    fds[i].events |= ~POLLOUT;
                    fds[i].events |= POLLIN;
                }
            }
        }
    }

    close(listenfd);
    return 0;
}