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
#include <netinet/tcp.h>

const int buf_size = 1024 * 3;
char buf[buf_size];
int main(int argc, const char* argv[])
{
    int clnt_sock;
    sockaddr_in serv_addr;
    
    assert(argc == 4);
    clnt_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(clnt_sock != -1);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    // 设置发送缓冲区，最小值是4608
    int send_buf = atoi(argv[3]);
    int sned_buf_len = sizeof(send_buf);
    int off = 0;
    // setsockopt(clnt_sock, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
    setsockopt(clnt_sock, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
    getsockopt(clnt_sock, SOL_SOCKET, SO_SNDBUF, &send_buf, (socklen_t*)&sned_buf_len);
    std::cout << "send buf size : " << send_buf << std::endl;
    assert(connect(clnt_sock, (sockaddr*)&serv_addr, sizeof(sockaddr)) != -1);
    memset(buf, 'a', sizeof(buf));
    int send_len = send(clnt_sock, buf, sizeof(buf), 0);
    std::cout << "send_len : " << send_len << std::endl;

    sleep(10);
    close(clnt_sock);
    return 0;
}